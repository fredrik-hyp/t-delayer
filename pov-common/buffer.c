#include <sys/time.h>
#include "buffer.h"
#include "common.h"
#include "states.h"
#include <assert.h>

extern uint8_t packetcountersinlist[MAX_BUFFER_SIZE];

int countList(bufferelement *buffer)
{
	int i = 0;
	for (i = 0; buffer != NULL; i++)
	{
		buffer = buffer->next;
	}
	return i;
}

#ifdef NODAEMON_DEBUG_BUFFER
void count_packets_and_check_if_counter_is_in_list(bufferelement *buffer, unsigned int counter, int *count, int *position)
{
	*position = -1;
	int i = 0;
	for (i = 0; buffer != NULL; i++)
	{
		if (buffer->header.counter == counter)
		{
			if (*position != -1)
			{
				fprintf(stderr,
						"counter dubblett! in pos %d and %d counter %u\n",
						*position, i, counter);
				exit(1);
			}
			*position = i;
		}
		buffer = buffer->next;
	}
	*count = i;
}
#endif

void printCounters(bufferelement *buffer)
{
#ifdef NODAEMON_DEBUG_BUFFER
	fprintf(stderr, "counters: ");
	while (buffer != NULL)
	{
		fprintf(stderr, "%d ", buffer->header.counter);
		buffer = buffer->next;
	}
	fprintf(stderr, "\n");
#endif
}

void initBufferElements(bufferelement **elem)
{
	int i = 0;
	bufferelement *elements = NULL;
	bufferelement *current = NULL;
	bufferelement *prev = NULL;
	
#ifdef NODAEMON_DEBUG_BUFFER
	fprintf(stderr, "bufferelemen: %lu\n",
		sizeof(bufferelement));
#endif
	for (i = 0; i < BUFFER_SIZE; i++)
	{
		current = calloc(1,sizeof(bufferelement));
		current->rtp = 0;
		if (i == 0)
		{
			current->prev = NULL;
			current->next = NULL;
			elements = current;
		}
		else
		{
			prev->next = current;
			current->prev = prev;
		}
		prev = current;
	}
	
	for (i = 0; i <= MAX_PACKET_COUNTER; i++)
	{
		packetcountersinlist[i] = 0;
	}
	
	elements->last = current;
	*elem = elements;
	assert(elements->prev == NULL);
}

void freeBufferElements(bufferelement **buf)
{
	bufferelement *next = *buf;
	bufferelement *curr;
	int freeCounter = 0;
	while (next)
	{
		curr = next;
		next = next->next;
		free(curr);
		curr = NULL;
		freeCounter++;
	}
	LOGMSG(LOG_WARNING, "freeBufferElements %d", freeCounter);
}

bufferelement *getAvailableBufferElement(bufferelement **availbuf)
{
	bufferelement *availbuffer = *availbuf;
	bufferelement *last;

	if (availbuffer == NULL)
	{
		return NULL;
	}
	
	if (availbuffer->last == NULL)
	{
		// set "basepointer" to NULL - no more buffers!
		*availbuf = NULL;
		// Return the last buffer
		return availbuffer;
	}
	last = availbuffer->last;

	if (availbuffer->last->prev != NULL)
	{
		availbuffer->last->prev->next = NULL;
		availbuffer->last = availbuffer->last->prev;
	}
	else if (availbuffer == availbuffer->last)
	{
		*availbuf = NULL;
	}
	return last;
}

void returnBufferElement(bufferelement **availbuf, bufferelement *element, int clearout)
{
	bufferelement *availbuffer = *availbuf;
	
	element->rudp_sendtime = 0;
	element->rtp = 0;
	if (availbuffer == NULL)
	{
		availbuffer = element;
		availbuffer->last = availbuffer;
		availbuffer->next = NULL;
		availbuffer->prev = NULL;
	}
	else if (availbuffer->last != NULL)
	{
		availbuffer->last->next = element;
		element->next = NULL;
		element->prev = availbuffer->last;
		availbuffer->last = element;
	}
#ifdef NODAEMON_DEBUG
	else
	{
		fprintf(stderr, "returnBufferElement ELSE\n");
		
		bufferelement *buffer=*availbuf;
		while (buffer != NULL)
		{
			fprintf(stderr, "[%p] element->prev %p element->next %p element->last %p\n",
				buffer, buffer->prev, buffer->next, buffer->last);
			buffer = buffer->next;
		}
		fprintf(stderr, "\n");

		return;
	}
#endif
	
	*availbuf = availbuffer;

	if (clearout == 0)
	{
		return;
	}
	if (element->header.counter > 0 && element->header.counter <= MAX_PACKET_COUNTER)
	{
		packetcountersinlist[element->header.counter] = 0;
	}
}


