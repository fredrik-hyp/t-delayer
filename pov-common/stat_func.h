#include <stdint.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include "prot_rudp.h"
#include "common.h"
#include "common-db.h"

#ifndef __STAT_FUNC_H__
#define __STAT_FUNC_H__

#define SEQ_STATS_MAX_LEN 32
typedef struct _sequence_statistics
{
	bool uninit;
	uint8_t num_in_buffer;
	uint16_t rtp_seq_list[SEQ_STATS_MAX_LEN];
	uint16_t rtp_seq_list_first[SEQ_STATS_MAX_LEN];
	uint8_t miss_count[SEQ_STATS_MAX_LEN];
	uint16_t reorder_last;
	uint16_t reorder_first;
	struct timeval last_tv;

	// Shared with other threads
	pthread_mutex_t *mutex_stat;
	uint64_t transfered_bytes_mc_udp;
	uint64_t transfered_bytes_rudp;
	uint32_t dropped_10_sec;
	uint32_t dropped_1_sec;
	uint64_t resend_counter;
	int num_paths;
	uint64_t transfered_bytes_rudp_per_path[MAX_INTERFACE_PATH];
	uint64_t acc_transfered_bytes_rudp_per_path[MAX_INTERFACE_PATH];
} sequence_statistics;

void stat_handle_packet_rtp(sequence_statistics *sequence, uint8_t *packet_buffer, ssize_t bytes);
uint32_t stat_handle_packet_rtp_get_dropped(sequence_statistics *sequence, uint8_t *packet_buffer, ssize_t bytes);
void stat_handle_packet_rudp(sequence_statistics *sequence, uint8_t *packet_buffer, ssize_t bytes, 
			bool resend, ssize_t *path_bytes);
void stat_handle_packet_rudp_receiver(sequence_statistics *sequence, uint8_t *packet_buffer, ssize_t bytes, 
			bool resend, ssize_t *path_bytes, rudpheader *header);
#endif // __STAT_FUNC_H__
