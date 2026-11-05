#ifndef __DELAYER_H__
#define __DELAYER_H__

#define TOTAL_DISC_BUFFER_TIME_IN_S 60
#define DISC_BUFFER_READAHEAD_TIME 10000

#include <pthread.h>
#include "delayer_common.h"
#include "buffer.h"
#include "stat_func.h"
#include "thread_statistic.h"

typedef struct _buffer_queue
{
	pthread_mutex_t mutex;
	int no_buffer_elements;
	bufferelement *buffer_elements;
} buffer_queue;


// if in livestreaming and a pause issued, the curpos are being updated with current timestamp.
// following play will put the state in PLAY_DELAYED

typedef struct _thread_mc_arg
{
	char multicast_address[16];
	int multicast_port;
	int multicast_ttl;
	uint64_t buffer_timeout;
	e_process_state *state;
	char *name;
	uint16_t rawudp;
	uint64_t stat_last_timestamp;
	uint16_t pathno;

	// statistics
	pthread_mutex_t *mutex_stat;
	s_runtimestat *rt_stats;
	sequence_statistics *rudp_sequence_list;

	// in multicast receiver this is the buffer_q_available[num_backup_sinks+1]
	// in multicast sender this is the buffer_q_available[sink_no]
	buffer_queue *buffer_q_available;

	// in multicast receiver this is the disc_buffer_elements
	// in multicast sender this is the send_buffer_elements
	buffer_queue *buffer_q;
} thread_mc_arg;

typedef struct _thread_mc_receiver_arg
{
	char multicast_address[16];
	int multicast_port;
	int multicast_ttl;
	uint64_t buffer_timeout;
	e_process_state *state;
	char *name;
	uint16_t rawudp;
	uint64_t stat_last_timestamp;
	uint16_t pathno;

	// statistics
	pthread_mutex_t *mutex_stat;
	s_runtimestat *rt_stats;
	sequence_statistics *rudp_sequence_list;

	// In Livestreaming writer will copy buffer to buffer_q_sender
	// otherwise just return the written buffer to buffer_q_available[no_sink]
	// No_sink = 1 (base interface) + num_backup_sink + 1 (disc)
	int no_sink;

	// in multicast receiver this is the buffer_q_available[num_backup_sinks+1]
	buffer_queue *buffer_q_available;

	// in multicast receiver this is the disc_buffer_elements
	buffer_queue *buffer_q_disc;

	// buffer_q_send are livestream shortcut to send
	buffer_queue *buffer_q_send;// Need to be an array!!!

	PGconn **dbconn;
	pthread_mutex_t *mutex_dbconn;


} thread_mc_receiver_arg;


typedef struct _pause_arg
{
	pthread_mutex_t mutex_pause;
	long pause_pos;
	uint64_t pause_time;
	uint64_t pause_diff;
	uint32_t pause_file_id;
} s_pause_arg;

typedef enum _file_status
{
	UNINIT=0,
	UNPARSED,
	VALID, // the file seems to be OK
	CALCULATED_END_TIME, // if read end time are way off (happens when the application crash)
	CORRUPT, // file is empty
	IN_PROGRESS // file is written to.
} e_file_status;

typedef struct _file_info
{
	pthread_mutex_t mutex_file;
	uint32_t id;
	uint64_t start_time;
	uint64_t end_time;
	e_file_status status;
} s_file_info;

typedef struct _thread_disc_writer_arg
{
	e_process_state *state;
	char *name;
	uint64_t streaminterface_id;
	s_file_info *writer_file;

	buffer_queue *buffer_q_available;
	// disc_buffer_elements
	buffer_queue *buffer_q_disc;
} thread_disc_writer_arg;

typedef struct _thread_disc_reader_arg
{
	e_process_state *state;
	char *name;
	uint64_t streaminterface_id;
	uint16_t pathno;
	s_file_info *writer_file;
	s_file_info *reader_file;
	s_pause_arg *pause;

	buffer_queue *buffer_q_available;
	buffer_queue *buffer_q_send;
	PGconn **dbconn;
	pthread_mutex_t *mutex_dbconn;
} thread_disc_reader_arg;

typedef struct _thread_disc_cleaner_arg
{
	e_process_state *state;
	s_pause_arg *pause;
	pthread_mutex_t *mutex_discclean_signal;
	pthread_cond_t *condt_discclean_signal;
	uint64_t *total_disc_buffer_time;
	char *name;
	uint64_t streaminterface_id;
	s_file_info *writer_file;
	s_file_info *reader_file;
	s_file_info *oldest_file;
} s_thread_disc_cleaner_arg;

int find_time(char *name, uint32_t file_id, uint64_t searched_time, s_pause_arg *pause);
int taste_file(char *name, s_file_info *file);
int find_time_in_buffer(char *name, uint32_t oldest_id, uint32_t newest_id,
	uint64_t searched_time, s_pause_arg *pause);

void *thread_disccleaner(void *arg);

void *thread_mc_receiver(void *arg);
void *thread_discwriter(void *arg);
void *thread_discreader(void *arg);
void *thread_mc_sender(void *arg);

#endif // __DELAYER_H__