//returns a chain from the beginning of the buffer of which packets that have timed out
bufferelement *getTimedoutPackets(bufferelement **buf, uint64_t ms_timeout)
{
	bufferelement *current,*result;
	struct timeval tv;
	uint64_t threshold;
	int timeresult;
	
	if (*buf == NULL)
	{
		return NULL;
	}
	
	timeresult = gettimeofday(&tv,NULL);
	if (timeresult)
	{
		LOGMSG(LOG_WARNING, "Time returned error %d",
			timeresult);
	}
	threshold = tv.tv_sec*1000000ULL + tv.tv_usec - ms_timeout*1000ULL;

	if ((*buf)->header.sendtime >= threshold)
	{
		return NULL;
	}
	
	current = *buf;
	while (current->header.sendtime <= threshold)
	{
		current = current->next;
		if (current == NULL)
		{
			break;
		}
	}
	
	result = *buf;
	if (current == NULL)
	{
		// all packets in buffer must be sent
		*buf = NULL;
		return result;
	}
	else if (current->prev != NULL)
	{
		current->prev->next = NULL;
		current->last = (*buf)->last;
		(*buf)->last = current->prev;
		current->prev = NULL;
		*buf = current;
	}
#ifdef NODAEMON_DEBUG_BUFFER
	else
	{
		fprintf(stderr, "-------- --------- ------- \n");
	}
#endif
	
	return result;
}

// Assume calling code have done appropriate locking on mutex_buffer
int free_time_out_buffer(bufferelement **buffer_to_clean, bufferelement **buffer_to_return_to, uint64_t msbuffer)
{
	bufferelement *current_bufelement = NULL;
	bufferelement *tmp_bufelement = NULL;
	int clean_counter = 0;

	current_bufelement = getTimedoutPackets(buffer_to_clean, msbuffer);

	while (current_bufelement != NULL &&
		   pstate != STATE_SHUTDOWN)
	{
#if 0
		LOGMSG(LOG_NOTICE, "buffer: free_time_out_buffer: %d",
			current_bufelement->header.counter);
#endif
		tmp_bufelement = current_bufelement->next;
		returnBufferElement(buffer_to_return_to, current_bufelement, 1);
		current_bufelement = tmp_bufelement;
		clean_counter++;
	}

	return clean_counter;
}

int putInBuffer(bufferelement **buf, bufferelement *newelement)
{
	bufferelement *current, *buffer = *buf;

	if (buffer == NULL)
	{
		// empty buffer
		buffer = newelement;
		buffer->next = NULL;
		buffer->prev = NULL;
		buffer->last = buffer;
		*buf = buffer;
		return 0;
	}

	if (buffer->last->header.sendtime <= newelement->header.sendtime)
	{
		// Put it last - its sendtime are later than the last element
		current = buffer->last;
		current->next = newelement;
		buffer->last = newelement;
		newelement->prev=current;
		newelement->next = NULL;
	}
	else
	{
		// search the place to put the packet
		current = buffer;
		while (current->header.sendtime <= newelement->header.sendtime &&
			   current->next != NULL)
		{
			current = current->next;
		}

		if (current->next == NULL)
		{
			current->next = newelement;
			buffer->last = newelement;
			newelement->prev=current;
			newelement->next = NULL;
		}
		else
		{
			//insert in middle
			newelement->next= current->next;
			current->next->prev = newelement;
			current->next = newelement;
			newelement->prev = current;
		}
	}
	*buf = buffer;
	return 0;
}

void move_first_element_to_other_buffer(bufferelement **frombuffer, bufferelement **tobuffer)
{
	// chop of the head of the buffer
	bufferelement *current_buffer_element = *frombuffer;

	if ((*frombuffer)->next != NULL)
	{
		(*frombuffer)->next->last = (*frombuffer)->last;
		(*frombuffer)->next->prev = NULL;
		*frombuffer = (*frombuffer)->next;
	}
	else
	{
		*frombuffer = NULL;
	}

	putInBuffer(tobuffer, current_buffer_element);
}

