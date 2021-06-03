/**
 * \file
 *
 * \brief Empty user application template
 *
 */
#include <asf.h>
#include <main.h>


/// @cond 0
/**INDENT-OFF**/
#ifdef __cplusplus
extern "C" {
#endif
/**INDENT-ON**/
/// @endcond



/**
 *  \brief Configure UART console.
 */
static void configure_console(void)
{
	const usart_serial_options_t uart_serial_options = {
		.baudrate = CONF_UART_BAUDRATE,
#ifdef CONF_UART_CHAR_LENGTH
        .charlength = CONF_UART_CHAR_LENGTH,
#endif
        .paritytype = CONF_UART_PARITY,
#ifdef CONF_UART_STOP_BITS
        .stopbits = CONF_UART_STOP_BITS,
#endif
	};
	
	/* Configure console UART. */
	sysclk_enable_peripheral_clock(CONSOLE_UART_ID);
	stdio_serial_init(CONF_UART, &uart_serial_options);
}

/************************************************************************/
/*    Map function form Arduino                                         */
/************************************************************************/

long map(long x, long in_min, long in_max, long out_min, long out_max) {
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

/*
 *	\brief Send commands in function of the received Art-Net data
 *
 * Art-Net functionality			                                    
 *	
 *	channel n: Wireless connectivity between nodes (not implemented)
 *		 0-20	: Each node functions for itself	
 *	channel n+1: Slave node 1 function
 *		 0-30   : Sensor disabled
 *		 31-60  : Sensor active on hue
 *		 61-90  : Sensor active on saturation
 *		 91-120 : Sensor active on intensity
 *		 121-150: Sensor disabled, receiving data to hue (not implemented)
 *		 151-180: Sensor disabled, receiving data to saturation (not implemented)
 *		 181-210: Sensor disabled, receiving data to intensity (not implemented)
 *		 211-255: Reset node (not implemented)
 *	channel n+2: Hue
 *	channel n+3: Saturation
 *	channel n+4: Dimmer
 *	channel n+5: Slave node 2 function
 *		 0-30   : Sensor disabled
 *		 31-60  : Sensor active on hue
 *		 61-90  : Sensor active on saturation
 *		 91-120 : Sensor active on intensity
 *		 121-150: Sensor disabled, receiving data to hue (not implemented)
 *		 151-180: Sensor disabled, receiving data to saturation (not implemented)
 *		 181-210: Sensor disabled, receiving data to intensity (not implemented)
 *		 211-255: Reset node (not implemented)
 *	channel n+6: Hue
 *	channel n+7: Saturation
 *	channel n+8: Dimmer
 *	channel n+9: Slave node 3 function
 *		 0-30   : Sensor disabled
 *		 31-60  : Sensor active on hue
 *		 61-90  : Sensor active on saturation
 *		 91-120 : Sensor active on intensity
 *		 121-150: Sensor disabled, receiving data to hue (not implemented)
 *		 151-180: Sensor disabled, receiving data to saturation (not implemented)
 *		 181-210: Sensor disabled, receiving data to intensity (not implemented)
 *		 211-255: Reset node (not implemented)
 *	channel n+10: Hue
 *	channel n+11: Saturation
 *	channel n+12: Dimmer
 *	channel n+13: Slave node 4 function
 *		 0-30   : Sensor disabled
 *		 31-60  : Sensor active on hue
 *		 61-90  : Sensor active on saturation
 *		 91-120 : Sensor active on intensity
 *		 121-150: Sensor disabled, receiving data to hue (not implemented)
 *		 151-180: Sensor disabled, receiving data to saturation (not implemented)
 *		 181-210: Sensor disabled, receiving data to intensity (not implemented)
 *		 211-255: Reset node (not implemented)
 *	channel n+14: Hue
 *	channel n+15: Saturation
 *	channel n+16: Dimmer
 *	
*/
static void artnetToCommand(void)
{
	__disable_irq(); // Set PRIMASK

	//NVIC_DisableIRQ(GMAC_IRQn);
	
	uint8_t nodeFunction;
	uint8_t currentNode = 0;
	dataOut.srcNode = 0;
	uint8_t masterData = artnet_data_buffer[artnetDmxAddress-1];
	uint16_t i = artnetDmxAddress -1; //Array starts at 0 but Art-Net data array had the first data byte at 0
//masterNode data - takes 1 channel starting at n
/*	if (masterData<=20){
		Each node to itself 
	}
	if (masterData>=21 && masterData<=40){
		//Node 2 -> 1
	}
	if (masterData>=41 && masterData<=60){
		//Node 3 -> 1
	}
	if (masterData>=61 && masterData<=80){
		//Node 4 -> 1
	}
	if (masterData>=81 && masterData<=100){
		//Node 1 -> 2
	}
	if (masterData>=101 && masterData<=120){
		//Node 3 -> 2
	}
	if (masterData>=121 && masterData<=140){
		//Node 4 -> 2
	}
	if (masterData>=141 && masterData<=160){
		//Node 1 -> 3
	}
	if (masterData>=161 && masterData<=180){
		//Node 2 -> 3
	}
	if (masterData>=181 && masterData<=200){
		//Node 4 -> 3
	}
	if (masterData>=201 && masterData<=220){
		//Node 1 -> 4
	}
	if (masterData>=221 && masterData<=240){
		//Node 2 -> 4
	}
	if (masterData>=241 && masterData<=255){
		//Node 3 -> 4
	}
*/
	currentNode++;	
//slaveNode data - takes 4 channels starting from n+1
	for(i = artnetDmxAddress; i < (artnetDmxAddress + (nodes * 4)); i++)
	{
		nodeFunction = artnet_data_buffer[i++]; //use i, then increment
		dataOut.hue = artnet_data_buffer[i++];
		dataOut.saturation = artnet_data_buffer[i++];
		dataOut.intensity = artnet_data_buffer[i];
#ifdef _DEBUG_
	printf("Node %d | HSV %d, %d, %d\r\n", currentNode, dataOut.hue, dataOut.saturation, dataOut.intensity);
#endif		
		if (nodeFunction <= 30)
		{
			//disabled
			nRF24_openWritingPipe(listeningPipes[currentNode]);
			dataOut.destNode = currentNode;
			dataOut.senCommand = disabled;
			
			if(!nRF24_write(&dataOut, sizeof(dataOut)))
			{
#ifdef _DEBUG_
	printf("transmission failed\n\r");
#endif
			}
			
#ifdef _DEBUG_
	printf("disable sensor node %d\r\n", currentNode);
#endif			
		}
		else if (nodeFunction >= 31 && nodeFunction <= 60)
		{
			//active_hue
			nRF24_openWritingPipe(listeningPipes[currentNode]);
			dataOut.destNode = currentNode;
			dataOut.senCommand = active_hue;
			
			if(!nRF24_write(&dataOut, sizeof(dataOut)))
			{
				#ifdef _DEBUG_
				printf("transmission failed\n\r");
				#endif
			}
			
#ifdef _DEBUG_
	printf("Sensor active_hue node %d\r\n", currentNode);
#endif
		}
		else if (nodeFunction >= 61 && nodeFunction <= 90)
		{
			//active_sat
			nRF24_openWritingPipe(listeningPipes[currentNode]);
			dataOut.destNode = currentNode;
			dataOut.senCommand = active_sat;
			
			if(!nRF24_write(&dataOut, sizeof(dataOut)))
			{
				#ifdef _DEBUG_
				printf("transmission failed\n\r");
				#endif
			}
			
#ifdef _DEBUG_
	printf("Sensor active_sat node %d\r\n", currentNode);
#endif
		}
		else if (nodeFunction >= 91 && nodeFunction <= 120)
		{
			//active_int
			nRF24_openWritingPipe(listeningPipes[currentNode]);
			dataOut.destNode = currentNode;
			dataOut.senCommand = active_int;
			
			if(!nRF24_write(&dataOut, sizeof(dataOut)))
			{
				#ifdef _DEBUG_
				printf("transmission failed\n\r");
				#endif
			}
			
#ifdef _DEBUG_
	printf("Sensor active_sat node %d\r\n", currentNode);
#endif
		}
		else if (nodeFunction >= 121 && nodeFunction <= 150)
		{
			//receive_hue
		}
		if (nodeFunction >= 151 && nodeFunction <= 180)
		{
			//receive_sat
		}
		if (nodeFunction >= 181 && nodeFunction <= 210)
		{
			//receive_int
		}
		if (nodeFunction >= 211)
		{
			//reset
		}
		
		currentNode++;
	}//end for-loop
	__enable_irq(); // Clear PRIMASK
	//NVIC_EnableIRQ(GMAC_IRQn);

}

int main (void)
{
	/* Insert system clock initialization code here (sysclk_init()). */
	sysclk_init();
	board_init();
	
	/* Initialize the console UART. */
	configure_console();
	puts(STRING_HEADER);

	//functies zijn overbodig omdat feedback niet gegeven kan worden
	fill_ArtNode(&ArtNode);
	//fill_ArtPollReply(&ArtPollReply, &ArtNode);	

	if (!init_gmac_ethernet())
	{
		return -1;
	}
	
#ifdef _DEBUG_
	// Display MAC & IP settings
	printf("-- MAC %x:%x:%x:%x:%x:%x\n\r",
	gs_uc_mac_address[0], gs_uc_mac_address[1], gs_uc_mac_address[2],
	gs_uc_mac_address[3], gs_uc_mac_address[4], gs_uc_mac_address[5]);

	printf("-- IP  %d.%d.%d.%d\n\r", gs_uc_ip_address[0], gs_uc_ip_address[1],
	gs_uc_ip_address[2], gs_uc_ip_address[3]);
	
	puts("link detected\r");
#endif

	spi_master_initialize();
	nRF24_begin();
	nRF24_setPALevel(RF_PA_HIGH);
	nRF24_stopListening();
	
#ifdef _DEBUG_
	printDetails();
#endif	
	
	while(1)
	{
		// Process packets
		if (GMAC_OK == read_dev_gmac()) {
			if (ul_frm_size_rx > 0) {
				// Handle input frame
				if(handleGMAC_Packet((uint8_t *) gs_uc_eth_buffer_rx, ul_frm_size_rx)){
					artnetToCommand();
				}//end handle 
			}//end of framesize
		}//end read_GMAC
	}//end of loop
}//end of program

/*
	- Check if UDP -
	- Check if ArtNet -
	- Check ArtNet packetType -
	- Handle packetType -
*/
bool handleGMAC_Packet(uint8_t *p_uc_data, uint32_t ul_size){
	p_ethernet_header_t p_eth = (p_ethernet_header_t) p_uc_data;
	p_T_ArtPoll p_artPoll_packet = (p_T_ArtPoll) (p_uc_data + ETH_HEADER_SIZE + ETH_IP_HEADER_SIZE + ICMP_HEADER_SIZE);
	p_T_ArtDmx p_artDmx_packet = (p_T_ArtDmx) (p_uc_data + ETH_HEADER_SIZE + ETH_IP_HEADER_SIZE + ICMP_HEADER_SIZE);
	//p_T_ArtAddress p_artAdress_packet = (p_T_ArtAddress) (p_uc_data + ETH_HEADER_SIZE + ETH_IP_HEADER_SIZE + ICMP_HEADER_SIZE);
	uint16_t eth_pkt_format = SWAP16(p_eth->et_protlen);
	uint32_t hdr_len = ETH_HEADER_SIZE + ETH_IP_HEADER_SIZE + UDP_HEADER_SIZE;
	
	if(eth_pkt_format == ETH_PROT_IPV4){
		p_ip_header_t p_ip = (p_ip_header_t) (p_uc_data+ ETH_HEADER_SIZE);
		if (p_ip->ip_p == IP_PROT_UDP){
			/*Check on added Art-Net header*/
#ifdef _DEBUG_
	printf("M: UDP\r\n");
#endif
/************************************************************************/
/* Controle op Art-Net anders uitvoeren                                 */
/************************************************************************/
			if (ul_size > hdr_len){
				PacketType = (T_ArtPacketType) get_packet_type(p_uc_data + ETH_HEADER_SIZE + ETH_IP_HEADER_SIZE + ICMP_HEADER_SIZE);
				if(PacketType == FAULTY_PACKET){  // bad packet
					return 0;
				}	
				//printf("M: Art-Net compatible\r\n");
				if(PacketType == ARTNET_DMX){
					/*if(sizeof(packetBuffer) < sizeof(T_ArtDmx)){
						return 0;
					}
					else{*/
#ifdef _DEBUG_
	printf("M: DMX\r\n");
#endif
						if(p_artDmx_packet->SubUni == ArtNode.swout[0])
						{
							//memcpy (artnet_data_buffer, (uint8_t *)packet->Data, MaxDataLength);
							memcpy(artnet_data_buffer, p_artDmx_packet->Data, SWAP16(p_artDmx_packet->Length)); //mempcy(dst, src, arraylength);
							//printf("M: DMX saved\r\n");
						}
					//}
				}
				else if(PacketType == ARTNET_POLL){
					/*if(sizeof(packetBuffer) < sizeof(T_ArtPoll)){ 
						return 0;
					}
					else{*/
#ifdef _DEBUG_
	printf("M: ArtPoll\r\n");
#endif
						//handle_poll(p_artPoll_packet, p_uc_data);
						if((p_artPoll_packet->Flags & 8) == 1) // controller say: send unicast reply
						{
							//send_reply(UNICAST, p_uc_data, (uint8_t *)&ArtPollReply);
							//printf("M: ArtPollReply Unicast\r\n");
						}
						else // controller say: send broadcast reply
						{
							//send_reply(BROADCAST, p_uc_data, (uint8_t *)&ArtPollReply);
							//printf("M: ArtPollReply Broadcast\r\n");
						}
						return 0;
					//}
				} 
/*				else if(packet_type == ARTNET_IPPROG)
				{
					if(sizeof(packetBuffer) < sizeof(artnet_ipprog_t))
						return;
					else
						handle_ipprog((artnet_ipprog_t *)&packetBuffer);
				} */
				else if(PacketType == ARTNET_ADDRESS){
					/*if(sizeof(packetBuffer) < sizeof(T_ArtAddress)){
						return 0;
					}
					else{*/
						//handle_address(p_artAdress_packet, p_uc_data);
						return 0;
					//}
				}
			}
		}
		else if(p_ip->ip_p == IP_PROT_ICMP)
		{
			gmac_process_ICMP_packet(p_uc_data, ul_size);
			return 0;
		}
	}
	else if(eth_pkt_format == ETH_PROT_ARP){
		gmac_process_arp_packet(p_uc_data, ul_size);
		return 0;
	}
	else{
#ifdef _DEBUG_
	printf("=== Default w_pkt_format= 0x%X===\n\r", eth_pkt_format);
#endif
		return 0;	
	}
	return 1;
}

void fill_ArtNode(T_ArtNode *node)
{
	
	//fill to 0's
	memset (node, 0, sizeof(node));
	
	//fill data
	memcpy (node->mac, factory_mac, 6);                   // the mac address of node
	memcpy (node->localIp, factory_localIp, 4);           // the IP address of node
	memcpy (node->broadcastIp, factory_broadcastIp, 4);   // broadcast IP address
	memcpy (node->gateway, factory_gateway, 4);           // gateway IP address
	memcpy (node->subnetMask, factory_subnetMask, 4);     // network mask (art-net use 'A' network type)
	
	sprintf((char *)node->id, "Art-Net\0"); // *** don't change never ***
	sprintf((char *)node->shortname, "Control node\0");
	sprintf((char *)node->longname, "Interactive System Master Control Node (c) Robbie Smedts\0");
	
	memset (node->porttypes,  0x80, 4);
	memset (node->goodinput,  0x08, 4);
	//memset (node->goodoutput, 0x00, 4);
	
	
	node->subH           = 0x00;        // high byte of the Node Subnet Address (This field is currently unused and set to zero. It is
	// provided to allow future expansion.) (art-net III)
	node->sub            = 0x00;        // low byte of the Node Subnet Address
	
	// **************************** art-net address of universes **********************************
	node->swout      [0] = 0x00;        // This array defines the 8 bit Universe address of the available output channels.
	node->swout      [1] = 0x01;        // values from 0x00 to 0xFF
	node->swout      [2] = 0x02;
	node->swout      [3] = 0x03;
	
	// not implemented yet
	node->swin       [0] = 0x00;        // This array defines the 8 bit Universe address of the available input channels.
	node->swin       [1] = 0x01;        // values from 0x00 to 0xFF
	node->swin       [2] = 0x02;
	node->swin       [3] = 0x03;
	

	node->goodoutput [0] = 0x80;

	node->etsamanH = 'S';        // The ESTA manufacturer code.
	node->etsamanL = 'R';        // The ESTA manufacturer code.
	node->localPort  = 0x1936;   // artnet UDP port is by default 6454 (0x1936)
	node->verH       = 0;        // high byte of Node firmware revision number.
	node->ver        = 1;        // low byte of Node firmware revision number.
	node->ProVerH    = 0;        // high byte of the Art-Net protocol revision number.
	node->ProVer     = 14;       // low byte of the Art-Net protocol revision number.
	node->oemH       = 0;        // high byte of the oem value.
	node->oem        = 0xFF;     // low byte of the oem value. (0x00FF = developer code)
	node->ubea       = 0;        // This field contains the firmware version of the User Bios Extension Area (UBEA). 0 if not used
	node->status     = 0x08;
	node->swvideo    = 0;
	node->swmacro    = 0;
	node->swremote   = 0;
	node->style      = 0;        // StNode style - A DMX to/from Art-Net device
}
/*
void fill_ArtPollReply(T_ArtPollReply *poll_reply, T_ArtNode *node)
{
	//fill to 0's
	memset (poll_reply, 0, sizeof(poll_reply));
	
	//copy data from node
	memcpy (poll_reply->ID, node->id, sizeof(poll_reply->ID));
	memcpy (poll_reply->BoxAddr.IP, node->localIp, sizeof(poll_reply->BoxAddr.IP));
	memcpy (poll_reply->Mac, node->mac, sizeof(poll_reply->Mac));
	memcpy (poll_reply->ShortName, node->shortname, sizeof(poll_reply->ShortName));
	memcpy (poll_reply->LongName, node->longname, sizeof(poll_reply->LongName));
	memcpy (poll_reply->NodeReport, node->nodereport, sizeof(poll_reply->NodeReport));
	memcpy (poll_reply->PortTypes, node->porttypes, sizeof(poll_reply->PortTypes));
	memcpy (poll_reply->GoodInput, node->goodinput, sizeof(poll_reply->GoodInput));
	memcpy (poll_reply->GoodOutputA, node->goodoutput, sizeof(poll_reply->GoodOutputA));
	memcpy (poll_reply->SwIn, node->swin, sizeof(poll_reply->SwIn));
	memcpy (poll_reply->SwOut, node->swout, sizeof(poll_reply->SwOut));
	memcpy (poll_reply->EstaManHi, node->etsamanH, sizeof(poll_reply->EstaManHi));
	memcpy (poll_reply->EstaManLo, node->etsamanL, sizeof(poll_reply->EstaManLo));
	
	sprintf((char *)poll_reply->NodeReport, "%i nRF output universe active.\0", node->numbports);
	
	poll_reply->OpCode = 0x2100;  // ARTNET_REPLY
	poll_reply->BoxAddr.Port = node->localPort;
	poll_reply->VersionInfoHi = node->verH;
	poll_reply->VersionInfoLo = node->ver;
	poll_reply->NetSwitch = node->subH;
	poll_reply->SubSwitch = node->sub;
	poll_reply->OemHi = node->oemH;
	poll_reply->OemLo = node->oem;
	poll_reply->Status = node->status;
	poll_reply->NumPortsHi = node->numbportsH;
	poll_reply->NumPortsLo = node->numbports;
	poll_reply->SwMacro         = node->swmacro;
	poll_reply->SwRemote        = node->swremote;
	poll_reply->Style           = node->style;
} 
*/
void handle_address(p_T_ArtAddress *packet, uint8_t *p_uc_data) //Not properly implemented yet
{
	send_reply(UNICAST, p_uc_data, (uint8_t *)&ArtPollReply);
#ifdef _DEBUG_
	printf("M: Address unicast\r\n");
#endif
}

T_ArtPacketType get_packet_type(uint8_t *packet) //this get artnet packet type
{
	if (! memcmp( packet, ArtNode.id, 8))
	{
		return BYTES_TO_SHORT(packet[9], packet[8]);
	}
	return 0;  // bad packet
}

void send_reply(uint8_t mode_broadcast, uint8_t *p_uc_data, uint8_t *packet)
{
	uint8_t ul_rc = GMAC_OK;
	uint32_t ul_size;
	uint8_t i;
	p_ethernet_header_t p_eth = (p_ethernet_header_t) p_uc_data;
	p_ip_header_t p_ip = (p_ip_header_t) (p_uc_data + ETH_HEADER_SIZE);
	p_udp_header_t p_udp = (p_udp_header_t) (p_uc_data + ETH_HEADER_SIZE + ETH_IP_HEADER_SIZE);
	p_T_ArtPollReply p_artPR = (p_T_ArtPollReply) (p_uc_data + ETH_HEADER_SIZE + ETH_IP_HEADER_SIZE + UDP_HEADER_SIZE);
	
	//Swap ethernet [MAC] source and destination address
	for (i = 0; i < 6; i++) {
		p_eth->et_dest[i] = p_eth->et_src[i];
		p_eth->et_src[i] = gs_uc_mac_address[i];
	}
	
	//merge ArtPollReply into GMAC data
	memcpy(p_artPR, packet, sizeof(T_ArtPollReply));//dest, src, len
	
	p_udp->udp_len = sizeof(T_ArtPollReply);
	
	//calculate size of the GMAC macket
	ul_size = ETH_HEADER_SIZE + ETH_IP_HEADER_SIZE + UDP_HEADER_SIZE + sizeof(T_ArtPollReply);
	
	if(mode_broadcast == 1) // send broadcast packet
	{
		//Fill in source IP address
		for (i = 0; i < 4; i++) {
			p_ip->ip_dst[i] = 255;
			p_ip->ip_src[i] = gs_uc_ip_address[i];
		}
		ul_rc = gmac_dev_write(&gs_gmac_dev, *p_uc_data, GMAC_QUE_0, ul_size, NULL);
	}
	else // send unicast packet to controller
	{
		//Fill in source IP address
		for (i = 0; i < 4; i++) {
			p_ip->ip_dst[i] =p_ip->ip_src[i];
			p_ip->ip_src[i] = gs_uc_ip_address[i];
		}
		ul_rc = gmac_dev_write(&gs_gmac_dev, *p_uc_data, GMAC_QUE_0, ul_size, NULL);
	}
	
#ifdef _DEBUG_
	if (ul_rc != GMAC_OK)
	{
	printf("E: ArtPollReply not send");
	}
	else
	{
	printf("M: ArtPollReply send\r\n");
	}
#endif
	
}

/// @cond 0
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/// @endcond