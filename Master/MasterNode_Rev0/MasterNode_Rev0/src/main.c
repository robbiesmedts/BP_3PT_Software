/**
 * \file
 *
 * \brief Empty user application template
 *
 */


/* Commend if debugging by console isn't necessary */
#define _DEBUG

/*
	included libraries
*/
#include <asf.h>
#include "GMAC_Artnet.h"
#include "SAM_SPI.h"
#include "nRF24.h"
#include "nRF24L01.h"

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

static const uint32_t listeningPipes[5] = {0x3A3A3AD2UL, 0x3A3A3AC3UL, 0x3A3A3AB4UL, 0x3A3A3AA5UL, 0x3A3A3A96UL}; //unieke adressen gebruikt door de nodes.
static uint16_t artnetDmxAddress = 1;
static const uint8_t nodes = 4;

#ifdef _DEBUG
#define STRING_EOL    "\r"
#define STRING_HEADER "-- MasterNode_Rev0 --\r\n" \
		"-- "BOARD_NAME" --\r\n" \
		"-- Compiled: "__DATE__" "__TIME__" --"STRING_EOL

/**
 *  \brief Configure UART console.
 */
static void configure_console(void)
{
	const usart_serial_options_t uart_serial_options = {
		.baudrate = CONF_UART_BAUDRATE,
        .charlength = CONF_UART_CHAR_LENGTH,
        .paritytype = CONF_UART_PARITY,
        .stopbits = CONF_UART_STOP_BITS,
	};
	
	/* Configure console UART. */
	sysclk_enable_peripheral_clock(CONSOLE_UART_ID);
	stdio_serial_init(CONF_UART, &uart_serial_options);
}
#endif
	
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
 *		21-30 	| Sensor to node 0 (dependable of source Node)
 *		31-40 	| Sensor to node 1 (dependable of source Node)
 *		41-50 	| Sensor to node 2 (dependable of source Node)
 *		61-100	| (future nodes)
 *		101-230	| (future functionality)
 *		231-240	| Lighting desk controls actuator -> extra functionality
 *		241-250	| sensor feedback to lighting desk
 *		251-255 | reset node (reset any saved sensor values and resets the actuator)
 *	
 * Extra functionality
 *	if (101 < general < 110)
 *		0 - 255	| Value for actuator
 *	else
 *		0-255	| not active
 *	