void move_first_element_last(bufferelement **buffer)
{
	// If only one element in buffer
	if ((*buffer)->next == NULL)
	{
		return;
	}

	bufferelement *first_element = *buffer;
	bufferelement *second_element = (*buffer)->next;
	bufferelement *old_last_element = (*buffer)->last;

	old_last_element->next = first_element;
	old_last_element->last = first_element;

	first_element->next = NULL;
	first_element->last = NULL;
	first_element->prev = old_last_element;
	
	second_element->prev = NULL;
	second_element->last = first_element;

	*buffer = second_element;
}

bufferelement *findByCounter(bufferelement **buf, uint32_t counter)
{
	if (buf == NULL || (*buf) == NULL)
	{
		return NULL;
	}

	bufferelement *buffer = *buf;
	bufferelement *current = *buf;
	bufferelement *result = NULL;

	// Assume that the loop is fast enough to check pstate before / after
	while (current != NULL)
	{
		if (current->header.counter != counter)
		{
			// This is not the item we are looking for - continue
			current = current->next;
			continue;
		}
		break;
	}

	if (current == NULL)
	{
		// Did not find any matching buffer
		return NULL;
	}

	// Have found the item we are looking for
	result = current;

	// This is the only packet in the buffer
	if (current == buffer && buffer->next == NULL)
	{
		*buf = NULL;
	}
	// This is the first element - chop the head!
	else if (current == buffer)
	{
		//fprintf(stderr, "!findByCounter(%d): first - chop of the head\n", current->header.counter);
		bufferelement *second = (*buf)->next;
		second->prev = NULL;
		second->last = (*buf)->last;
		(*buf) = second;
	}
	// This is the last
	else if (current->next == NULL)
	{
		buffer->last = current->prev;
		current->prev->next = NULL;
	}
	// This is in the middle somewhere
	else
	{
		current->prev->next = current->next;
		current->next->prev = current->prev;
	}
	result->next = NULL;
	result->prev = NULL;
	result->last = NULL;
	return result;
}

void addHeaderAndPutInBuffer(bufferelement **buf, bufferelement *element, uint8_t typeofpacket,
							 uint32_t sessionid, uint16_t payloadlength, uint8_t channelnumber, uint32_t *counter)
{
	struct timeval tv;
	uint64_t sendtime;

	element->packet_buffer[0] = typeofpacket << 4;
	element->packet_buffer[1] = (sessionid & 0xff000000) >> 24;
	element->packet_buffer[2] = (sessionid & 0x00ff0000) >> 16;
	element->packet_buffer[3] = (sessionid & 0x0000ff00) >> 8;
	element->packet_buffer[4] = sessionid & 0x000000ff;
	element->packet_buffer[5] = (payloadlength & 0xff00) >> 8;
	element->packet_buffer[6] = payloadlength & 0x00ff;
	element->packet_buffer[7] = channelnumber;
	element->packet_buffer[8] = (*counter & 0x00ff0000ULL) >> 16;
	element->packet_buffer[9] = (*counter & 0x0000ff00ULL) >> 8;
	element->packet_buffer[10] = (*counter & 0x000000ffULL);
	element->header.counter = *counter;

	*counter += 1;
#ifdef RUDP_FILE
	if (typeofpacket == RUDP_PACK_DATA_FILEINFO)
	{
		// RUDP_PACK_DATA_FILEINFO is the status update packets.
		if (*counter > MAX_FILEINFO_COUNTER)
		{
			*counter = MAX_PACKET_COUNTER + 1;
		}
	}
	else
	{
		if (*counter > MAX_PACKET_COUNTER)
		{
			*counter = 1;
		}
	}
#else
	if (*counter > MAX_PACKET_COUNTER)
	{
		*counter = 1;
	}
#endif

	memset(&tv,0,sizeof(struct timeval));
	gettimeofday(&tv, NULL);

	sendtime = tv.tv_sec*1000000ULL + tv.tv_usec;
	element->header.sendtime = sendtime;
	element->header.typeofpacket = typeofpacket;
	element->header.payloadlength = payloadlength;

	element->packet_buffer[11] = (sendtime & 0xff00000000000000ULL) >> 56;
	element->packet_buffer[12] = (sendtime & 0x00ff000000000000ULL) >> 48;
	element->packet_buffer[13] = (sendtime & 0x0000ff0000000000ULL) >> 40;
	element->packet_buffer[14] = (sendtime & 0x000000ff00000000ULL) >> 32;
	element->packet_buffer[15] = (sendtime & 0x00000000ff000000ULL) >> 24;
	element->packet_buffer[16] = (sendtime & 0x0000000000ff0000ULL) >> 16;
	element->packet_buffer[17] = (sendtime & 0x000000000000ff00ULL) >> 8;
	element->packet_buffer[18] = (sendtime & 0x00000000000000ffULL);

	putInBuffer(buf, element);
}

