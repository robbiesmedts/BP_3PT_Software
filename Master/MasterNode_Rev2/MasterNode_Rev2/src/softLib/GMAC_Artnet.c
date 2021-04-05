/*
 * GMAC_Artnet.c
 *
 * Created: 12/09/2020 19:28:52
 * Author: Design
 */ 

#include "GMAC_Artnet.h"
#include "softLib/ArtNet/Art-Net.h"

uint32_t read_dev_gmac(void)
{
	return gmac_dev_read(&gs_gmac_dev, GMAC_QUE_0, (uint8_t *) gs_uc_eth_buffer_rx, sizeof(gs_uc_eth_buffer_rx), &ul_frm_size_rx);
}

/**
 * \brief Send ulLength bytes from pcFrom. This copies the buffer to one of the
 * GMAC Tx buffers, and then indicates to the GMAC that the buffer is ready.
 * If lEndOfFrame is true then the data being copied is the end of the frame
 * and the frame can be transmitted.
 *
 * \param p_gmac_dev Pointer to the GMAC device instance.
 * \param p_buffer       Pointer to the data buffer.
 * \param ul_size    Length of the frame.
 * \param func_tx_cb  Transmit callback function.
 *
 * \return Length sent.
 */
uint32_t write_dev_gmac(void *p_buffer, uint32_t ul_size)
{
	return gmac_dev_write(&gs_gmac_dev, GMAC_QUE_0, p_buffer, ul_size, NULL);
}

/** The MAC address used for the test */
uint8_t gs_uc_mac_address[] =
{ ETHERNET_CONF_ETHADDR0, ETHERNET_CONF_ETHADDR1, ETHERNET_CONF_ETHADDR2, ETHERNET_CONF_ETHADDR3, ETHERNET_CONF_ETHADDR4, ETHERNET_CONF_ETHADDR5};

/** The IP address used for test (ping ...) */
uint8_t gs_uc_ip_address[] =
{ ETHERNET_CONF_IPADDR0, ETHERNET_CONF_IPADDR1, ETHERNET_CONF_IPADDR2, ETHERNET_CONF_IPADDR3 };

/* Art-Net identification string*/
static uint8_t artnet_id[] =
{0x41, 0x72, 0x74, 0x2D, 0x4E, 0x65, 0x74, 0x00}; // ascii for "Art-Net"

/** The GMAC driver instance */
gmac_device_t gs_gmac_dev;

/** Buffer for ethernet packets */
volatile uint8_t gs_uc_eth_buffer_rx[GMAC_FRAME_LENTGH_MAX];
volatile uint8_t gs_uc_eth_buffer_tx[GMAC_FRAME_LENTGH_MAX];

/** Buffer for Artnet DMX data */
uint8_t artnet_data_buffer[512];

uint32_t ul_frm_size_rx, ul_frm_size_tx;
volatile uint32_t ul_delay;
gmac_options_t gmac_option;
T_Addr p_artAddr;
T_ArtPollReply p_artPollReply_packet;

static uint16_t gmac_icmp_checksum(uint16_t *p_buff, uint32_t ul_len)
{
	uint32_t i, ul_tmp;

	for (i = 0, ul_tmp = 0; i < ul_len; i++, p_buff++) {

		ul_tmp += SWAP16(*p_buff);
	}
	ul_tmp = (ul_tmp & 0xffff) + (ul_tmp >> 16);

	return (uint16_t) (~ul_tmp);
}

bool init_gmac_ethernet(void)
{
	#ifdef ETH_SUPPORT_AT24MAC
	at24mac_get_mac_address();
	#endif


	// Wait for PHY to be ready (CAT811: Max400ms)
	ul_delay = sysclk_get_cpu_hz() / 1000 / 3 * 400;
	while (ul_delay--);

	// Enable GMAC clock
	pmc_enable_periph_clk(ID_GMAC);

	// Fill in GMAC options
	gmac_option.uc_copy_all_frame = 0;
	gmac_option.uc_no_boardcast = 0;

	memcpy(gmac_option.uc_mac_addr, gs_uc_mac_address, sizeof(gs_uc_mac_address));

	gs_gmac_dev.p_hw = GMAC;

	// Init GMAC driver structure
	gmac_dev_init(GMAC, &gs_gmac_dev, &gmac_option);

	// Enable Interrupt
	NVIC_EnableIRQ(GMAC_IRQn);

	// Init MAC PHY driver
	if (ethernet_phy_init(GMAC, BOARD_GMAC_PHY_ADDR, sysclk_get_cpu_hz())
	!= GMAC_OK) {
		puts("PHY Initialize ERROR!\r");
		return 0;
	}

	// Auto Negotiate, work in RMII mode
	if (ethernet_phy_auto_negotiate(GMAC, BOARD_GMAC_PHY_ADDR) != GMAC_OK) {

		puts("Auto Negotiate ERROR!\r");
		return 0;
	}

	// Establish ethernet link
	while (ethernet_phy_set_link(GMAC, BOARD_GMAC_PHY_ADDR, 1) != GMAC_OK) {
		puts("Set link ERROR!\r");
		return 0;
	}
	
	for(uint8_t i = 0; i<4; i++)
		p_artAddr.IP[i] = gs_uc_ip_address[i];
	
	p_artAddr.Port = 0x6391;
	
	return 1;
}

