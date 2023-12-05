#include <sys/time.h>
#include "common.h"
#include "prot_rudp.h"

// packet ts-data must start at offset HEADER_SIZE
uint64_t addHeader(uint8_t *packet_buffer, uint8_t typeofpacket, uint32_t sessionid, uint16_t payloadlength, uint8_t channelnumber, uint32_t counter)
{
	struct timeval tv;
	uint64_t sendtime;
	
	packet_buffer[0] = typeofpacket << 4;
	packet_buffer[1] = (sessionid & 0xff000000) >> 24;
	packet_buffer[2] = (sessionid & 0x00ff0000) >> 16;
	packet_buffer[3] = (sessionid & 0x0000ff00) >> 8;
	packet_buffer[4] = sessionid & 0x000000ff;
	packet_buffer[5] = (payloadlength & 0xff00) >> 8;
	packet_buffer[6] = payloadlength & 0x00ff;
	packet_buffer[7] = channelnumber;
	packet_buffer[8] = (counter & 0x00ff0000ULL) >> 16;
	packet_buffer[9] = (counter & 0x0000ff00ULL) >> 8;
	packet_buffer[10] = (counter & 0x000000ffULL);
	
	memset(&tv,0,sizeof(struct timeval));
	gettimeofday(&tv, NULL);

	sendtime = tv.tv_sec*1000000ULL + tv.tv_usec;

	packet_buffer[11] = (sendtime & 0xff00000000000000ULL) >> 56;
	packet_buffer[12] = (sendtime & 0x00ff000000000000ULL) >> 48;
	packet_buffer[13] = (sendtime & 0x0000ff0000000000ULL) >> 40;
	packet_buffer[14] = (sendtime & 0x000000ff00000000ULL) >> 32;
	packet_buffer[15] = (sendtime & 0x00000000ff000000ULL) >> 24;
	packet_buffer[16] = (sendtime & 0x0000000000ff0000ULL) >> 16;
	packet_buffer[17] = (sendtime & 0x000000000000ff00ULL) >> 8;
	packet_buffer[18] = (sendtime & 0x00000000000000ffULL);
	return sendtime;
}

void parseHeader(uint8_t *packet_buffer, rudpheader *header)
{
	header->typeofpacket = (uint8_t)packet_buffer[0] >> 4;
	header->reserved = (uint8_t)packet_buffer[0];
	header->sessionid = (uint32_t)packet_buffer[1] << 24;
	header->sessionid += (uint32_t)packet_buffer[2] << 16;
	header->sessionid += (uint32_t)packet_buffer[3] << 8;
	header->sessionid += (uint32_t)packet_buffer[4];
	header->payloadlength = (uint16_t)packet_buffer[5] << 8;
	header->payloadlength += (uint16_t)packet_buffer[6];
	header->channelnumber = (uint8_t)packet_buffer[7];
	header->counter = packet_buffer[8] << 16;
	header->counter += packet_buffer[9] << 8;
	header->counter += packet_buffer[10];
	
	header->sendtime =  ((uint64_t)packet_buffer[11]) << 56;
	header->sendtime += ((uint64_t)packet_buffer[12]) << 48 ;
	header->sendtime += ((uint64_t)packet_buffer[13]) << 40;
	header->sendtime += ((uint64_t)packet_buffer[14]) << 32;
	header->sendtime += ((uint64_t)packet_buffer[15]) << 24;
	header->sendtime += ((uint64_t)packet_buffer[16]) << 16;
	header->sendtime += ((uint64_t)packet_buffer[17]) << 8;
	header->sendtime += ((uint64_t)packet_buffer[18]) ;
}

#ifdef NODAEMON_DEBUG
void printpacketheader(uint8_t *packet)
{
	int i;

	for (i=0; i < RUDP_HEADER_SIZE; i++)
	{
		fprintf(stderr, "%d ", packet[i]);
	}
	
	fprintf(stderr, "\n");
}
#endif

uint32_t genrudpsessionid()
{
	struct timeval tv;
	unsigned int seed;
	uint32_t sessid =0 ;
	gettimeofday(&tv, NULL);
	seed = tv.tv_usec << 8;
	srandom(seed * 67);
	
	sessid = random();
	sessid += (random() <<16);
	
	return sessid;
}