int receiverPutInBuffer(bufferelement **buf, bufferelement *newelement, int buffer_timeout)
{
	bufferelement *current = NULL;
	bufferelement *buffer = *buf;
	struct timeval tv;
	uint64_t curtime = 0;

	// Sanity check - counter cant be 0 and cant be greater than MAX_PACKET_COUNTER
	if (newelement->header.counter == 0 ||
			newelement->header.counter > MAX_PACKET_COUNTER)
	{
#ifdef NODAEMON_DEBUG_BUFFER
		LOGMSG(LOG_NOTICE, "%d: Illegal counter %d.",
			   __LINE__, newelement->header.counter);
#endif
		return 1;
	}

	// Check if this counter are in the packetcountersinlist => this is a resend packet
	if (packetcountersinlist[newelement->header.counter])
	{
#ifdef NODAEMON_DEBUG_BUFFER
		int pos_in_list;
		int count;
		count_packets_and_check_if_counter_is_in_list(*buf, newelement->header.counter, &count, &pos_in_list);
		LOGMSG(LOG_NOTICE, "Already in list %d #packets: %d isInList: %d",
			   newelement->header.counter, count, pos_in_list);
		if (pos_in_list == -1)
		{
			exit(1);
		}
#endif
		return 2;
	}

	// Check time against buffer time - is it to late to send it?
	// This check are for "realtime" streaming without disc backend
	gettimeofday(&tv, NULL);
	curtime = tv.tv_sec * 1000000 + tv.tv_usec;
	if (newelement->header.sendtime < (curtime - buffer_timeout*1000))
	{
#ifdef NODAEMON_DEBUG_BUFFER
		LOGMSG(LOG_NOTICE, "%d Packet to old to be used! %d sendtime %"PRIu64" < (%"PRIu64" - %d*1000=%"PRIu64") diff: %"PRIu64,
			   __LINE__, countList(*buf),
			   newelement->header.sendtime,
			   curtime, buffer_timeout, (curtime - buffer_timeout*1000),
			   (curtime - buffer_timeout*1000)-newelement->header.sendtime
			   );
#endif
		return 3;
	}

	// 1. Buffer is empty
	// Insert the new element in buffer
	// 2. Buffer has only one element
	// Insert it after the element in buffer
	// Insert it infront of the element in buffer
	// --- its more than one element in the buffer ---
	// 3. Test if it should be placed last (sendtime and counter on new element are greater than the last in buffer)
	// 4. Test if it should be placed first (sendtime and counter on new element are less than the first in buffer)
	// 5. Search backward for a place to put the new element
	// put it in infront of "current" position

	// == 1. Buffer is empty ==
	// Insert the new element in buffer
	if (buffer == NULL)
	{
		// empty buffer
		buffer = newelement;
		buffer->next = NULL;
		buffer->prev = NULL;
		buffer->last = buffer;
		*buf = buffer;
		packetcountersinlist[newelement->header.counter] = 1;
		return 0;
	}

	// == 2. Buffer has only one element ==
	// Insert it after the element in buffer
	// Insert it infront of the element in buffer
	if (buffer->next == NULL)
	{
		// Check sendtime and (the counters or a windows around the wrapping numbers)
		if (buffer->header.sendtime <= newelement->header.sendtime &&
				((buffer->header.counter < newelement->header.counter) ||
				 (buffer->header.counter > MAX_PACKET_COUNTER-10 && newelement->header.counter < 10)))
		{
#ifdef NODAEMON_DEBUG_BUFFER
			LOGMSG(LOG_NOTICE, "2. Buffer has only one element -> put it last %u [%u]",
				   buffer->header.counter, newelement->header.counter);
#endif
			// Put it last
			buffer->next = newelement;
			buffer->last = newelement;
			// Dont need to update buffer->prev - its the same
			newelement->prev = buffer;
			newelement->next = NULL;
			newelement->last = newelement;
			// Dont need to update *buf - its the same
		}
		else
		{
#ifdef NODAEMON_DEBUG_BUFFER
			LOGMSG(LOG_NOTICE, "2. Buffer has only one element -> put it infront [%u] %u",
				   newelement->header.counter, buffer->header.counter);
#endif
			// Put it infront
			buffer->prev = newelement;
			buffer->next = NULL;
			buffer->last = buffer;
			newelement->next = buffer;
			newelement->prev = NULL;
			newelement->last = buffer;
			*buf = newelement;
		}
	}

	// --- its more than one element in the buffer ---
	// == 3. Test if it should be placed last (sendtime and counter on new element are greater than the last in buffer) ==
	else if ((buffer->last->header.sendtime < newelement->header.sendtime) ||
			 ((buffer->last->header.sendtime == newelement->header.sendtime) &&
			  ((buffer->last->header.counter < newelement->header.counter) ||
			   (buffer->last->header.counter > MAX_PACKET_COUNTER-10 && newelement->header.counter < 10))))
	{
		// Put it last
#ifdef NODAEMON_DEBUG_BUFFER
		LOGMSG(LOG_NOTICE, "3. Test if it should be placed last -> put it last %u [%u]",
			   buffer->last->header.counter, newelement->header.counter);
#endif
		buffer->last->next = newelement;
		buffer->last->last = newelement;
		// Dont need to update buffer->last->prev - its the same
		newelement->prev = buffer->last;
		newelement->next = NULL;
		newelement->last = newelement;
		buffer->last = newelement;
		// Dont need to update *buf - its the same
	}
	// == 4. Test if it should be placed first (sendtime and counter on new element are less than the first in buffer) ==
	else if ((buffer->header.sendtime > newelement->header.sendtime) ||
			 (buffer->header.sendtime >= newelement->header.sendtime &&
			  ((buffer->header.counter > newelement->header.counter) ||
			   (buffer->header.counter < 10 && newelement->header.counter > MAX_PACKET_COUNTER-10))))
	{
#ifdef NODAEMON_DEBUG_BUFFER
		LOGMSG(LOG_NOTICE, "4. Test if it should be placed first -> put it infront [%u] %u",
			   newelement->header.counter, buffer->header.counter);
#endif
		// Put it infront
		// Dont need to update buffer->next its the same
		// Dont need to update buffer->last its the same
		buffer->prev = newelement;
		newelement->next = buffer;
		newelement->prev = NULL;
		newelement->last = buffer->last;
		*buf = newelement;
#ifdef NODAEMON_DEBUG_BUFFER
		LOGMSG(LOG_NOTICE, "4. after [%u] %u",
			   (*buf)->header.counter, (*buf)->next->header.counter);
		LOGMSG(LOG_NOTICE, "4. prev [%u] %u",
			   (*buf)->next->prev->header.counter, (*buf)->next->header.counter);
#endif
	}

	// == 5. Search backward for a place to put the new element ==
	// put it in infront of "current" position
	else
	{
		// more than one packet in buffer
		// Start from the last and search forward
		current = buffer->last;

		while (current->prev != NULL &&
			   pstate != STATE_SHUTDOWN)
		{
			// walk through the buffer and check if
			// elements sendtime are later in time (strict bigger) than the new element
			// or
			// elements sendtime are equal and elements counter are bigger than the newelement
			// move pointer to previous element
			// Otherwise
			// Break
			// current shall point to an element which comes prior to the new element in time
			if ((current->header.sendtime > newelement->header.sendtime) ||
					(current->header.sendtime == newelement->header.sendtime && current->header.counter > newelement->header.counter))
			{
#ifdef NODAEMON_DEBUG_BUFFER
				LOGMSG(LOG_NOTICE, "current->header.counter %u > newelement->header.counter %u (%d)",
					   current->header.counter, newelement->header.counter,
					   (current->header.counter > newelement->header.counter));
#endif
				current = current->prev;
			}
			else
			{
#ifdef NODAEMON_DEBUG_BUFFER
				LOGMSG(LOG_NOTICE,
					   "next->header.counter = %u %d => %d current->header.counter %u > newelement->header.counter %u && %d",
					   current->next->header.counter, current->next->header.counter > newelement->header.counter,
					   ( current->header.counter > newelement->header.counter),
					   current->header.counter, newelement->header.counter,
					   ( current->header.sendtime >= newelement->header.sendtime));
#endif
				break;
			}
		}
		if (pstate == STATE_SHUTDOWN)
		{
			return 4;
		}

#ifdef NODAEMON_DEBUG_BUFFER
		if (current == buffer)
		{
			LOGMSG(LOG_NOTICE, "Inserting packet position 1 in buffer! counter %d",
				   newelement->header.counter);
		}
		if (current->prev == NULL)
		{
			LOGMSG(LOG_NOTICE, "5. prev == NULL - counter: current %u > new %u",
				   current->header.counter, newelement->header.counter);
			LOGMSG(LOG_NOTICE, "sendtime current %"PRIu64" >= %"PRIu64,
				   current->header.sendtime, newelement->header.sendtime);
			LOGMSG(LOG_NOTICE, "current %p last %p",
				   current, buffer->last);

		}
		if (current->next == NULL)
		{
			LOGMSG(LOG_NOTICE, "5. next == NULL - counter: current %u > new %u",
				   current->header.counter, newelement->header.counter);
			LOGMSG(LOG_NOTICE, "sendtime current %"PRIu64" >= %"PRIu64,
				   current->header.sendtime, newelement->header.sendtime);
			LOGMSG(LOG_NOTICE, "current %p last %p",
				   current, buffer->last);
		}
#endif
		//if (newelement->header.counter != current->header.counter)
		if (current == buffer->last)
		{
			// Put it last
			current->next = newelement;
			current->last = newelement;
			// Dont need to update buffer->prev - its the same
			newelement->prev = current;
			newelement->next = NULL;
			newelement->last = newelement;

			buffer->last = newelement;
			// Dont need to update *buf - its the same
		}
		else
		{
			// Put it in the middle - after current
			newelement->next = current->next;
			newelement->prev = current;
			newelement->last = buffer->last;
			current->next->prev = newelement;
			current->next = newelement;

#ifdef NODAEMON_DEBUG_BUFFER
			LOGMSG(LOG_NOTICE, "FILL HOLE %u [%u] %u",
				   current->header.counter, newelement->header.counter,
				   newelement->next->header.counter);
			if (newelement->prev != NULL)
			{
				LOGMSG(LOG_NOTICE, "1. FILL HOLE %u [%u] %u",
					   newelement->prev->header.counter,
					   newelement->header.counter,
					   newelement->next->header.counter);
			}
			else
			{
				LOGMSG(LOG_NOTICE, "2. FILL HOLE [%u] %u",
					   newelement->header.counter, newelement->next->header.counter);
			}
			if (current->prev != NULL)
			{
				LOGMSG(LOG_NOTICE, "1. current: FILL HOLE %u %u [%u] %u",
					   current->prev->header.counter,
					   current->header.counter,
					   current->next->header.counter,
					   current->next->next->header.counter);
			}
			else
			{
				LOGMSG(LOG_NOTICE, "2. current: FILL HOLE %u [%u] %u",
					   current->header.counter,
					   current->next->header.counter,
					   current->next->next->header.counter);
			}
			LOGMSG(LOG_NOTICE, "time in front: %"PRIu64 " < %"PRIu64" (new) %d < %"PRIu64" %d < last(%u) %"PRIu64" %d",
				   current->header.sendtime,
				   newelement->header.sendtime,
				   (current->header.sendtime <= newelement->header.sendtime),
				   newelement->next->header.sendtime,
				   (newelement->header.sendtime <= newelement->next->header.sendtime),
				   (*buf)->last->header.counter, (*buf)->last->header.sendtime,
				   (newelement->next->header.sendtime <= (*buf)->last->header.sendtime));
#endif
		}
	}
#ifdef NODAEMON_DEBUG_BUFFER
	int lastID = newelement->header.counter-1;
	if (newelement->header.counter == 0)
	{
		lastID =  MAX_PACKET_COUNTER-1;
	}
	if (packetcountersinlist[lastID] == 0)
	{
		LOGMSG(LOG_NOTICE, "HAS NOT GOT COUNTER [%d] new header %u",
			lastID, newelement->header.counter);
	}
#endif
	packetcountersinlist[newelement->header.counter] = 1;
	return 0;
}