void gmac_process_arp_packet(uint8_t *p_uc_data, uint32_t ul_size)
{
	uint32_t i;
	uint8_t ul_rc = GMAC_OK;

	p_ethernet_header_t p_eth = (p_ethernet_header_t) p_uc_data;
	p_arp_header_t p_arp = (p_arp_header_t) (p_uc_data + ETH_HEADER_SIZE);

	if (SWAP16(p_arp->ar_op) == ARP_REQUEST) {
		printf("-- MAC %x:%x:%x:%x:%x:%x\n\r",
		p_eth->et_dest[0], p_eth->et_dest[1],
		p_eth->et_dest[2], p_eth->et_dest[3],
		p_eth->et_dest[4], p_eth->et_dest[5]);

		printf("-- MAC %x:%x:%x:%x:%x:%x\n\r",
		p_eth->et_src[0], p_eth->et_src[1],
		p_eth->et_src[2], p_eth->et_src[3],
		p_eth->et_src[4], p_eth->et_src[5]);

		/* ARP reply operation */
		p_arp->ar_op = SWAP16(ARP_REPLY);

		/* Fill the destination address and source address */
		for (i = 0; i < 6; i++) {
			/* Swap ethernet destination address and ethernet source address */
			p_eth->et_dest[i] = p_eth->et_src[i];
			p_eth->et_src[i] = gs_uc_mac_address[i];
			p_arp->ar_tha[i] = p_arp->ar_sha[i];
			p_arp->ar_sha[i] = gs_uc_mac_address[i];
		}
		/* Swap the source IP address and the destination IP address */
		for (i = 0; i < 4; i++) {
			p_arp->ar_tpa[i] = p_arp->ar_spa[i];
			p_arp->ar_spa[i] = gs_uc_ip_address[i];
		}

		ul_rc = gmac_dev_write(&gs_gmac_dev, GMAC_QUE_0, p_uc_data, ul_size, NULL);

		if (ul_rc != GMAC_OK) {
			printf("E: ARP Send - 0x%x\n\r", ul_rc);
		}
	}
}

void gmac_process_ICMP_packet(uint8_t *p_uc_data, uint32_t ul_size)
{
	uint32_t i;
	uint32_t ul_icmp_len;
	int32_t ul_rc = GMAC_OK;

	/* avoid Cppcheck Warning */
	UNUSED(ul_size);

	p_ethernet_header_t p_eth = (p_ethernet_header_t) p_uc_data;
	p_ip_header_t p_ip_header = (p_ip_header_t) (p_uc_data + ETH_HEADER_SIZE);

	p_icmp_echo_header_t p_icmp_echo = (p_icmp_echo_header_t) ((int8_t *) p_ip_header + ETH_IP_HEADER_SIZE);
		
	if (p_icmp_echo->type == ICMP_ECHO_REQUEST) {
		p_icmp_echo->type = ICMP_ECHO_REPLY;
		p_icmp_echo->code = 0;
		p_icmp_echo->cksum = 0;

		/* Checksum of the ICMP message */
		ul_icmp_len = (SWAP16(p_ip_header->ip_len) - ETH_IP_HEADER_SIZE);
		if (ul_icmp_len % 2) {
			*((uint8_t *) p_icmp_echo + ul_icmp_len) = 0;
			ul_icmp_len++;
		}
		ul_icmp_len = ul_icmp_len / sizeof(uint16_t);

		p_icmp_echo->cksum = SWAP16(
		gmac_icmp_checksum((uint16_t *)p_icmp_echo, ul_icmp_len));
		/* Swap the IP destination  address and the IP source address */
		for (i = 0; i < 4; i++) {
			p_ip_header->ip_dst[i] =
			p_ip_header->ip_src[i];
			p_ip_header->ip_src[i] = gs_uc_ip_address[i];
		}
		/* Swap ethernet destination address and ethernet source address */
		for (i = 0; i < 6; i++) {
			/* Swap ethernet destination address and ethernet source address */
			p_eth->et_dest[i] = p_eth->et_src[i];
			p_eth->et_src[i] = gs_uc_mac_address[i];
		}
		/* Send the echo_reply */
		#if (SAM4E)
		ul_rc = gmac_dev_write(&gs_gmac_dev, p_uc_data,
		SWAP16(p_ip_header->ip_len) + 14, NULL);
		#else
		ul_rc = gmac_dev_write(&gs_gmac_dev, GMAC_QUE_0, p_uc_data,
		SWAP16(p_ip_header->ip_len) + 14, NULL);
		#endif
		if (ul_rc != GMAC_OK) {
			printf("E: ICMP Send - 0x%x\n\r", ul_rc);
		}
	}
}
/**
 * Function is from the example project and not further of service in this project
 * \brief Display the IP packet.
 *
 * \param p_ip_header Pointer to the IP header.
 * \param ul_size    The data size.
 */