*/
static void artnetToCommand(void)
{
	uint8_t dmxValue = 0;
	uint8_t extraValue = 0;
	uint8_t currentNode = 0;
	bool extraFunction = false;

	//array start at 0 so the dmx address must be lowered by 1
	for(uint8_t i = artnetDmxAddress-1; i < artnetDmxAddress + (nodes * 2); i++)
	{
		dmxValue = artnet_data_buffer[i];
		if (!extraFunction)
		{
/************************************************************************/
/* Node disabled                                                        */
/************************************************************************/
			if(dmxValue <= 10)
			{
				if (currentNode != 0)
				{
					nRF24_openWritingPipe(listeningPipes[currentNode]);
					dataOut.command = 0;
					nRF24_write(&dataOut, sizeof(dataOut));
				}
				else
				{
					//disable masterNode
				}
			}
/************************************************************************/
/* Read own sensor                                                      */
/************************************************************************/
			else if(dmxValue >= 11 && dmxValue <= 20)
			{
				
				if (currentNode != 0)
				{
					nRF24_openWritingPipe(listeningPipes[currentNode]);
					dataOut.command = 1;
					nRF24_write(&dataOut, sizeof(dataOut));
				}
				else
				{
					//read own sensor
				}
			}
/************************************************************************/
/* Send sensor to other node                                            */
/************************************************************************/
			else if(dmxValue >= 21 && dmxValue <= 30)
			{
				/*Sensor to Node n*/
				if (currentNode == 0)
				{	//other node	
					nRF24_openWritingPipe(listeningPipes[1]);
					dataOut.command = 3;
					dataOut.datavalue  = 0xAAAA;
					nRF24_write(&dataOut, sizeof(dataOut));
				}
				else
				{
					nRF24_openWritingPipe(listeningPipes[currentNode]);
					dataOut.command = 2;
					dataOut.destAddr = listeningPipes[0];
					nRF24_write(&dataOut, sizeof(dataOut));
				}

			}
			else if(dmxValue >= 31 && dmxValue <= 40)
			{
				/*Sensor to Node n+1*/
				if (currentNode == 0)
				{	
					nRF24_openWritingPipe(listeningPipes[2]);
					dataOut.command = 3;
					dataOut.datavalue  = 0xAAAA;
					nRF24_write(&dataOut, sizeof(dataOut));
				}
				else if(currentNode == 1)
				{
					nRF24_openWritingPipe(listeningPipes[currentNode]);
					dataOut.command = 2;
					dataOut.destAddr = listeningPipes[2];
					nRF24_write(&dataOut, sizeof(dataOut));	
				}
				else
				{
					nRF24_openWritingPipe(listeningPipes[currentNode]);
					dataOut.command = 2;
					dataOut.destAddr = listeningPipes[1];
					nRF24_write(&dataOut, sizeof(dataOut));
				}
			}
			else if(dmxValue >= 41 && dmxValue <= 50)
			{
				/*Sensor to Node n+2*/
				if (currentNode == 0)
				{	
					nRF24_openWritingPipe(listeningPipes[3]);
					dataOut.command = 3;
					dataOut.datavalue = 0xAAAA;
					nRF24_write(&dataOut, sizeof(dataOut));
				}
				else if(currentNode == 3)
				{
					//read own sensor
					nRF24_openWritingPipe(listeningPipes[currentNode]);
					dataOut.command = 2;
					dataOut.destAddr = listeningPipes[2];
					nRF24_write(&dataOut, sizeof(dataOut));
				}
				else
				{
					nRF24_openWritingPipe(listeningPipes[currentNode]);
					dataOut.command = 2;
					dataOut.destAddr = listeningPipes[3];
					nRF24_write(&dataOut, sizeof(dataOut));
				}
			}
/*			else if(51 < dmxValue < 230)
			{
				//voor extra nodes of extra functionaliteit
			}*/
/************************************************************************/
/* Use DMX value                                                        */
/************************************************************************/
			else if(dmxValue >= 231 && dmxValue <= 240)
			{
				//desk -> actuator
				if (currentNode != 0)
				{
					nRF24_openWritingPipe(listeningPipes[currentNode]);
					dataOut.command = 3;
				}
				else
				{
					//write own actuator
				}			
				extraFunction = true;
				extraValue = dmxValue;				
			}
/*			else if(dmxValue >= 241 && dmxValue <= 250)
			{
				//Sensor -> desk
				//ongeïmplementeerd
			}*/
/************************************************************************/
/* Global variables                                                     */
/************************************************************************/
			else if(dmxValue >= 251 && dmxValue <= 255)
			{
				//reset Node
			}
			else
			{
				//no functionality
			}
		
			//skip the extra functionality
			if (!extraFunction)
			{
				i++;
				currentNode++;
			}
		}
		else //extra functionaliteit
		{
			//read data
			if(extraValue >= 231 && extraValue <= 240)
			{
				dataOut.datavalue = dmxValue;
				nRF24_write(&dataOut, sizeof(dataOut));
			}
			
			currentNode++;
			extraFunction = false;
		}
	}
	
}


int main (void)
{
	/************************************************************************/
	/* Initialization of the masterNode                                     */
	/************************************************************************/
	sysclk_init();
	board_init();
	
#ifdef _DEBUG
	/* Initialize the console UART. */
	configure_console();

	puts(STRING_HEADER);
#endif	
	/*Initialize SPI module*/
	spi_master_initialize();
	
	/*Initialize nRF24 module as TX*/
//	nRF24_begin();
//	nRF24_setPALevel(RF_PA_MIN);
//	nRF24_stopListening();
	
#ifdef _DEBUG
	
	// Display MAC & IP settings
	printf("-- MAC %x:%x:%x:%x:%x:%x\n\r",
	gs_uc_mac_address[0], gs_uc_mac_address[1], gs_uc_mac_address[2],
	gs_uc_mac_address[3], gs_uc_mac_address[4], gs_uc_mac_address[5]);

	printf("-- IP  %d.%d.%d.%d\n\r", gs_uc_ip_address[0], gs_uc_ip_address[1],
	gs_uc_ip_address[2], gs_uc_ip_address[3]);
	
	//display nRF settings
	printDetails();
#endif
	
	/*Initialize ETH module and set link*/
	if (!init_gmac_ethernet())
	{
		return -1;
	}
	puts("link detected\r");

/************************************************************************/
/* Start Infinite loop                                                  */
/************************************************************************/	
	while (1) {
		
		// Process packets
		if (GMAC_OK != read_dev_gmac()) {
			continue; // return to while(1){}
		}

		if (ul_frm_size > 0) {
			// Handle input frame
			//gmac_process_eth_packet((uint8_t *) gs_uc_eth_buffer, ul_frm_size);
			proces_artnet_packet((uint8_t *) gs_uc_eth_buffer, ul_frm_size);
			//artnetToCommand();
		}
	}//end loop
}//end main
