#ifndef __PROTRUDP_H__
#define __PROTRUDP_H__
#include <stdint.h>

#define PKTSIZE 1316
#define RTP_MIN_HEADER_SIZE 12
#define RUDP_HEADER_SIZE 19
#define RUDP_HEARTBEAT_TIMEOUT		2000000
#define RUDP_HEARTBEAT_TIMEOUT_SEND	200000

// file transfer doesnt have RTP header - use the whole packet
#define FILETRANSFER_PKTSIZE 1328

#define RUDP_PACK_SETUP 0x0
#define RUDP_PACK_CLOSE 0x1
#define RUDP_PACK_HEARTBEAT 0x2
#define RUDP_PACK_DATA  0x3
#define RUDP_PACK_DATAACK 0x4
#define RUDP_PACK_DATA_FILEINFO	0x5

typedef struct _rudpheader{
	unsigned int typeofpacket : 4;
	unsigned int reserved : 4;
	uint32_t sessionid : 32; // 1
	uint16_t payloadlength : 16; // 5
	uint32_t channelnumber : 8; // 7
	uint32_t counter : 24; //8
	uint64_t sendtime; // 11
} rudpheader;

uint64_t addHeader(uint8_t *packet_buffer, uint8_t typeofpacket, uint32_t sessionid, uint16_t payloadlength, uint8_t channelnumber, uint32_t counter);

void parseHeader(uint8_t *packet_buffer, rudpheader *header);

#ifdef NODAEMON_DEBUG
void printpacketheader(uint8_t *packet);
#endif

uint32_t genrudpsessionid();
#endif
