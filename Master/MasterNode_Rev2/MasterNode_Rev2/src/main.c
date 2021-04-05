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


#ifdef _DEBUG_
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
#endif

/************************************************************************/
/*    Map function form Arduino                                         */
/************************************************************************/

long map(long x, long in_min, long in_max, long out_min, long out_max) {
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

/************************************************************************/
/*
 *	\brief Send commands in function of the received Art-Net data
 *
 * Art-Net functionality			                                    
 *	
 *	channel n: General functionality node 0
 *	channel n+1: Extra functionality node 0
 *	channel n+2: General functionality node 1
 *	channel n+3: Extra functionality node 1
 *	channel n+4: General functionality node 2
 *	channel n+5: Extra functionality node 2
 *	channel n+6: General functionality node 3
 *	channel n+7: Extra functionality node 3
 *	
 * General functionality:
 *		0-10	| Node disabled
 *		11-20	| Sensor to own actuator
 *		21-30 	| Sensor to node w (dependable of source Node)
 *		31-40 	| Sensor to node x (dependable of source Node)
 *		41-50 	| Sensor to node y (dependable of source Node)
 *		51-60	| Sensor to node z (dependable of source Node)
 *		61-70	| Accel Tap
 *		71-80	| (future features)
 *		81-90	| External node input
 *		91-100	| Sensor feedback to lighting desk
 *		101-230	| Art-Net input
 *		231-255 | reset node (reset any saved sensor values and resets the actuator)
 *	
*/
static void artnetToCommand(void)
{
	uint8_t dmxValue = 0;
	uint8_t currentNode = 1;

	//array start at 0 so the dmx address must be lowered by 1 because Art-Net sends DMX data without zero byte
	for(uint16_t i = artnetDmxAddress-1; i < (artnetDmxAddress + nodes - 1); i++)
	{
		dmxValue = artnet_data_buffer[i];
/************************************************************************/
/* Node disabled                                                        */
/************************************************************************/
			if(dmxValue <= 10)
			{
					nRF24_openWritingPipe(listeningPipes[currentNode]);
					dataOut.command = 0;
					nRF24_write(&dataOut, sizeof(dataOut));
#ifdef _DEBUG_
					//printf("disable node %d\r\n", currentNode);
#endif
			}
/************************************************************************/
/* Read own sensor                                                      */
/************************************************************************/
			else if(dmxValue >= 11 && dmxValue <= 20)
			{		
					nRF24_openWritingPipe(listeningPipes[currentNode]);
					dataOut.command = 1;
					nRF24_write(&dataOut, sizeof(dataOut));
#ifdef _DEBUG_
					printf("Node %d read own sensor\r\n", currentNode);
#endif
			}
/************************************************************************/
/* Send sensor to other node                                            */
/************************************************************************/
/*			else if(dmxValue >= 21 && dmxValue <= 30)
			{
				//Sensor to Node n
				nRF24_openWritingPipe(listeningPipes[currentNode]);
				dataOut.command = 2;
				if (currentNode == 1)
				{
					dataOut.destAddr = listeningPipes[2];
				}
				else
				{
					dataOut.destAddr = listeningPipes[1];
				}
				nRF24_write(&dataOut, sizeof(dataOut));
#ifdef _DEBUG_
				printf("Send data from node %d to other node\r\n", currentNode);
#endif
			}
			else if(dmxValue >= 31 && dmxValue <= 40)
			{
				//Sensor to Node n+1
				nRF24_openWritingPipe(listeningPipes[currentNode]);
				dataOut.command = 2;
				if (currentNode == 1 || currentNode == 2)
				{
					dataOut.destAddr = listeningPipes[3];
				}
				else
				{
					dataOut.destAddr = listeningPipes[2];
				}
				nRF24_write(&dataOut, sizeof(dataOut));
#ifdef _DEBUG_
				printf("Send data from node %d to other node\r\n", currentNode);
#endif
			}
			
			else if(dmxValue >= 41 && dmxValue <= 50)
			{
				//Sensor to Node n+2
				nRF24_openWritingPipe(listeningPipes[currentNode]);
				dataOut.command = 2;
				if (currentNode == 4 || currentNode == 5)
				{
					dataOut.destAddr = listeningPipes[3];
				}
				else
				{
					dataOut.destAddr = listeningPipes[4];
				}
				nRF24_write(&dataOut, sizeof(dataOut));
#ifdef _DEBUG_
				printf("Send data from node %d to other node\r\n", currentNode);
#endif				
			}
			
			else if(dmxValue >= 51 && dmxValue <= 60)
			{
				//Sensor to node n+3
				nRF24_openWritingPipe(listeningPipes[currentNode]);
				dataOut.command = 2;
				if (currentNode == 5)
				{
					dataOut.destAddr = listeningPipes[4];
				}
				else
				{
					dataOut.destAddr = listeningPipes[5];
				}
				nRF24_write(&dataOut, sizeof(dataOut));
#ifdef _DEBUG_
				printf("Send data from node %d to other node\r\n", currentNode);
#endif
			}*/
/*			else if(61 < dmxValue < 80)
			{
				//voor extra nodes
			}*/
/*			else if (dmxValue >= 81 && dmxValue <= 90)
			{
				//send nothing to the node to avoid command collisions with the sensor data
			}*/
/*			else if (dmxValue >= 91 && dmxValue <= 100)
			{
				//send sensorData to Midi node for feedback to audio, light or video
				nRF24_openWritingPipe(listeningPipes[currentNode]);
				dataOut.command = 2;
				dataOut.destAddr = listeningPipes[6];
				nRF24_write(&dataOut, sizeof(dataOut));
#ifdef _DEBUG_
				printf("Send data from node %d to Midi node\r\n", currentNode);
#endif				
			}*/
/************************************************************************/
/* Use DMX value                                                        */
/************************************************************************/
			else if(dmxValue >= 101 && dmxValue <= 230)
			{
				//desk -> actuator
				nRF24_openWritingPipe(listeningPipes[currentNode]);
				//calculating and remapping received DMX values
				uint8_t m_dmx = dmxValue - 101;
				m_dmx = map(m_dmx, 0, 129, 0, 255);
				
				dataOut.datavalue = m_dmx;
				dataOut.command = 3;
#ifdef _DEBUG_
				printf("connect desk to node %d for %d\r\n", currentNode, m_dmx);
#endif // _DEBUG_
						
			}
/************************************************************************/
/* Reset Node		                                                    */
/************************************************************************/
/*			else if(dmxValue >= 231 && dmxValue <= 255)
			{
				//reset Node
				nRF24_openWritingPipe(listeningPipes[currentNode]);
				dataOut.command = 4;
				nRF24_write(&dataOut, sizeof(dataOut));
#ifdef _DEBUG_
				printf("Reset node %d\r\n", currentNode);
#endif
			}//end if structure*/
		//printf("current node: %d\n\r", currentNode);	
		currentNode++;
	}//end for-loop
}

int main (void)
{
	/* Insert system clock initialization code here (sysclk_init()). */
	sysclk_init();
	board_init();
	
#ifdef _DEBUG_
	/* Initialize the console UART. */
	configure_console();
#endif

	puts(STRING_HEADER);
	
	fill_ArtNode(&ArtNode);
	fill_ArtPollReply(&ArtPollReply, &ArtNode);	

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
				gmac_process_eth_packet((uint8_t *) gs_uc_eth_buffer_rx, ul_frm_size_rx);
				//handleGMAC_Packet((uint8_t *) gs_uc_eth_buffer_rx, ul_frm_size_rx);
				artnetToCommand();
			}//end of process
		}
	}//end of loop
}//end of program