void gmac_display_ip_packet(p_ip_header_t p_ip_header, uint32_t ul_size)
{
	printf("======= IP %4d bytes, HEADER ==========\n\r", (int)ul_size);
	printf(" IP Version        = v.%d", (p_ip_header->ip_hl_v & 0xF0) >> 4);
	printf("\n\r Header Length     = %d", p_ip_header->ip_hl_v & 0x0F);
	printf("\n\r Type of service   = 0x%x", p_ip_header->ip_tos);
	printf("\n\r Total IP Length   = 0x%X",
	(((p_ip_header->ip_len) >> 8) & 0xff) +
	(((p_ip_header->ip_len) << 8) & 0xff00));
	printf("\n\r ID                = 0x%X",
	(((p_ip_header->ip_id) >> 8) & 0xff) +
	(((p_ip_header->ip_id) << 8) & 0xff00));
	printf("\n\r Header Checksum   = 0x%X",
	(((p_ip_header->ip_sum) >> 8) & 0xff) +
	(((p_ip_header->ip_sum) << 8) & 0xff00));
	puts("\r Protocol          = ");

	switch (p_ip_header->ip_p) {
		case IP_PROT_ICMP:
		puts("ICMP");
		break;

		case IP_PROT_IP:
		puts("IP");
		break;

		case IP_PROT_TCP:
		puts("TCP");
		break;

		case IP_PROT_UDP:
		puts("UDP");
		break;

		default:
		printf("%d (0x%X)", p_ip_header->ip_p, p_ip_header->ip_p);
		break;
	}

	printf("\n\r IP Src Address    = %d:%d:%d:%d",
	p_ip_header->ip_src[0],
	p_ip_header->ip_src[1],
	p_ip_header->ip_src[2], p_ip_header->ip_src[3]);

	printf("\n\r IP Dest Address   = %d:%d:%d:%d",
	p_ip_header->ip_dst[0],
	p_ip_header->ip_dst[1],
	p_ip_header->ip_dst[2], p_ip_header->ip_dst[3]);
	puts("\n\r----------------------------------------\r");
}

static void display_ArtDmx_packet(p_T_ArtDmx p_ArtDmx_packet)
{
	printf("\n\r");
	printf("identifier: %c%c%c%c%c%c%c%c\n\r", (char)p_ArtDmx_packet->ID[0], (char)p_ArtDmx_packet->ID[1], (char)p_ArtDmx_packet->ID[2], (char)p_ArtDmx_packet->ID[3], (char)p_ArtDmx_packet->ID[4], (char)p_ArtDmx_packet->ID[5], (char)p_ArtDmx_packet->ID[6], (char)p_ArtDmx_packet->ID[7]);
	printf("Opcode: %X\n\r", p_ArtDmx_packet->OpCode);

		for (long i = 0; i<512; i++)
		{
			printf("Channel %ld: %d\n\r", i+1, p_ArtDmx_packet->Data[i]);
		}
}

