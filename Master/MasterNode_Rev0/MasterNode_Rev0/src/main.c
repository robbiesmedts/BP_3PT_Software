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
#include "nRF24.h"
#include "nRF24L01.h"


#ifdef _DEBUG
#define STRING_EOL    "\r"
#define STRING_HEADER "-- MasterNode_Rev0 --\r\n" \
		"-- "BOARD_NAME" --\r\n" \
		"-- Compiled: "__DATE__" "__TIME__" --"STRING_EOL

/************************************************************************/
/* Globele variabelen                                                                     */
/************************************************************************/

/* Datapaket standaard.
   datapaketten verzonden binnen dit project zullen dit formaat hanteren om een uniform systeem te vormen
   destAddr     //adres (6x8bits) ontvangen met pakket, zal volgens commando een ontvangend adres worden of een adres waarnaar gezonden word
   dataValue    //variabele (8bits) om binnenkomende/uitgaande data in op te slagen
   command      //commando (8bits) gestuctureerd volgens command table

   command table
   0 = Stop command
   1 = use sensor for own actuator
   2 = send sensor value to other actuator
   3 = receive sensor value for own actuator
*/
struct dataStruct{
	uint32_t destAddr;
	uint16_t datavalue;
	uint8_t command;
}dataIn, dataOut;

static const uint32_t listeningPipes[5] = {0x3A3A3AD2UL, 0x3A3A3AC3UL, 0x3A3A3AB4UL, 0x3A3A3AA5UL, 0x3A3A3A96UL}; //unieke adressen gebruikt door de nodes.
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


int main (void)
{
	/* Insert system clock initialization code here (sysclk_init()). */
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
	nRF24_begin();
	nRF24_setPALevel(RF_PA_MIN);
	nRF24_stopListening();
	
	/*Initialize ETH module and set link*/
	if (!init_gmac_ethernet())
	{
		return -1;
	}	

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
/************************************************************************/
/* Start Infinite loop                                                  */
/************************************************************************/	
	while (1)
	{
		// Process packets
		if (GMAC_OK != read_dev_gmac()) {
			continue; // return to while(1){}
		}

		if (ul_frm_size > 0) {
			// Handle input frame
			gmac_process_eth_packet((uint8_t *) gs_uc_eth_buffer, ul_frm_size);

			}
	}//end loop
}//end main
