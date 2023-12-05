#ifndef __BUFFER_H__
#define __BUFFER_H__

#include "prot_rudp.h"
#include <stdlib.h>
#include <pthread.h>

// 10 sec = 10 000 000 usec
#define CHUNK_TIME_DIFF 10000000

#define MAX_PACKET_COUNTER	54292

#ifdef RUDP_FILE
#define MAX_FILEINFO_PACKETS	10
#define MAX_FILEINFO_COUNTER	(MAX_PACKET_COUNTER + MAX_FILEINFO_PACKETS)
#define MAX_BUFFER_SIZE		(MAX_PACKET_COUNTER + MAX_FILEINFO_PACKETS + 1)
#else
#define MAX_BUFFER_SIZE		(MAX_PACKET_COUNTER + 1)
#endif

// Assume 0.5s is max between 2 frames
#define MAX_NTP_CHANGE		0

#ifdef DELAYERD
// Delayer has several different Q for each function and dont need to be so big
#define BUFFER_SIZE 2500
#else
// number of packet in output buffer
#define BUFFER_SIZE 50000
#endif


#define PACKET_BUFFER_SIZE (PKTSIZE+RUDP_HEADER_SIZE+RTP_MIN_HEADER_SIZE)

typedef struct _bufferelement
{
	rudpheader header;
	uint64_t rudp_sendtime;
	uint8_t rtp;
	uint8_t packet_buffer[PACKET_BUFFER_SIZE];
	struct _bufferelement *prev;
	struct _bufferelement *next; 
	struct _bufferelement *last;
} bufferelement;

//  (1+4+2+1+3+8) + (8 + 1 + 1*(1316 + 12 + 19) + 24)

int countList(bufferelement *buffer);
void printCounters(bufferelement *buffer);
void initBufferElements(bufferelement **elem);
void freeBufferElements(bufferelement **buf);

bufferelement *getAvailableBufferElement(bufferelement **availbuf);
void returnBufferElement(bufferelement **availbuf, bufferelement *element, int clearout);
bufferelement *getTimedoutPackets(bufferelement **buf, uint64_t ms_timeout);

// From transmitter
int free_time_out_buffer(bufferelement **buffer_to_clean, bufferelement **buffer_to_return_to, uint64_t msbuffer);

int putInBuffer(bufferelement **buf, bufferelement *newelement);
void move_first_element_to_other_buffer(bufferelement **frombuffer, bufferelement **tobuffer);
void move_first_element_last(bufferelement **buffer);
bufferelement *findByCounter(bufferelement **buf, uint32_t counter);
void addHeaderAndPutInBuffer(bufferelement **buf, bufferelement *element, uint8_t typeofpacket,
uint32_t sessionid, uint16_t payloadlength, uint8_t channelnumber, uint32_t *counter);

// From receiver
int receiverPutInBuffer(bufferelement **buf, bufferelement *newelement, int buffer_timeout);
bufferelement *check_and_get_resend_buffer(bufferelement **src, uint32_t last_counter, uint64_t last_time_written_to_disc);
bufferelement *chop_next_counter_head_of_buffer(bufferelement **src, uint32_t last_counter);
bufferelement *chop_head_of_buffer(bufferelement **src);

#endif