static void gmac_process_artnet_packet(uint8_t *p_uc_data, uint32_t ul_size)
{
	int32_t ul_rc = GMAC_OK;
	p_art_packet_t p_art_packet = (p_art_packet_t) (p_uc_data + ETH_HEADER_SIZE + ETH_IP_HEADER_SIZE + ICMP_HEADER_SIZE);
	p_T_ArtPoll p_artPoll_packet = (p_T_ArtPoll) (p_uc_data + ETH_HEADER_SIZE + ETH_IP_HEADER_SIZE + ICMP_HEADER_SIZE);
	p_T_ArtDmx p_artDmx_packet = (p_T_ArtDmx) (p_uc_data + ETH_HEADER_SIZE + ETH_IP_HEADER_SIZE + ICMP_HEADER_SIZE);
	
	//printf("OpCode = %X\n\r", p_art_packet->art_OpCode);
	
	switch (p_art_packet->art_OpCode)
	{
	case OpPoll:
		for (uint8_t i = 0; i<8; i++)
		{
			p_artPollReply_packet.ID[i] = p_artPoll_packet->ID[i];
		}
		p_artPollReply_packet.OpCode = OpPollReply;
		p_artPollReply_packet.BoxAddr = p_artAddr;
		p_artPollReply_packet.VersionInfoLo = 0x01; //Version 1 of the firmware
		//p_artPollReply_packet.NetSwitch = ;
		//p_artPollReply_packet.SubSwitch = ;
		//p_artPollReply_packet.OemHi = 0x00; //OEM Unknown 0x00FF
		p_artPollReply_packet.OemLo = 0xFF;
		//p_artPollReply_packet.UbeaVerion = 0x00; //Not programmed
		p_artPollReply_packet.Status = 0b00000010;
		p_artPollReply_packet.EstaManLo = "S";
		p_artPollReply_packet.EstaManHi = "R";
		strcpy (p_artPollReply_packet.ShortName, "MstrTX");
		strcpy (p_artPollReply_packet.LongName, "Masternode for interactive theater system");
		//p_artPollReply_packet.NodeReport = ;
		//p_artPollReply_packet.NumPortsHi = 0;
		p_artPollReply_packet.NumPortsLo = 0;
		//p_artPollReply_packet.PortTypes[4] = {0, 0, 0, 0};
		//p_artPollReply_packet.GoodInput[4] = {0, 0, 0, 0};
		p_artPollReply_packet.GoodOutputA[0] = 0x80;
		//p_artPollReply_packet.SwIn[4] = {};
		//p_artPollReply_packet.SwOut[4] = {};
		//p_artPollReply_packet.AcnPriority = ;
		//p_artPollReply_packet.SwMacro = 0;
		//p_artPollReply_packet.SwRemote = 0;
		//p_artPollReply_packet.Spare1 = 0;
		//p_artPollReply_packet.Spare2 = 0;
		//p_artPollReply_packet.Spare3 = 0;
		//p_artPollReply_packet.Style = 0; //StNode - A DMX to/from Art-Net device
		
		for (uint8_t i = 0; i < 6; i++)
		{
			p_artPollReply_packet.Mac[i] = gs_uc_mac_address[i];	
		}
		//p_artPollReply_packet.BindIp[4] = {};
		//p_artPollReply_packet.BindIndex = 0;
		//p_artPollReply_packet.Status2 = 0;
		p_artPollReply_packet.GoodOutputB[0] = 0b11000000;
		p_artPollReply_packet.Status3 = 0b010000000;
		//p_artPollReply_packet.DefaultUiResponder = ;
		//p_artPollReply_packet.Filler[15] = {};	
		
		//send ArtPollReply
		
		puts("ArtPoll replied\r");
		break;
	case OpDmx:
		//printf("Universe: %d%d\n\r", p_artDmx_packet->Net, p_artDmx_packet->SubUni);
		if (p_artDmx_packet->SubUni == 0 && p_artDmx_packet->Net == 0)
		{
			//Length is send LSB first an received byte swapped, for the correct length we have to re-swap it
			memcpy(artnet_data_buffer, p_artDmx_packet->Data, SWAP16(p_artDmx_packet->Length)); //mempcy(dst, src, arraylength);
			puts("DMX saved\r");
			//display_ArtDmx_packet(p_artDmx_packet);
		}
		
		break;
	default:
		puts("Unimplemented OpCode");
		break;
	}
}

/**
 * \brief Process the received IP packet; change address and send it back.
 *
 * \param p_uc_data  The data to process.
 * \param ul_size The data size.
 */
