/*
 * GMAC_Artnet.h
 *
 * Created: 12/09/2020 19:47:28
 *  Author: Design
 */ 


#ifndef GMAC_ARTNET_H_
#define GMAC_ARTNET_H_

#include <asf.h>
#include <string.h>
#include "mini_ip.h"
#include "conf_eth.h"

extern uint8_t gs_uc_mac_address[];
extern uint32_t ul_frm_size_rx, ul_frm_size_tx;
extern gmac_device_t gs_gmac_dev;
extern volatile uint8_t gs_uc_eth_buffer_rx[GMAC_FRAME_LENTGH_MAX];
extern volatile uint8_t gs_uc_eth_buffer_tx[GMAC_FRAME_LENTGH_MAX];
extern uint8_t artnet_data_buffer[512];
extern uint8_t gs_uc_ip_address[];

uint32_t read_dev_gmac(void);
uint32_t write_dev_gmac(void);
bool init_gmac_ethernet(void);
void at24mac_get_mac_address(void);
void gmac_display_ip_packet(p_ip_header_t p_ip_header, uint32_t ul_size);
void display_artnet_packet(uint8_t *p_uc_data, uint32_t ul_size);
void gmac_process_eth_packet(uint8_t *p_uc_data, uint32_t ul_size);
void display_eth_buffer(uint8_t *eth_buffer, uint32_t ul_size);
char compareArray(uint8_t a[],uint8_t b[],uint8_t size);

#endif /* GMAC_ARTNET_H_ */