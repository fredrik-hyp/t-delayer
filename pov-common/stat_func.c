#include "stat_func.h"
#include "thread_statistic.h"
#include "defs.h"
#include "prot_rudp.h"
#include "common.h"

#define MAX_MISS_COUNT 64

void stat_rtp_init(sequence_statistics *sequence, uint16_t new_rtp_sequence);
void stat_rtp_check_holes(sequence_statistics *sequence);
uint32_t stat_rtp_check_if_buffer_full(sequence_statistics *sequence);
uint32_t stat_rtp_check_misscount(sequence_statistics *sequence);
void stat_rtp_insert_at_front(sequence_statistics *sequence, uint16_t new_rtp_sequence);
uint32_t stat_rtp_insert_in_buffer(sequence_statistics *sequence, uint16_t new_rtp_sequence);
uint32_t stat_rtp_handle_statistics(sequence_statistics *sequence, uint16_t new_rtp_sequence);


void stat_rtp_init(sequence_statistics *sequence, uint16_t new_rtp_sequence)
{
	sequence->uninit=0;

	memset(sequence->rtp_seq_list, 0, sizeof(uint16_t)*SEQ_STATS_MAX_LEN);
	memset(sequence->rtp_seq_list_first, 0, sizeof(uint16_t)*SEQ_STATS_MAX_LEN);
	memset(sequence->miss_count, 0, sizeof(uint8_t)*SEQ_STATS_MAX_LEN);
	sequence->reorder_last = 0;
	sequence->reorder_first = 0;

	sequence->rtp_seq_list[0] = new_rtp_sequence;
	sequence->rtp_seq_list_first[0] = new_rtp_sequence;
	sequence->num_in_buffer = 1;

	// == Shared with other threads ==
	pthread_mutex_lock(sequence->mutex_stat);
	sequence->dropped_10_sec = 0;
	sequence->dropped_1_sec = 0;
	sequence->resend_counter = 0;
	sequence->transfered_bytes_mc_udp = 0;
	sequence->transfered_bytes_rudp = 0;
	memset(sequence->transfered_bytes_rudp_per_path, 0, sizeof(uint64_t)*MAX_INTERFACE_PATH);
	memset(sequence->acc_transfered_bytes_rudp_per_path, 0, sizeof(uint64_t)*MAX_INTERFACE_PATH);
	pthread_mutex_unlock(sequence->mutex_stat);
	
}

void stat_rtp_check_holes(sequence_statistics *sequence)
{
	for (uint8_t i = 0; i < sequence->num_in_buffer-1; i++)
	{
		if ((uint16_t)(sequence->rtp_seq_list[i]+1) != sequence->rtp_seq_list_first[i+1])
		{
			continue;
		}
		// Have found a sequence!
		sequence->rtp_seq_list[i] = sequence->rtp_seq_list[i+1];
		if (i != sequence->num_in_buffer-1)
		{
			// Close the gap
			for (i++; i < sequence->num_in_buffer-1; i++)
			{
				sequence->rtp_seq_list[i] = sequence->rtp_seq_list[i+1];
				sequence->rtp_seq_list_first[i] = sequence->rtp_seq_list_first[i+1];
				sequence->miss_count[i] = sequence->miss_count[i+1];
			}
			sequence->num_in_buffer--;
		}
	}
}

uint32_t stat_rtp_check_if_buffer_full(sequence_statistics *sequence)
{
	uint32_t total_dropped = 0;
	if (sequence->num_in_buffer == SEQ_STATS_MAX_LEN)
	{
		uint8_t i, dropp_diff=0;
		if (sequence->rtp_seq_list_first[1] > sequence->rtp_seq_list[0])
		{
			dropp_diff = (sequence->rtp_seq_list_first[1] - sequence->rtp_seq_list[0]);
		}
		else
		{
			dropp_diff = (65536+(uint32_t)sequence->rtp_seq_list_first[1]) - ((uint32_t)sequence->rtp_seq_list[0]);
		}
		if (dropp_diff > 1)
		{
			total_dropped += dropp_diff - 1;
		}

#ifdef NODAEMON_DEBUG
		fprintf(stderr, "stat_rtp_check_if_buffer_full: Buffer is full dropped: %d\n", dropp_diff);
#else
		for (i = 0; i < SEQ_STATS_MAX_LEN; i++)
			LOGMSG(LOG_WARNING, "stat_rtp_check_if_buffer_full: [%d] %u %u", i, sequence->rtp_seq_list_first[i], sequence->rtp_seq_list[i]);	
#endif
		for (i = 0; i < sequence->num_in_buffer-1; i++)
		{
			sequence->rtp_seq_list[i] = sequence->rtp_seq_list[i+1];
			sequence->rtp_seq_list_first[i] = sequence->rtp_seq_list_first[i+1];
			sequence->miss_count[i] = sequence->miss_count[i+1];
		}
		sequence->num_in_buffer--;
	}
	return total_dropped;
}

