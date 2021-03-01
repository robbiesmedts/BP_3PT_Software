/**
 * \file
 *
 * \brief Empty user application template
 *
 */

#define _DEBUG_

#include <asf.h>
#include "softLib/GMAC_Artnet.h"
#include "softLib/nRF24.h"
#include "softLib/nRF24L01.h"
#include "softLib/SAM_SPI.h"


/// @cond 0
/**INDENT-OFF**/
#ifdef __cplusplus
extern "C" {
#endif
/**INDENT-ON**/
/// @endcond

#ifdef _DEBUG_
#define STRING_EOL    "\r"
#define STRING_HEADER "-- MasterNode_Rev0-1 --\r\n" \
"-- "BOARD_NAME" --\r\n" \
"-- Compiled: "__DATE__" "__TIME__" --"STRING_EOL
#endif

/************************************************************************/
/* Function prototypes                                                  */
/************************************************************************/

long map(long x, long in_min, long in_max, long out_min, long out_max);

/************************************************************************/
/* Global variables                                                     */
/************************************************************************/

/* Data standard.
   data communicated in this project follows the next structure to form an uniform protocol.
   destAddr     //address (32bits) given by the command this is an receiving address or an destination address
   dataValue    //variable (16bits) to save incoming and outgoing data
   command      //commando (8bits) structured by command table

   command table
   0 = Stop command
   1 = use sensor for own actuator
   2 = send sensor value to other actuator
   3 = receive sensor value for own actuator
   4 = reset node
*/
struct dataStruct{
	uint32_t destAddr;
	uint16_t datavalue;
	uint8_t command;
}dataIn, dataOut;

static const uint32_t listeningPipes[6] = {0x3A3A3AA1UL, 0x3A3A3AB1UL, 0x3A3A3AC1UL, 0x3A3A3AD1UL, 0x3A3A3AE1UL, 0x3A3A3A0A}; //unieke adressen gebruikt door de nodes.
static uint16_t artnetDmxAddress = 1;
static const uint8_t nodes = 5; //number of sensor nodes

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
/**
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
	for(uint16_t i = artnetDmxAddress-1; i < (artnetDmxAddress + nodes); i++)
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
			else if(dmxValue >= 21 && dmxValue <= 30)
			{
				/*Sensor to Node n*/
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
				/*Sensor to Node n+1*/
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
				/*Sensor to Node n+2*/
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
			}
/*			else if(61 < dmxValue < 80)
			{
				//voor extra nodes
			}*/
/*			else if (dmxValue >= 81 && dmxValue <= 90)
			{
				//send nothing to the node to avoid command collisions with the sensor data
			}*/
			else if (dmxValue >= 91 && dmxValue <= 100)
			{
				//send sensorData to Midi node for feedback to audio, light or video
				nRF24_openWritingPipe(listeningPipes[currentNode]);
				dataOut.command = 2;
				dataOut.destAddr = listeningPipes[6];
				nRF24_write(&dataOut, sizeof(dataOut));
#ifdef _DEBUG_
				printf("Send data from node %d to Midi node\r\n", currentNode);
#endif				
			}
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
			else if(dmxValue >= 231 && dmxValue <= 255)
			{
				//reset Node
				nRF24_openWritingPipe(listeningPipes[currentNode]);
				dataOut.command = 4;
				nRF24_write(&dataOut, sizeof(dataOut));
#ifdef _DEBUG_
				printf("Reset node %d\r\n", currentNode);
#endif
			}//end if structure
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
	nRF24_setPALevel(RF_PA_MIN);
	nRF24_stopListening();
	
#ifdef _DEBUG_
	printDetails();
#endif	
	
	while(1)
	{
		// Process packets
		if (GMAC_OK != read_dev_gmac()) {
			continue; // return to while(1){}
		}

		if (ul_frm_size_rx > 0) {
			// Handle input frame
			gmac_process_eth_packet((uint8_t *) gs_uc_eth_buffer_rx, ul_frm_size_rx);
			artnetToCommand();
		}//end of process
	}//end of loop
}//end of program

/// @cond 0
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/// @endcond