bufferelement *check_and_get_resend_buffer(bufferelement **src, uint32_t last_counter, uint64_t last_time_written_to_disc)
{
	bufferelement *source = *src;
	bufferelement *ret = NULL;

	if (source == NULL)
	{
		return NULL;
	}
	// last_counter == MAX_PACKET_COUNTER+1 the first buffer in this file
	if (last_counter == (MAX_PACKET_COUNTER + 1))
	{
		return NULL;
	}
	if (last_counter == MAX_PACKET_COUNTER &&
			source->header.counter == 1)
	{
		// Dont do anything
		// Its ok
		return NULL;
	}
	if (source->header.counter != (last_counter + 1))
	{
#if 0
		if (source->next == NULL)
		{
			LOGMSG(LOG_WARNING, "check_and_resend1: GAP IN COUNTER %u != %u + 1 has NO NEXT",
				   source->header.counter, last_counter);
		}
		else
		{
			LOGMSG(LOG_WARNING,
				   "check_and_resend2: GAP IN COUNTER %u != %u + 1 has NEXT %u has next->next %d",
				   source->header.counter, last_counter, source->next->header.counter,
				   (source->next->next != NULL));
		}
#endif
		// If source sendtime <= last_time_written_to_disc and counter are less than last_counter
		if (source->header.sendtime <= last_time_written_to_disc)
		{
#ifdef NODAEMON_DEBUG_BUFFER
#if 0
			LOGMSG(LOG_WARNING, "source.sendtime %"PRIu64"  <= %"PRIu64,
				   source->header.sendtime, last_time_written_to_disc);
#endif
			// source   [  5  6  7  8  9 ] last_counter 6
			// sendtime [ 10 11 12 13 14 ] last_time_written_to_disc 11
			// Chop 5
			//
			if (source->header.counter < last_counter)
			{
				LOGMSG(LOG_WARNING,
					   "1. This is a resent packet source->header.counter %u < lastcounter %u",
					   source->header.counter, last_counter);
			}

			// source [ 54291 6 7 8 9 ]
			// last_counter 5
			else if (source->header.counter > (MAX_PACKET_COUNTER - 100) && last_counter < 100)
			{
				LOGMSG(LOG_WARNING,
					   "2. This is a resent wrapped packet source->header.counter %u < lastcounter %u",
					   source->header.counter, last_counter);
			}
#endif

			ret = *src;
			if (source->next != NULL)
			{
				source->next->last = source->last;
				source->next->prev = NULL;
				*src = source->next;
			}
			else
			{
				*src = NULL;
			}
			ret->next = NULL;
			ret->prev = NULL;
			return ret;
		}
	}
	return NULL;
}

