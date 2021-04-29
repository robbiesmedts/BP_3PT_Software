/*
 * main.h
 *
 * Created: 30/03/2021 20:36:19
 *  Author: Design
 */ 


#ifndef MAIN_H_
#define MAIN_H_

#define _DEBUG_

#include "softLib/GMAC_Artnet.h"
#include "softLib/ArtNet/Art-Net.h"
#include "softLib/nRF24.h"
#include "softLib/nRF24L01.h"
#include "softLib/SAM_SPI.h"




#define STRING_EOL    "\r"
#define STRING_HEADER "-- MasterNode_Rev0-1 --\r\n" \
"-- "BOARD_NAME" --\r\n" \
"-- Compiled: "__DATE__" "__TIME__" --"STRING_EOL


/************************************************************************/
/* Definitions                                                          */
/************************************************************************/
#define UNICAST               0
#define BROADCAST             1

/************************************************************************/
/* Macros                                                               */
/************************************************************************/
#define SWAP16(x)   (((x & 0xff) << 8) | (x >> 8))
#define BYTES_TO_SHORT(h,l)	( ((h << 8) & 0xff00) | (l & 0x00FF) );

/************************************************************************/
/* Structures                                                           */
/************************************************************************/
typedef struct artnet_node_s {
	uint8_t  id           [8];
	uint16_t opCode;
	uint8_t  localIp      [4];
	uint16_t localPort;
	uint8_t  verH;
	uint8_t  ver;
	uint8_t  subH;
	uint8_t  sub;
	uint8_t  oemH;
	uint8_t  oem;
	uint8_t  ubea;
	uint8_t  status;
	uint8_t  etsamanH;
	uint8_t  etsamanL;
	uint8_t  shortname    [18];
	uint8_t  longname     [64];
	uint8_t  nodereport   [64];
	uint8_t  numbportsH;
	uint8_t  numbports;
	uint8_t  porttypes    [4];
	uint8_t  goodinput    [4];
	uint8_t  goodoutput   [4];
	uint8_t  swin         [4];
	uint8_t  swout        [4];
	uint8_t  swvideo;
	uint8_t  swmacro;
	uint8_t  swremote;
	uint8_t  style;
	uint8_t  remoteIp     [4];
	uint16_t remotePort;
	uint8_t  broadcastIp  [4];
	uint8_t  gateway      [4];
	uint8_t  subnetMask   [4];
	uint8_t  mac          [6];
	uint8_t  ProVerH;
	uint8_t  ProVer;
	uint8_t  ttm;
} T_ArtNode;

typedef enum artnet_packet_type_e {
	FAULTY_PACKET = 0x0000,
	ARTNET_POLL = 0x2000,
	ARTNET_REPLY = 0x2100,
	ARTNET_DMX = 0x5000,
	ARTNET_ADDRESS = 0x6000,
	ARTNET_INPUT = 0x7000,
	ARTNET_TODREQUEST = 0x8000,
	ARTNET_TODDATA = 0x8100,
	ARTNET_TODCONTROL = 0x8200,
	ARTNET_RDM = 0x8300,
	ARTNET_VIDEOSETUP = 0xa010,
	ARTNET_VIDEOPALETTE = 0xa020,
	ARTNET_VIDEODATA = 0xa040,
	ARTNET_MACMASTER = 0xf000,
	ARTNET_MACSLAVE = 0xf100,
	ARTNET_FIRMWAREMASTER = 0xf200,
	ARTNET_FIRMWAREREPLY = 0xf300,
	ARTNET_IPPROG = 0xf800,
	ARTNET_IPREPLY = 0xf900,
	ARTNET_MEDIA = 0x9000,
	ARTNET_MEDIAPATCH = 0x9200,
	ARTNET_MEDIACONTROLREPLY = 0x9300
} T_ArtPacketType;

/************************************************************************/
/* Function prototypes                                                  */
/************************************************************************/
long map(long x, long in_min, long in_max, long out_min, long out_max);
bool handleGMAC_Packet(uint8_t *p_uc_data, uint32_t ul_size);
T_ArtPacketType get_packet_type(uint8_t *packet);
void fill_ArtNode(T_ArtNode *node);
void fill_ArtPollReply(T_ArtPollReply *poll_reply, T_ArtNode *node);
void handle_address(p_T_ArtAddress *packet, uint8_t *p_uc_data);
void send_reply(uint8_t mode_broadcast, uint8_t *p_uc_data, uint8_t *packet);

/************************************************************************/
/* Global variables                                                     */
/************************************************************************/
typedef enum sensorCommand{
  disabled = 0,
  active_hue,
  active_sat,
  active_int, 
  receive_hue,
  receive_sat,
  receive_int,
  reset
}e_command;

/* Datapaket standaard.
   datapaketten verzonden binnen dit project zullen dit formaat hanteren om een uniform systeem te vormen
   srcNode    //Enumeration van de node waar de data van origineerd
   destNode   //Enumeration van de node waar deze data voor bestemd is.
   command      //commando (Enum) gestuctureerd volgens command table
   intensity    //intensity of the LEDs
   hue          //color of the LEDs transcoded in a hue
   saturation   //saturation of the colors
   sensorval    //sensor values to use in calculations sigend (8-bit, value tussen -128 tot 127)
*/
struct dataStruct {
  uint8_t srcNode;
  uint8_t destNode;
  e_command senCommand;
  uint8_t intensity;
  uint8_t hue;
  uint8_t saturation;
  int8_t sensorVal;
} dataIn, dataOut;

static const uint32_t listeningPipes[6] = {0x3A3A3AA1UL, 0x3A3A3AB1UL, 0x3A3A3AC1UL, 0x3A3A3AD1UL, 0x3A3A3AE1UL, 0x3A3A3A0A}; //unieke adressen gebruikt door de nodes.
static uint16_t artnetDmxAddress = 1;
static const uint8_t nodes = 4; //number of sensor nodes

uint8_t factory_mac [6] = {ETHERNET_CONF_ETHADDR0, ETHERNET_CONF_ETHADDR1, ETHERNET_CONF_ETHADDR2, ETHERNET_CONF_ETHADDR3, ETHERNET_CONF_ETHADDR4, ETHERNET_CONF_ETHADDR5};
uint8_t factory_localIp [4] = {ETHERNET_CONF_IPADDR0, ETHERNET_CONF_IPADDR1, ETHERNET_CONF_IPADDR2, ETHERNET_CONF_IPADDR3};
uint8_t factory_broadcastIp  [4] = {ETHERNET_CONF_IPADDR0, 255, 255, 255};           // broadcast IP address
uint8_t factory_gateway      [4] = {ETHERNET_CONF_GATEWAY_ADDR0, ETHERNET_CONF_GATEWAY_ADDR1, ETHERNET_CONF_GATEWAY_ADDR2, ETHERNET_CONF_GATEWAY_ADDR3};           // gateway IP address (use ip address of controller)
uint8_t factory_subnetMask   [4] = {ETHERNET_CONF_NET_MASK0, ETHERNET_CONF_NET_MASK1, ETHERNET_CONF_NET_MASK2, ETHERNET_CONF_NET_MASK3};           // network mask (art-net use 'A' network type)

uint8_t factory_swin         [4] = {   0,   1,   2,   3};
uint8_t factory_swout        [4] = {   0,   1,   2,   3};


T_ArtNode ArtNode;
T_ArtPollReply ArtPollReply;
T_ArtPacketType PacketType;

#endif /* MAIN_H_ */