uint32_t stat_rtp_check_misscount(sequence_statistics *sequence)
{
	uint32_t total_dropped = 0;
	for (uint8_t x = 0; x < sequence->num_in_buffer-1; x++)
	{
		if (sequence->miss_count[x] > MAX_MISS_COUNT)
		{
			uint32_t dropp_diff; // = (sequence->rtp_seq_list_first[x+1] - sequence->rtp_seq_list[x]);
			if (sequence->rtp_seq_list_first[x+1] > sequence->rtp_seq_list[x])
			{
				dropp_diff = (sequence->rtp_seq_list_first[x+1] - sequence->rtp_seq_list[x]);
			}
			else
			{
				dropp_diff = (65536+(uint32_t)sequence->rtp_seq_list_first[x+1]) - ((uint32_t)sequence->rtp_seq_list[x]);
			}

			if (dropp_diff > 1)
			{
				total_dropped += dropp_diff - 1;
			}
#ifdef NODAEMON_DEBUG
			fprintf(stderr, "stat_rtp_check_misscount: misscount reached limit dropping %d\n", dropp_diff);
#else
			for (int i = 0; i <  sequence->num_in_buffer; i++)
				LOGMSG(LOG_WARNING, "stat_rtp_check_misscount: [%d] %u %u", i, sequence->rtp_seq_list_first[i], sequence->rtp_seq_list[i]);
#endif
			// Move one step down
			for (uint8_t i = x; i < sequence->num_in_buffer-1; i++)
			{
				sequence->rtp_seq_list[i] = sequence->rtp_seq_list[i+1];
				sequence->rtp_seq_list_first[i] = sequence->rtp_seq_list_first[i+1];
				sequence->miss_count[i] = sequence->miss_count[i+1];
			}
			sequence->num_in_buffer--;
			// for will increment the counter with one and "this" index has a new data in it
			// So need to do some dirty tricks
			x--;
		}
	}
	return total_dropped;
}

void stat_rtp_insert_at_front(sequence_statistics *sequence, uint16_t new_rtp_sequence)
{
	sequence->rtp_seq_list[0] = new_rtp_sequence;
	sequence->rtp_seq_list_first[0] = new_rtp_sequence;
	stat_rtp_check_holes(sequence);
}

uint32_t stat_rtp_insert_in_buffer(sequence_statistics *sequence, uint16_t new_rtp_sequence)
{
	// Search and look if there is other places with a sequence
	uint8_t didnt_found_a_sequence = 1;
	for (uint8_t i = 0; i < sequence->num_in_buffer; i++)
	{
		if ((uint16_t)(sequence->rtp_seq_list[i]+1) == new_rtp_sequence)
		{
#ifdef NODAEMON_DEBUG
			fprintf(stderr, "insert LAST in sequence [%d - %d]\n",
					sequence->rtp_seq_list_first[i], new_rtp_sequence);
#endif
			sequence->rtp_seq_list[i] = new_rtp_sequence;
			sequence->reorder_last++;
			didnt_found_a_sequence = 0;
			break;
		}
		else if ((uint16_t)(new_rtp_sequence+1) == sequence->rtp_seq_list_first[i])
		{
#ifdef NODAEMON_DEBUG
			fprintf(stderr, "insert FIRST in sequence [%d - %d]\n",
					new_rtp_sequence, sequence->rtp_seq_list[i]);
#endif
			sequence->rtp_seq_list_first[i] = new_rtp_sequence;
			sequence->reorder_first++;
			didnt_found_a_sequence = 0;
			break;
		}
		sequence->miss_count[i]++;
	}
	uint32_t dropped = 0;
	// === MANAGEMENT ===
	// Remove filled holes
	stat_rtp_check_holes(sequence);

	// Remove miss count > MAX_MISS_COUNT
	dropped = stat_rtp_check_misscount(sequence);

	// Check if the buffer is full
	dropped += stat_rtp_check_if_buffer_full(sequence);

	if (didnt_found_a_sequence)
	{
		// === ADD THE NEW VALUE TO THE END ===
		uint8_t index = sequence->num_in_buffer;
		sequence->num_in_buffer++;
		sequence->rtp_seq_list[index] = new_rtp_sequence;
		sequence->rtp_seq_list_first[index] = new_rtp_sequence;
		sequence->miss_count[index] = 0;
	}
	return dropped;
}