bufferelement *chop_next_counter_head_of_buffer(bufferelement **src, uint32_t last_counter)
{
	bufferelement *source = *src;

	if (source == NULL)
	{
		return NULL;
	}
	// if the counter is increasing its ok - chop the head and return
	if ((last_counter+1) == source->header.counter)
	{
		return chop_head_of_buffer(src);
	}

	// if its a special counter - to be removed
	if (last_counter == (MAX_PACKET_COUNTER + 1) || last_counter == 0)
	{
		return chop_head_of_buffer(src);
	}

	// if the counter has wrapped
	if (last_counter == MAX_PACKET_COUNTER &&
		source->header.counter == 1)
	{
		// Its ok - wrapped
		return chop_head_of_buffer(src);
	}

#if 0
	if (source->next == NULL)
	{
		fprintf(stderr,
			"chop_head_of_buffer1: GAP IN COUNTER %u != %u + 1 has NO NEXT\n",
			source->header.counter, last_counter);
	}
	else
	{
		fprintf(stderr,
			"chop_head_of_buffer2: GAP IN COUNTER %u != %u + 1 has NEXT %u has next->next %d\n",
			source->header.counter, last_counter, source->next->header.counter,
			(source->next->next != NULL));
		if (source->header.counter > last_counter)
		{
			fprintf(stderr,
				"source.sendtime %"PRIu64"  source->next->header.sendtime %"PRIu64"\n",
				source->header.sendtime, source->next->header.sendtime);
		}
	}
#endif
	return NULL;
}

// No logic - give me the first element! (or null if its empty)
bufferelement *chop_head_of_buffer(bufferelement **src)
{
	bufferelement *source = *src;
	bufferelement *ret = NULL;

	if (source == NULL)
	{
		return NULL;
	}

	ret = *src;
	if (source->next != NULL)
	{
		source->next->last = source->last;
		source->next->prev = NULL;
		*src = source->next;
	}
	else
	{
		*src = NULL;
	}
	ret->next = NULL;
	ret->prev = NULL;
	return ret;
}

