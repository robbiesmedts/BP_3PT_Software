/**
 * \file
 *
 * \brief Empty user application template
 *
 */

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

#define STRING_EOL    "\r"
#define STRING_HEADER "-- MasterNode_Rev1 --\r\n" \
"-- "BOARD_NAME" --\r\n" \
"-- Compiled: "__DATE__" "__TIME__" --"STRING_EOL


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


int main (void)
{
	/* Insert system clock initialization code here (sysclk_init()). */
	sysclk_init();
	board_init();
	
	/* Initialize the console UART. */
	configure_console();

	puts(STRING_HEADER);
	
	if (!init_gmac_ethernet())
	{
		return -1;
	}
	
	// Display MAC & IP settings
	printf("-- MAC %x:%x:%x:%x:%x:%x\n\r",
	gs_uc_mac_address[0], gs_uc_mac_address[1], gs_uc_mac_address[2],
	gs_uc_mac_address[3], gs_uc_mac_address[4], gs_uc_mac_address[5]);

	printf("-- IP  %d.%d.%d.%d\n\r", gs_uc_ip_address[0], gs_uc_ip_address[1],
	gs_uc_ip_address[2], gs_uc_ip_address[3]);
	
	puts("link detected\r");

//	spi_master_initialize();
//	nRF24_begin();
//	nRF24_setPALevel(RF_PA_MIN);
//	nRF24_stopListening();
	
//	printDetails();	
	
	while(1)
	{
		// Process packets
		if (GMAC_OK != read_dev_gmac()) {
			continue; // return to while(1){}
		}

		if (ul_frm_size > 0) {
			// Handle input frame
			gmac_process_eth_packet((uint8_t *) gs_uc_eth_buffer, ul_frm_size);
			//artnetToCommand();
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