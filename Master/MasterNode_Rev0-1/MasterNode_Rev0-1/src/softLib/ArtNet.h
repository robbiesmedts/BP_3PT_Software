// OpCodes
#define OpPoll 			0x2000
#define OpPollReply 	0x2100
#define OpDiagData		0x2300
#define OpCommand		0x2400
#define OpDmx			0x5000
#define OpNzs			0x5100
#define OpSync			0x5200
#define OpAddress		0x6000
#define OpInput			0x7000
#define OpTodRequest	0x8000
#define OpTodData		0x8100
#define OpTodControl	0x8200


typedef struct artnet_packet {
	uint8_t art_id[8];			//Art-Net identifier
	uint16_t art_OpCode;		//Art-Net packet type
	uint16_t art_protver;		//Art-Net protocol version
	uint8_t art_seq;			//sequence
	uint8_t art_phy;			//physical port used
	uint16_t art_uninet;		//destination universe
	uint16_t art_datlen;		//length of data array
	uint8_t art_data[512];		//data array
	} art_packet_t, *p_art_packet_t;
	
typedef struct artPoll_packet {
	uint8_t art_id[8];			//Art-Net identifier
	uint16_t art_OpCode;		//Art-Net packet type
	uint8_t art_protverHi;		//Art-Net protocol version High byte
	uint8_t art_protverLo;		//Art-Net protocol version Low byte
	uint8_t art_TalkToMe;		//Behavior of node
	uint8_t art_Priority;		//Priority of the message
	} artPoll_t, *p_artPoll_t;
	