uint32_t stat_rtp_handle_statistics(sequence_statistics *sequence, uint16_t new_rtp_sequence)
{
	uint32_t dropped = 0;
	if (sequence->uninit)
	{
		stat_rtp_init(sequence, new_rtp_sequence);
		return dropped;
	}
	
	if ((uint16_t)(sequence->rtp_seq_list[0]+1) == new_rtp_sequence)
	{
		stat_rtp_insert_at_front(sequence, new_rtp_sequence);
		return dropped;
	}

	dropped = stat_rtp_insert_in_buffer(sequence, new_rtp_sequence);
	return dropped;
}

void stat_handle_packet_rtp(sequence_statistics *sequence, uint8_t *packet_buffer, ssize_t bytes)
{
	uint32_t dropped = 0;
#ifdef NORUDPSTAT
	if (packet_buffer[0] == MPEG_SYNC_BYTE)
#else
	if (packet_buffer[RUDP_HEADER_SIZE] == MPEG_SYNC_BYTE)
#endif
	{
		// Not implemented yet
	}
#ifdef NORUDPSTAT
	else if(bytes > RTP_MIN_HEADER_SIZE)
	{
		struct raw_rtp_packet *packet = (struct raw_rtp_packet *)(packet_buffer);
#else
	else if (bytes > (RUDP_HEADER_SIZE + RTP_MIN_HEADER_SIZE))
	{
		struct raw_rtp_packet *packet = (struct raw_rtp_packet *)(packet_buffer+RUDP_HEADER_SIZE);
#endif
		uint16_t new_sequence = ntohs(packet->sequence_number);
		dropped = stat_rtp_handle_statistics(sequence, new_sequence);
	}
	pthread_mutex_lock(sequence->mutex_stat);
	sequence->transfered_bytes_mc_udp += bytes;
	sequence->dropped_10_sec += dropped;
	sequence->dropped_1_sec += dropped;
	pthread_mutex_unlock(sequence->mutex_stat);
}

uint32_t stat_handle_packet_rtp_get_dropped(sequence_statistics *sequence, uint8_t *packet_buffer, ssize_t bytes)
{
	uint32_t dropped = 0;
	if (packet_buffer[RUDP_HEADER_SIZE] == MPEG_SYNC_BYTE)
	{
		// Not implemented yet
	}
	else if (bytes > (RUDP_HEADER_SIZE + RTP_MIN_HEADER_SIZE))
	{
		struct raw_rtp_packet *packet = (struct raw_rtp_packet *)(packet_buffer+RUDP_HEADER_SIZE);
		uint16_t new_sequence = ntohs(packet->sequence_number);
		dropped = stat_rtp_handle_statistics(sequence, new_sequence);
	}
	return dropped;
}

void stat_handle_packet_rudp(sequence_statistics *sequence, uint8_t *packet_buffer, ssize_t bytes, bool resend, ssize_t *path_bytes)
{
	pthread_mutex_lock(sequence->mutex_stat);
	sequence->transfered_bytes_rudp += bytes;

	for (int i = 0; i < sequence->num_paths && i < MAX_INTERFACE_PATH; i++)
	{
		sequence->transfered_bytes_rudp_per_path[i] += path_bytes[i];
	}

	if (resend)
	{
		sequence->resend_counter++;
	}
	pthread_mutex_unlock(sequence->mutex_stat);
}

void stat_handle_packet_rudp_receiver(sequence_statistics *sequence, uint8_t *packet_buffer, ssize_t bytes, bool resend, ssize_t *path_bytes, rudpheader *header)
{
	stat_handle_packet_rudp(sequence, packet_buffer, bytes, resend, path_bytes);
}