/*
	- Check if UDP -
	- Check if ArtNet -
	- Check ArtNet packetType -
	- handle packetType
*/
void handleGMAC_Packet(uint8_t *p_uc_data, uint32_t ul_size){
	p_ethernet_header_t p_eth = (p_ethernet_header_t) p_uc_data;
	uint8_t packetBuffer[GMAC_FRAME_LENTGH_MAX];
	uint16_t eth_pkt_format = SWAP16(p_eth->et_protlen);
	uint32_t hdr_len = ETH_HEADER_SIZE + ETH_IP_HEADER_SIZE + UDP_HEADER_SIZE;
	
	if(eth_pkt_format == ETH_PROT_IPV4){
		p_ip_header_t p_ip = (p_ip_header_t) (p_uc_data+ ETH_HEADER_SIZE);
		if (p_ip->ip_p == IP_PROT_UDP){
			/*Check on added Art-Net header*/
			printf("M: UDP\r\n");
			if (ul_size > hdr_len){
				PacketType = (T_ArtPacketType) get_packet_type(p_uc_data + ETH_HEADER_SIZE + ETH_IP_HEADER_SIZE + ICMP_HEADER_SIZE); 
				if(PacketType == FAULTY_PACKET){  // bad packet
					return;
				}	
				printf("M: Art-Net compatible\r\n");
				*packetBuffer = *(p_uc_data + ETH_HEADER_SIZE + ETH_IP_HEADER_SIZE + ICMP_HEADER_SIZE);
				if(PacketType == ARTNET_DMX){
					if(sizeof(packetBuffer) < sizeof(T_ArtDmx)){ 
						return;
					}
					else{
						printf("M: DMX\r\n");
						handle_dmx((T_ArtDmx *)&packetBuffer);
					}
				}
				else if(PacketType == ARTNET_POLL){
					if(sizeof(packetBuffer) < sizeof(T_ArtPoll)){ 
						return;
					}
					else{
						printf("M: ArtPoll\r\n");
						handle_poll((T_ArtPoll *)&packetBuffer, p_uc_data);
					}
				} 
/*				else if(packet_type == ARTNET_IPPROG)
				{
					if(sizeof(packetBuffer) < sizeof(artnet_ipprog_t))
						return;
					else
						handle_ipprog((artnet_ipprog_t *)&packetBuffer);
				} */
				else if(PacketType == ARTNET_ADDRESS){
					if(sizeof(packetBuffer) < sizeof(T_ArtAddress)){
						return;
					}
					else{
						handle_address((T_ArtAddress *)&packetBuffer, p_uc_data);
					}
				}
			}
		}
	}
	else{ 
		return;	
	}
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

	node->etsamanH = "S";        // The ESTA manufacturer code.
	node->etsamanL = "R";        // The ESTA manufacturer code.
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

void handle_dmx(T_ArtDmx *packet)
{
	if(packet->SubUni == ArtNode.swout[0])
	{
	  memcpy ((uint8_t *) artnet_data_buffer, (uint8_t *)packet->Data, MaxDataLength);
	  printf("M: DMX saved\r\n");
	}
}

void handle_poll(T_ArtPoll *packet, uint8_t *p_uc_data)
{
	if((packet->Flags & 8) == 1) // controller say: send unicast reply
	{
		send_reply(UNICAST, p_uc_data, (uint8_t *)&ArtPollReply);
		printf("M: ArtPollReply Unicast\r\n");
	}
	else // controller say: send broadcast reply
	{
		send_reply(BROADCAST, p_uc_data, (uint8_t *)&ArtPollReply);
		printf("M: ArtPollReply Broadcast\r\n");
	}
}

void handle_address(T_ArtAddress *packet, uint8_t *p_uc_data) //Not properly implemented yet
{
	send_reply(UNICAST, p_uc_data, (uint8_t *)&ArtPollReply);
	printf("M: ArtPollReply unicast\r\n");
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