static void gmac_process_ip_packet(uint8_t *p_uc_data, uint32_t ul_size)
{
	uint32_t i;
	uint8_t controle[8];
	uint32_t hdr_len = ETH_HEADER_SIZE + ETH_IP_HEADER_SIZE + ICMP_HEADER_SIZE;
	
	p_ip_header_t p_ip_header = (p_ip_header_t) (p_uc_data + ETH_HEADER_SIZE);
	if (p_ip_header ->ip_p == IP_PROT_UDP)
	{
		//puts("UDP packet\r\n");
		/*Check on added Art-Net header*/
		if (ul_size > hdr_len)
		{
			/*Check if the 8 following bytes of the headers are part of the Art-Net header*/
			for (i = 0; i < 8; i++)
			{
				controle[i] = p_uc_data[hdr_len+i];
			}
			/*Check if data read compares to "Art-Net"*/
			if (!compareArray(controle, artnet_id, 8))
			{	
				//printf("Art-Net compatible\r");
				gmac_process_artnet_packet(p_uc_data, ul_size);
			}
		}
	}
}

/**
 * \brief Process the received GMAC packet.
 *
 * \param p_uc_data  The data to process.
 * \param ul_size The data size.
 */
void gmac_process_eth_packet(uint8_t *p_uc_data, uint32_t ul_size)
{
	uint16_t us_pkt_format;

	p_ethernet_header_t p_eth = (p_ethernet_header_t) (p_uc_data);
	p_ip_header_t p_ip_header = (p_ip_header_t) (p_uc_data + ETH_HEADER_SIZE);
	ip_header_t ip_header;
	us_pkt_format = SWAP16(p_eth->et_protlen);

	if (us_pkt_format == ETH_PROT_IPV4)
	{
		/* Backup the header */
		//memcpy(&ip_header, p_ip_header, sizeof(ip_header_t)); //memcpy(dest, src, size)

		/* Process the IP packet */
		gmac_process_ip_packet(p_uc_data, ul_size);
		// Dump the IP header
		//gmac_display_ip_packet(&ip_header, ul_size);
	}
	else
	{
		printf("=== Default w_pkt_format= 0x%X===\n\r", us_pkt_format);
	}
}

/*
	Debugging purposes;
	Function prints the whole Ethernet buffer to the terminal
*/
void display_eth_buffer(uint8_t *eth_buffer, uint32_t ul_size)
{
	uint32_t b_size;
	for (b_size = 0; b_size < ul_size; b_size++)
	{
		printf("package %ld: %d\n\r", b_size, eth_buffer[b_size]);
	}
}

#ifdef ETH_SUPPORT_AT24MAC
void at24mac_get_mac_address(void)
{
	twihs_options_t opt;
	twihs_packet_t packet_mac_addr;
	uint8_t orginal_mac_addr[BOARD_AT24MAC_PAGE_SIZE];

	/* Enable TWI peripheral */
	pmc_enable_periph_clk(ID_TWIHS0);

	/* Init TWI peripheral */
	opt.master_clk = sysclk_get_cpu_hz();
	opt.speed = BOARD_AT24MAC_TWIHS_CLK;
	twihs_master_init(BOARD_AT24MAC_TWIHS, &opt);

	/* MAC address */
	packet_mac_addr.chip = BOARD_AT24MAC_ADDRESS;
	packet_mac_addr.addr[0] = 0x9A;
	packet_mac_addr.addr_length = 1;
	packet_mac_addr.buffer = orginal_mac_addr;
	packet_mac_addr.length = BOARD_AT24MAC_PAGE_SIZE;

	twihs_master_read(BOARD_AT24MAC_TWIHS, &packet_mac_addr);

	if ((orginal_mac_addr[0] == 0xFC) && (orginal_mac_addr[1] == 0xC2)
	&& (orginal_mac_addr[2] == 0x3D)) {
		for (uint8_t i = 0; i < 6; i++)
		gs_uc_mac_address[i] = orginal_mac_addr[i];
	}
}
#endif

/**
 * \brief GMAC interrupt handler.
 */
void GMAC_Handler(void)
{
	gmac_handler(&gs_gmac_dev, GMAC_QUE_0);
}

char compareArray(uint8_t a[],uint8_t b[],uint8_t size)	
{
	int i;
	for(i=0;i<size;i++){
		if(a[i]!=b[i])
		return 1;
	}
	return 0;
}