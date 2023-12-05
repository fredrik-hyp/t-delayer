#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include "states.h"
#include "buffer.h"
#include "print_struct.h"

// =====  Variables shared between files  =====
// buffer.c
uint8_t packetcountersinlist[MAX_PACKET_COUNTER+1];

// states.h
volatile PROCESS_STATE pstate = STATE_STARTING;

void check_order_asc(bufferelement *buffer)
{
	if (buffer == NULL)
	{
		fprintf(stderr, "empty buffer\n");
		return;
	}

	bufferelement *current_item = buffer;
	int counter = current_item->header.counter;

	while(current_item)
	{
		if (counter+1 != current_item->header.counter)
		{
			fprintf(stderr, "Last counter %d current counter %d\n", counter, current_item->header.counter);
		}
		counter = current_item->header.counter;
		current_item = current_item->next;
	}
}

void check_order_deasc(bufferelement *buffer)
{
	if (buffer == NULL || buffer->last == NULL)
	{
		fprintf(stderr, "empty buffer\n");
		return;
	}

	bufferelement *current_item = buffer->last;
	int counter = current_item->header.counter;

	while(current_item)
	{
		if (counter-1 != current_item->header.counter)
		{
			fprintf(stderr, "current counter %d last counter %d\n", current_item->header.counter, counter);
		}
		counter = current_item->header.counter;
		current_item = current_item->prev;
	}
}

/*
int main(int argc, char **argv)
{
	bufferelement *current_bufelement=NULL;
	current_bufelement = getAvailableBufferElement(&availablebufferelements);

	addHeaderAndPutInBuffer(&firstsendbuffer, current_bufelement, RUDP_PACK_DATA,
			0, recv_bytes, 0,  &current_packet_counter);

	current_buffer_element = findByCounter(&mainbuffer, header.counter);

	// move this buffer last in resentbuffer...
	move_first_element_last(&resentbuffer);
	free_time_out_buffer(&mainbuffer);
	move_first_element_to_other_buffer(&mainbuffer, &resentbuffer);
	move_first_element_to_other_buffer(&firstsendbuffer, &mainbuffer);
}
*/
int main(int argc, char **argv)
{


// Testordning
// Skapa alla buffrar
// Frigör alla buffrar
// Skapa alla buffrar
// Flytta första elementet sist
// Elementet till en annan buffert
	// först -> sist
	// först -> Först
	// först -> I mitten 
	// I mitten -> sist
	// I mitten -> Först
	// I Mitten -> Sist
	// Sist -> sist
	// Sist -> först
	// sist -> i mitten
// Hitta element först, mitten sist ta bort
// Returnera buffert
// ....
	// === PROGRAM START ===
	// Statistic variables
	bufferelement *availablebufferelements;
	bufferelement *firstsendbuffer;
	bufferelement *mainbuffer;
	bufferelement *resentbuffer;

	struct timeval tv1, tv2, tv3, tv4, tv5;
	firstsendbuffer = NULL;
	mainbuffer = NULL;
	resentbuffer = NULL;

	// Setup buffer
	gettimeofday(&tv1, NULL);
	initBufferElements(&availablebufferelements);
	freeBufferElements(&availablebufferelements);
	gettimeofday(&tv2, NULL);
	initBufferElements(&availablebufferelements);

	bufferelement *current_bufelement=NULL;
	int current_packet_counter = 0;
	int i;
	int recv_bytes = PKTSIZE+RUDP_HEADER_SIZE+RTP_MIN_HEADER_SIZE;

	for (i = 0; i < BUFFER_SIZE-1; i++)
	{
		current_bufelement = getAvailableBufferElement(&availablebufferelements);
		memset(current_bufelement->packet_buffer, i % 127, recv_bytes);
		addHeaderAndPutInBuffer(&firstsendbuffer, current_bufelement, RUDP_PACK_DATA,
			0, recv_bytes, 0,  &current_packet_counter);
	}

fprintf(stderr, "\n\nascending order\n");
	check_order_asc(firstsendbuffer);
fprintf(stderr, "\n\nreverse order\n");
	check_order_deasc(firstsendbuffer);


	for (i = 0; i < 5; i++)
	{
		move_first_element_to_other_buffer(&firstsendbuffer, &mainbuffer);
	}
	check_order_asc(mainbuffer);
	check_order_deasc(mainbuffer);

	// Find one in the middle
	fprintf(stderr, "\nfindByCounter - in the middle - counter == 2\n");
	current_bufelement = findByCounter(&mainbuffer, 2);
	if (current_bufelement == NULL)
	{
		fprintf(stderr, "findByCounter (2) returned NULL\n");
	}
	else
	{
		printCounters(mainbuffer);
		fprintf(stderr, "findByCounter: %d\n\n", current_bufelement->header.counter);
		returnBufferElement(&availablebufferelements, current_bufelement, 1);
	}
	
	// Find the last one - counter == 4
	fprintf(stderr, "\nfindByCounter(4) - the last one\n");
	current_bufelement = findByCounter(&mainbuffer, 4);
	if (current_bufelement == NULL)
	{
		fprintf(stderr, "findByCounter (4) returned NULL\n");
	}
	else
	{
		printCounters(mainbuffer);
		fprintf(stderr, "findByCounter: %d\n\n", current_bufelement->header.counter);
		returnBufferElement(&availablebufferelements, current_bufelement, 1);
	}

	// Find the first one - counter == 0
	fprintf(stderr, "\nfindByCounter(0) - the first one\n");
	current_bufelement = findByCounter(&mainbuffer, 0);
	if (current_bufelement == NULL)
	{
		fprintf(stderr, "findByCounter (0) returned NULL\n");
	}
	else
	{
		printCounters(mainbuffer);
		fprintf(stderr, "findByCounter: %d\n\n", current_bufelement->header.counter);
		returnBufferElement(&availablebufferelements, current_bufelement, 1);
	}

	
	gettimeofday(&tv3, NULL);

#if 0
	fprintf(stderr, "firstsendbuffer\n");
	printCounters(firstsendbuffer);
#endif
	fprintf(stderr, "mainbuffer\n");
	printCounters(mainbuffer);

	freeBufferElements(&availablebufferelements);
	freeBufferElements(&firstsendbuffer);
	freeBufferElements(&mainbuffer);
	freeBufferElements(&resentbuffer);
	gettimeofday(&tv4, NULL);

	fprintf(stderr, "%lu:%lu\n", tv1.tv_sec, tv1.tv_usec);
	fprintf(stderr, "%lu:%lu\n", tv2.tv_sec, tv2.tv_usec);
	fprintf(stderr, "%lu:%lu\n", tv3.tv_sec, tv3.tv_usec);
	fprintf(stderr, "%lu:%lu\n", tv4.tv_sec, tv4.tv_usec);
	fprintf(stderr, "%lu:%lu\n", tv5.tv_sec, tv5.tv_usec);
	fprintf(stderr, "%d\n", current_packet_counter);

	return 0;
}
