#include "thread_traps.h"

#include "buffer.h"
#include "stat_func.h"
#include "print_struct.h"

#include "states.h"
#include "common-time.h"

#include <assert.h>
#ifdef __linux__
#include <syscall.h>
#endif
#include <semaphore.h>
#include <sched.h>

#include "delayer.h"
#include "dbsysteminfo.h"

#define TIME_FWRITE_CALL	1
#define TIME_FREAD_CALL		1
// BINARY_STREAM_MODE 
// 0 == open/writev/readv/etc
// 1 == fopen/fwrite/fread/etc
#define BINARY_STREAM_MODE	1

// If BINARY_STREAM_MODE == 1, NO_BUFFERED_STREAM set file buffer size to zero
#define NO_BUFFERED_STREAM	0

// =====  Variables shared between files  =====

// pov-database repository dbstreaminterfaces.h
extern s_interface streaminterface;

// delayer_main.c
extern char video_path[VIDEO_PATH_LEN];

// ===== Global variables in this file =====
extern pthread_mutex_t mutex_disc_signal;
//extern pthread_cond_t condt_disc_signal;

// To communicate between multicast and rudp / talkback thread
extern sem_t semaphore;

static int multicast_interface_address(struct in_addr *address)
{
	const char *configured = getenv("DELAYER_MULTICAST_INTERFACE");
	address->s_addr = htonl(INADDR_ANY);
	if (configured == NULL || configured[0] == '\0')
		return 0;
	if (inet_pton(AF_INET, configured, address) != 1 ||
		address->s_addr == htonl(INADDR_ANY) ||
		address->s_addr == htonl(INADDR_BROADCAST) ||
		IN_MULTICAST(ntohl(address->s_addr)))
	{
		LOGMSG(LOG_ERR, "%s", "DELAYER_MULTICAST_INTERFACE must be a local unicast IPv4 address");
		return -1;
	}
	return 0;
}


int find_time(char *name, uint32_t file_id, uint64_t searched_time, s_pause_arg *pause)
{
	if (name == NULL || pause == NULL)
	{
		pstate = STATE_SHUTDOWN;
		syslog(LOG_CRIT, "find_time arguments are NULL");
		return 1;
	}

#if BINARY_STREAM_MODE
	FILE *fp=NULL;
#else
	int fd=0;
#endif
	char filename[VIDEO_PATH_LEN+256];
	bufferelement current_bufelement;
	int read_ret;

	snprintf(filename, VIDEO_PATH_LEN+255, "%s/%s_chunk_%u.bin",
		video_path, name, file_id);
#ifdef NODAEMON_DEBUG
	uint64_t last_time=0; // debug
	fprintf(stderr, "find_time: Open filename: %s\n",
		filename);
#endif

#if BINARY_STREAM_MODE
	fp = fopen(filename, "r");
	if (fp == NULL)
	{
		if (errno == EACCES)
		{
			LOGMSG(LOG_WARNING, "find_time: file (%s) doesnt exist!",
				filename);
			return 2;
		}
		LOGMSG(LOG_WARNING, "find_time: fopen failed: %d %s",
			errno, strerror(errno));
		return 3;
	}
#else
	fd = open(filename, O_RDONLY);
	if (fd == -1)
	{
		if (errno == EACCES)
		{
			LOGMSG(LOG_WARNING, "find_time: file (%s) doesnt exist!",
				filename);
			return 2;
		}
		LOGMSG(LOG_WARNING, "find_time: open failed: %d %s",
			errno, strerror(errno));
		return 3;
	}
#endif

	while (pstate != STATE_SHUTDOWN)
	{
		// Read first buffer

#if BINARY_STREAM_MODE
		read_ret = fread(current_bufelement.packet_buffer, PACKET_BUFFER_SIZE, 1, fp);
		if (read_ret == 0)
		{
#ifdef NODAEMON_DEBUG
			LOGMSG(LOG_WARNING, "find_time: feof: %d ferror: %d errno %d str: %s\n",
				feof(fp), ferror(fp), errno, strerror(errno));
#endif
			fclose(fp);
			return 4;
		}
#else
		// ======= UNBUFFERED =======
		read_ret = read(fd, current_bufelement.packet_buffer, PACKET_BUFFER_SIZE);
		if (read_ret == -1)
		{
			// errno describe the error
#ifdef NODAEMON_DEBUG
			LOGMSG(LOG_WARNING, "find_time: errno %d str: %s\n",
				errno, strerror(errno));
#endif
			close(fd);
			return 4;
		}
		if (read_ret == 0)
		{
			// read_ret == 0 => end of file
			// run over the cliff
			close(fd);
			return 6;
		}
#endif
		parseHeader(current_bufelement.packet_buffer, &(current_bufelement.header));
		if (current_bufelement.header.sendtime >= searched_time)
		{
			pause->pause_time = current_bufelement.header.sendtime;
#if BINARY_STREAM_MODE
			pause->pause_pos = ftell(fp);
#else
			// ======= UNBUFFERED =======
			pause->pause_pos = lseek(fd, 0, SEEK_CUR);
#endif
			if (pause->pause_pos == -1)
			{
#ifdef NODAEMON_DEBUG
				fprintf(stderr, "Failed to get position %d %s\n",
					errno, strerror(errno));
#endif
#if BINARY_STREAM_MODE
				fclose(fp);
#else
				close(fd);
#endif
				return 5;
			}
#ifdef NODAEMON_DEBUG
			fprintf(stderr, "find_time: last_time %"PRIu64" current_bufelement[%u]: %"PRIu64"\n",
				last_time, current_bufelement.header.counter, current_bufelement.header.sendtime);
#endif
			break;
		}
#ifdef NODAEMON_DEBUG
		last_time = current_bufelement.header.sendtime; // debug
#endif
	}
#if BINARY_STREAM_MODE
	if (feof(fp))
	{
		// run over the cliff
		fclose(fp);
		return 6;
	}
	fclose(fp);
#else
	close(fd);
#endif
	pause->pause_file_id = file_id;
	return 0;
}
/* The disk reader can prefetch more than one chunk ahead of the sender.
 * Resolve the packet timestamp instead of adjusting its current file by one.
 * Caller holds the pause mutex; this does not alter shared file metadata. */
int find_time_in_buffer(char *name, uint32_t oldest_id, uint32_t newest_id,
	uint64_t searched_time, s_pause_arg *pause)
{
	uint64_t low = oldest_id, high = newest_id;
	s_file_info candidate;
	while (low <= high && pstate != STATE_SHUTDOWN)
	{
		uint32_t id = (uint32_t)(low + (high - low) / 2);
		memset(&candidate, 0, sizeof(candidate));
		candidate.id = id;
		int ret = taste_file(name, &candidate);
		if (ret != 0)
			return ret;
		if (searched_time < candidate.start_time)
		{
			if (id == 0)
				break;
			high = id - 1;
		}
		else if (searched_time > candidate.end_time)
			low = (uint64_t)id + 1;
		else
			return find_time(name, id, searched_time, pause);
	}
	/* A recording gap resolves to the next available packet. */
	if (low <= newest_id)
		return find_time(name, (uint32_t)low, searched_time, pause);
	return 4;
}

// ret == 0 => Everything is ok
// ret == 1 => Inparameters are NULL
// ret == 2 => File doesnt exist - stat
// ret == 3 => File doesnt exist - EACCES
// ret == 4 => Cant open the file
// ret == 5 => Cant get the first buffer (and the starttime in the file)
// ret == 6 => Cant get the last buffer (and the last time in the file)
// ret == 7 => Cant seek to end of file
int taste_file(char *name, s_file_info *file)
{ 
	if (name == NULL || file == NULL)
	{
		pstate = STATE_SHUTDOWN;
		syslog(LOG_CRIT, "taste_file arguments are NULL");
		return 1;
	}
	file->start_time = file->end_time = 0;

#if BINARY_STREAM_MODE
	FILE *fp=NULL;
#else
	// ======= UNBUFFERED =======
	int fd = 0;
#endif
	char filename[VIDEO_PATH_LEN+256];
	bufferelement current_bufelement;
	int ret;
	struct stat stat_buf;

	snprintf(filename, VIDEO_PATH_LEN+255, "%s/%s_chunk_%u.bin",
		video_path, name, file->id);

	if (stat(filename, &stat_buf) == -1)
	{
#ifdef NODAEMON_DEBUG
		fprintf(stderr, "stat(%s) %s(%d)\n",
			filename, strerror(errno), errno);
#endif
		return 2;
	}

#if BINARY_STREAM_MODE
	fp = fopen(filename, "r");

	if (fp == NULL)
	{
		file->status = CORRUPT;
		if (errno == EACCES)
		{
			LOGMSG(LOG_WARNING, "file (%s) doesnt exist!",
				filename);
			return 3;
		}
		LOGMSG(LOG_WARNING, "fopen failed: %d %s",
			errno, strerror(errno));
		return 4;
	}
#else
	// ======= UNBUFFERED =======
	fd = open(filename, O_RDONLY);
	if (fd == -1)
	{
		file->status = CORRUPT;
		if (errno == EACCES)
		{
			LOGMSG(LOG_WARNING, "file (%s) doesnt exist!",
				filename);
			return 3;
		}
		LOGMSG(LOG_WARNING, "open failed: %d %s",
			errno, strerror(errno));
		return 4;
	}
#endif

	// Read first buffer
#if BINARY_STREAM_MODE
	ret = fread(current_bufelement.packet_buffer, PACKET_BUFFER_SIZE, 1, fp);
	if (ret == 0)
	{
		file->status = CORRUPT;
#ifdef NODAEMON_DEBUG
		fprintf(stderr, "feof: %d ferror: %d errno %d str: %s\n",
			feof(fp), ferror(fp), errno, strerror(errno));
#endif
		fclose(fp);
		return 5;
	}
#else
	// ======= UNBUFFERED =======
	ret = read(fd, current_bufelement.packet_buffer, PACKET_BUFFER_SIZE);
	if (ret <= 0)
	{
		file->status = CORRUPT;
		// errno describe the error
#ifdef NODAEMON_DEBUG
		LOGMSG(LOG_WARNING, "errno %d str: %s\n",
			errno, strerror(errno));
#endif
		close(fd);
		return 5;
	}
#endif
	parseHeader(current_bufelement.packet_buffer, &(current_bufelement.header));
	file->start_time = current_bufelement.header.sendtime;

	// TODO
	// Check the start_time to see if its reasonable "in time"

#if BINARY_STREAM_MODE
	// Seek to end of file
	ret = fseek(fp, -PACKET_BUFFER_SIZE, SEEK_END);
#else
	// Seek to end of file
	ret = lseek(fd, -PACKET_BUFFER_SIZE, SEEK_END);
#endif
	if (ret == -1)
	{
		file->status = CORRUPT;

#ifdef NODAEMON_DEBUG
		fprintf(stderr, "seek failed %d %s\n",
				errno, strerror(errno));
#endif
#if BINARY_STREAM_MODE
		fclose(fp);
#else
		close(fd);
#endif
		return 7;
	}

#if BINARY_STREAM_MODE
	// Read the last buffer from the file
	ret = fread(current_bufelement.packet_buffer, PACKET_BUFFER_SIZE, 1, fp);
	if (ret == 0)
	{
		file->status = CALCULATED_END_TIME;
		file->end_time = file->start_time+CHUNK_TIME_DIFF;
#ifdef NODAEMON_DEBUG
		fprintf(stderr, "feof: %d ferror: %d errno %d str: %s\n",
			feof(fp), ferror(fp), errno, strerror(errno));
#endif
		fclose(fp);
		return 6;
	}
	fclose(fp);
#else
	// ======= UNBUFFERED =======
	
	ret = read(fd, current_bufelement.packet_buffer, PACKET_BUFFER_SIZE);
	close(fd);
	if (ret <= 0)
	{
		file->status = CALCULATED_END_TIME;
		file->end_time = file->start_time+CHUNK_TIME_DIFF;
#ifdef NODAEMON_DEBUG
		fprintf(stderr, "errno %d str: %s\n",
			errno, strerror(errno));
#endif
		return 6;
	}
#endif

	// ------------------- file are closed -------------------

	parseHeader(current_bufelement.packet_buffer, &(current_bufelement.header));
	file->end_time = current_bufelement.header.sendtime;

	if (file->end_time > (file->start_time + CHUNK_TIME_DIFF + 1000000))
	{
		// e_time are probably screwed up because this file are probably currently written.

		file->status = CALCULATED_END_TIME;
		file->end_time = file->start_time + CHUNK_TIME_DIFF;

		// consider the file read successfully
		return 0;
	}

#if 0
	fprintf(stderr, "taste_file(%s): Start %"PRIu64" Last %"PRIu64"\n",
		filename, file->start_time, file->end_time);
#endif
	file->status = VALID;	

	return 0;
}

void *thread_disccleaner(void *arg)
{
	s_thread_disc_cleaner_arg *disccleaner_arg = (s_thread_disc_cleaner_arg *)arg;
	struct timespec disc_signal_timeout;
	struct timeval tv;
	int ret, pthread_ret=0;
	bool remove_file = false;	

	while (pstate != STATE_SHUTDOWN)
	{
#ifdef NODAEMON_DEBUG
		fprintf(stderr, "thread_disccleaner: %u\n", disccleaner_arg->oldest_file->id);
#endif
		if (disccleaner_arg->oldest_file->id == disccleaner_arg->writer_file->id)
		{
			// This happens in a new system (or cache is removed)
#ifdef NODAEMON_DEBUG
			fprintf(stderr, "NEW OLDEST TIME: %"PRIu64"\n",
				disccleaner_arg->writer_file->start_time);
#endif
			s_file_info tmpinfo;
			pthread_ret = pthread_mutex_lock(&disccleaner_arg->writer_file->mutex_file);
			LOG_PTHREAD_ERROR_BREAK(pthread_ret, disccleaner_arg->name, __LINE__);
			tmpinfo.id = disccleaner_arg->writer_file->id;
			tmpinfo.start_time = disccleaner_arg->writer_file->start_time;
			tmpinfo.end_time = disccleaner_arg->writer_file->end_time;
			tmpinfo.status = disccleaner_arg->writer_file->status;
			pthread_ret = pthread_mutex_unlock(&disccleaner_arg->writer_file->mutex_file);
			LOG_PTHREAD_ERROR_BREAK(pthread_ret, disccleaner_arg->name, __LINE__);

			pthread_ret = pthread_mutex_lock(&disccleaner_arg->oldest_file->mutex_file);
			LOG_PTHREAD_ERROR_BREAK(pthread_ret, disccleaner_arg->name, __LINE__);
			disccleaner_arg->oldest_file->start_time = tmpinfo.start_time;
			disccleaner_arg->oldest_file->end_time = tmpinfo.end_time;
			disccleaner_arg->oldest_file->status = tmpinfo.status;
			pthread_ret = pthread_mutex_unlock(&disccleaner_arg->oldest_file->mutex_file);
			LOG_PTHREAD_ERROR_BREAK(pthread_ret, disccleaner_arg->name, __LINE__);

			// Wait for the writer file to not use the same ID as oldest file - but still want to update end_time
			sleep(1);
			continue;
		}


		// ========= TASTE FILE TO GET START_TIME / END_TIME =============
#ifdef NODAEMON_DEBUG
		fprintf(stderr, "taste file\n");
#endif
		pthread_ret = pthread_mutex_lock(&disccleaner_arg->oldest_file->mutex_file);
		LOG_PTHREAD_ERROR_BREAK(pthread_ret, disccleaner_arg->name, __LINE__);
		ret = taste_file(disccleaner_arg->name, disccleaner_arg->oldest_file);

		// OBS writer_file->id
		if ((ret == 2 || ret == 3) && (disccleaner_arg->oldest_file->id < disccleaner_arg->writer_file->id))
		{
#ifdef NODAEMON_DEBUG
			fprintf(stderr, "oldest_file (%d) doesnt exist, step forward to next file\n",
				disccleaner_arg->oldest_file->id);
#endif
			disccleaner_arg->oldest_file->id++;
			disccleaner_arg->oldest_file->start_time = disccleaner_arg->oldest_file->end_time = 0;
			disccleaner_arg->oldest_file->status = UNPARSED;
			pthread_ret = pthread_mutex_unlock(&disccleaner_arg->oldest_file->mutex_file);
			LOG_PTHREAD_ERROR_BREAK(pthread_ret, disccleaner_arg->name, __LINE__);
			continue;
		}
		pthread_ret = pthread_mutex_unlock(&disccleaner_arg->oldest_file->mutex_file);
		LOG_PTHREAD_ERROR_BREAK(pthread_ret, disccleaner_arg->name, __LINE__);

		remove_file = false;
		if (ret != 0)
		{
			LOGMSG(LOG_CRIT, "%s: taste _new_ UNPARSED oldest file failed!",
				disccleaner_arg->name);
#ifdef NODAEMON_DEBUG
			fprintf(stderr, "%s: taste _new_ UNPARSED oldest file (id: %d) failed! ret: %d\n",
				disccleaner_arg->name, disccleaner_arg->oldest_file->id, ret);
#endif
			remove_file = true;
		}

		// ============= CHECK IF TIME TO REMOVE THIS FILE ===============
		gettimeofday(&tv, NULL);
		if ((tv.tv_sec - *disccleaner_arg->total_disc_buffer_time)*1000000ULL > disccleaner_arg->oldest_file->end_time)
		{
			remove_file = true;
		}


		// ============ WAIT FOR RIGHT TIME TO REMOVE FILE ===================
		if (remove_file == false)
		{
#if 0
			fprintf(stderr, "====tv_sec %llu total_disc_buffer_time %llu > end_time %"PRIu64"  ? %d\n",
				tv.tv_sec*1000000ULL, *disccleaner_arg->total_disc_buffer_time *1000000ULL,
				disccleaner_arg->oldest_file->end_time,
				((tv.tv_sec - *disccleaner_arg->total_disc_buffer_time)*1000000ULL > disccleaner_arg->oldest_file->end_time));
#endif


			// for example
			// endtime are 14:00
			// now = 15:00
			// buffersize = 2h
			// now - buffersize = 13:00 - everything prior to 13:00 will be removed
			// but the oldest time in the interface are 14:00 so we need to wait 14:00 - 13:00 = 1h

#ifdef NODAEMON_DEBUG
			uint64_t diff;

			diff = disccleaner_arg->oldest_file->end_time/1000000ULL
				- tv.tv_sec
				+ *disccleaner_arg->total_disc_buffer_time
				+ CHUNK_TIME_DIFF/1000000;
			fprintf(stderr, "thread_disccleaner: WAIT diff %"PRIu64" s\n", diff);

			// disc_signal_timeout.tv_sec = tv.tv_sec + diff;
			// but diff includes -tv.tv_sec and we can remove it from the calculation
#endif

			disc_signal_timeout.tv_sec = disccleaner_arg->oldest_file->end_time/1000000ULL
						+ *disccleaner_arg->total_disc_buffer_time
						+ CHUNK_TIME_DIFF/1000000;

			disc_signal_timeout.tv_nsec = 0;

			pthread_ret = pthread_mutex_lock(disccleaner_arg->mutex_discclean_signal);
			LOG_PTHREAD_ERROR_BREAK(pthread_ret, disccleaner_arg->name, __LINE__);

			pthread_ret = pthread_cond_timedwait(disccleaner_arg->condt_discclean_signal,
								disccleaner_arg->mutex_discclean_signal,
								&disc_signal_timeout);
			if (pthread_ret != 0 && pthread_ret != ETIMEDOUT)
			{
				LOGMSG(LOG_WARNING, "%s: pthread_cond_timedwait failed %d EINVAL %d ETIMEDOUT %d EPERM %d",
						disccleaner_arg->name, pthread_ret,
						pthread_ret == EINVAL, pthread_ret == ETIMEDOUT, pthread_ret == EPERM);
				break;
			}
			pthread_ret = pthread_mutex_unlock(disccleaner_arg->mutex_discclean_signal);
			LOG_PTHREAD_ERROR_BREAK(pthread_ret, disccleaner_arg->name, __LINE__);

			// ============ CHECK SO ITS TIME TO REMOVE FILE - Can be interrupted by an exit signal ==========
			gettimeofday(&tv, NULL);
			if ((tv.tv_sec - *disccleaner_arg->total_disc_buffer_time)*1000000ULL > disccleaner_arg->oldest_file->end_time)
			{
				remove_file = true;
			}
		}

		// ============ REMOVE FILE ================
		if (remove_file)
		{
			char filename[VIDEO_PATH_LEN+256];
			snprintf(filename, VIDEO_PATH_LEN+255, "%s/%s_chunk_%u.bin",
				video_path, disccleaner_arg->name, disccleaner_arg->oldest_file->id);
#ifdef NODAEMON_DEBUG
			fprintf(stderr, "remove file %s\n", filename);
#endif
			unlink(filename);
#if 0
			fprintf(stderr, "disccleaner: reader_file->id %u oldest_file->id %u\n",
				disccleaner_arg->reader_file->id, disccleaner_arg->oldest_file->id);
#endif
			pthread_ret = pthread_mutex_lock(&disccleaner_arg->oldest_file->mutex_file);
			LOG_PTHREAD_ERROR_BREAK(pthread_ret, disccleaner_arg->name, __LINE__);
			disccleaner_arg->oldest_file->id++;
			disccleaner_arg->oldest_file->start_time = disccleaner_arg->oldest_file->end_time = 0;
			disccleaner_arg->oldest_file->status = UNPARSED;
			pthread_ret = pthread_mutex_unlock(&disccleaner_arg->oldest_file->mutex_file);
			LOG_PTHREAD_ERROR_BREAK(pthread_ret, disccleaner_arg->name, __LINE__);
			continue;
		}
	}

	pstate = STATE_SHUTDOWN;

	LOGMSG(LOG_NOTICE, "%s: EXIT thread_disccleaner",
		disccleaner_arg->name);

	return NULL;
}

// From rudp_transmitter.c
//void *thread_tsreceiver(void *arg)
void *thread_mc_receiver(void *arg)
{
	//mc_receiver_arg.multicast_address
	//mc_receiver_arg.multicast_port
	//mc_receiver_arg.multicast_ttl
	//mc_receiver_arg.buffer_timeout


	thread_mc_receiver_arg *mc_receiver_arg = (thread_mc_receiver_arg *)arg;
	int fd_mc_input, pthread_ret;
	struct sockaddr_in sockaddr_mc_input;
	socklen_t addrlen;
	struct ip_mreq mreq;
	struct in_addr multicast_interface;
	bufferelement *current_bufelement=NULL;
	uint32_t current_packet_counter = 1;
	uint32_t dropped;
	ssize_t recv_bytes=0;
	if (multicast_interface_address(&multicast_interface) != 0)
	{
		pstate = STATE_SHUTDOWN;
		return NULL;
	}

#if 1
	fd_set fdset;
	sigset_t pselect_set;
	struct timespec tp_select;
	int res_select;
#endif

	int optval, retval, getsize;
	socklen_t len = sizeof(int);

#ifdef TRYLOCK
	int tryCount = 0;
#endif

#ifdef __linux__
	int sid = syscall(SYS_gettid);
	syslog(LOG_NOTICE, "%s: Starting ts thread SID %d",
		streaminterface.name, sid);
#endif
	while (pstate != STATE_SHUTDOWN)
	{
		syslog(LOG_NOTICE, "%s: TS mainloop",
			streaminterface.name);
#ifdef NODAEMON_DEBUG
		LOGMSG(LOG_NOTICE, "Setting up input socket %s:%d\n",
				mc_receiver_arg->multicast_address,
				mc_receiver_arg->multicast_port);
#endif
		if ((fd_mc_input=socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		{
			add_trap_to_queue(TRAP_DELAYER_FAILED_SETUP, "Failed to setup video input socket");

			LOGMSG(LOG_ERR, "%s: multicast socket failure",
				streaminterface.name);
			pstate = STATE_SHUTDOWN;
			continue;
		}

#if 0
		for (int setsize = 10524287; getsize < setsize; setsize -= 32768)
		{
			retval = setsockopt(fd_mc_input, SOL_SOCKET, SO_RCVBUF, (char*)&setsize, len);
			if (retval != 0)
			{
				LOGMSG(LOG_WARNING, "%s: setsockopt %s (%d)",
					mc_receiver_arg->name, strerror(errno), errno);
				break;
			}

			retval = getsockopt(fd_mc_input, SOL_SOCKET, SO_RCVBUF, (char*)&getsize, &len);
			if (retval != 0)
			{
				LOGMSG(LOG_WARNING, "%s: getsockopt %s (%d)",
					mc_receiver_arg->name, strerror(errno), errno);
				break;
			}
		}
#else
		retval = getsockopt(fd_mc_input, SOL_SOCKET, SO_RCVBUF, (char*)&getsize, &len);
		if (retval != 0)
		{
			LOGMSG(LOG_WARNING, "%s: getsockopt(fd_mc_input) %s (%d)",
				mc_receiver_arg->name, strerror(errno), errno);
		}
#endif

		LOGMSG(LOG_NOTICE, "%s: fd_mc_input SO_RCVBUF=%d",
			mc_receiver_arg->name, getsize);

		memset(&sockaddr_mc_input, 0, sizeof(struct sockaddr_in));
		sockaddr_mc_input.sin_family = AF_INET;
		sockaddr_mc_input.sin_addr.s_addr = inet_addr(mc_receiver_arg->multicast_address);
		sockaddr_mc_input.sin_port = htons(mc_receiver_arg->multicast_port);
#ifdef NODAEMON_DEBUG
		LOGMSG(LOG_NOTICE, "multicast address: %s:%d\n",
				mc_receiver_arg->multicast_address,
				mc_receiver_arg->multicast_port);
#endif

		optval = 1;
		if (setsockopt(fd_mc_input, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
		{
			syslog(LOG_WARNING, "%s: setsockopt failed errno: %d %s",
				streaminterface.name, errno, strerror(errno));
			close(fd_mc_input);
			continue;
		}

		syslog(LOG_NOTICE, "%s: binding to input %s:%d",
				streaminterface.name,
				mc_receiver_arg->multicast_address,
				mc_receiver_arg->multicast_port);

		int bindret = bind(fd_mc_input, (struct sockaddr *) &sockaddr_mc_input, sizeof(struct sockaddr_in));
		if (bindret == -1)
		{
			LOGMSG(LOG_WARNING, "%s: bind to input address failed: %s",
				streaminterface.name, strerror(errno));
			close(fd_mc_input);
			continue;
		}
		syslog(LOG_NOTICE, "%s: binding input done",
			streaminterface.name);
		mreq.imr_multiaddr.s_addr = inet_addr(mc_receiver_arg->multicast_address);
		mreq.imr_interface = multicast_interface;
		if (multicast_interface.s_addr != htonl(INADDR_ANY))
		{
#ifdef IP_MULTICAST_ALL
			// Receive only memberships joined by this socket on the selected interface.
			optval = 0;
			if (setsockopt(fd_mc_input, IPPROTO_IP, IP_MULTICAST_ALL, &optval, sizeof(optval)) == -1)
			{
				LOGMSG(LOG_ERR, "%s: cannot restrict multicast input: %s", streaminterface.name, strerror(errno));
				close(fd_mc_input);
				pstate = STATE_SHUTDOWN;
				return NULL;
			}
#endif
			LOGMSG(LOG_NOTICE, "%s: multicast input interface %s", streaminterface.name, getenv("DELAYER_MULTICAST_INTERFACE"));
		}

		syslog(LOG_NOTICE, "%s: adding membership",
			streaminterface.name);

		if (setsockopt(fd_mc_input, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) == -1)
		{
			if (multicast_interface.s_addr != htonl(INADDR_ANY) && IN_MULTICAST(ntohl(mreq.imr_multiaddr.s_addr)))
			{
				LOGMSG(LOG_ERR, "%s: cannot join multicast on selected interface: %s", streaminterface.name, strerror(errno));
				close(fd_mc_input);
				pstate = STATE_SHUTDOWN;
				return NULL;
			}
			LOGMSG(LOG_WARNING, "%s: Probably unicast - setsockopt failed errno: %d %s",
				streaminterface.name, errno, strerror(errno));
		}

		addrlen=sizeof(struct sockaddr_in);
		syslog(LOG_NOTICE, "%s: Starting ts receiver loop",
			streaminterface.name);
		int count_missed_acquire_buffer=0;
#if 0
		uint16_t old_sequence=0;
#endif
		while (pstate != STATE_SHUTDOWN)
		{
#ifdef TRYLOCK
			tryCount = 0;
			do {
				pthread_ret = pthread_mutex_trylock(&mc_receiver_arg->buffer_q_available[mc_receiver_arg->no_sink].mutex);
				tryCount++;
				if (pthread_ret == EBUSY)
					sched_yield();
			} while (pthread_ret == EBUSY);
			if (tryCount > 5)
			{
				fprintf(stderr, "thread_mc_receiver: buffer_q_available #1 busy %d times\n", tryCount);
			}
#else
			pthread_ret = pthread_mutex_lock(&mc_receiver_arg->buffer_q_available[mc_receiver_arg->no_sink].mutex);
			LOG_PTHREAD_ERROR_BREAK(pthread_ret, mc_receiver_arg->name, __LINE__);
#endif
			current_bufelement = getAvailableBufferElement(&mc_receiver_arg->buffer_q_available[mc_receiver_arg->no_sink].buffer_elements);
			if (current_bufelement != NULL)
			{
				// thread_mc_receiver
				mc_receiver_arg->buffer_q_available[mc_receiver_arg->no_sink].no_buffer_elements--;
			}
			pthread_ret = pthread_mutex_unlock(&mc_receiver_arg->buffer_q_available[mc_receiver_arg->no_sink].mutex);
			LOG_PTHREAD_ERROR_BREAK(pthread_ret, mc_receiver_arg->name, __LINE__);

			if (current_bufelement == NULL)
			{
				count_missed_acquire_buffer++;

				LOGMSG(LOG_WARNING, "%s: thread_mc_receiver current_bufelement == NULL A %d D %d\n",
					mc_receiver_arg->name,
					mc_receiver_arg->buffer_q_available[mc_receiver_arg->no_sink].no_buffer_elements,
					mc_receiver_arg->buffer_q_disc->no_buffer_elements);

				// Second time give up and shutdown!
				if (count_missed_acquire_buffer > 1)
				{
					add_trap_to_queue(TRAP_DELAYER_LOST_INPUT, "Input video buffer exhausted");
					close(fd_mc_input);

					logtodbsys(mc_receiver_arg->mutex_dbconn, *mc_receiver_arg->dbconn,
						streaminterface.id, POVLOGLEVEL_ERR,
						"%s: mc_receiver buffer exhausted!",
						streaminterface.name);

					pstate = STATE_SHUTDOWN;
					continue;
				}

				// Switch thread and let any other thread do something to get rid of a packet!
				// Maybe we loose a packet or two but its more important to not shutdown.
				sched_yield();
				continue;
			}
			count_missed_acquire_buffer = 0;

			do
			{
				// Start blocking socket fix
				FD_ZERO(&fdset);
				FD_SET(fd_mc_input, &fdset);
				tp_select.tv_sec=1;
				tp_select.tv_nsec=0;
				sigemptyset(&pselect_set);
				sigaddset(&pselect_set, SIGTERM);
				sigaddset(&pselect_set, SIGINT);

				res_select = pselect(fd_mc_input+1,&fdset, NULL, NULL, &tp_select, &pselect_set);
			// if res_select == 0 timeout occurred
			//  just loop and wait another timeout - no need to release the buffer and acquire it again
			//  unless we are in shutdown mode.
			} while (res_select == 0 && pstate != STATE_SHUTDOWN);

			if (pstate == STATE_SHUTDOWN)
			{
				continue;
			}

			recv_bytes = recvfrom(fd_mc_input, &(current_bufelement->packet_buffer[RUDP_HEADER_SIZE]),
						PKTSIZE + RTP_MIN_HEADER_SIZE, 0,
						(struct sockaddr *) &sockaddr_mc_input, &addrlen);

			if (recv_bytes == -1)
			{
				add_trap_to_queue(TRAP_DELAYER_LOST_INPUT, "Error receiving");
				LOGMSG(LOG_WARNING, "%s: Error receiving %s",
					streaminterface.name, strerror(errno));
#ifdef NODAEMON_DEBUG
				LOGMSG(LOG_NOTICE, "errno: %d %s\n", errno, strerror(errno));
#endif
#ifdef TRYLOCK
				tryCount = 0;
				do {
					pthread_ret = pthread_mutex_trylock(&mc_receiver_arg->buffer_q_available[mc_receiver_arg->no_sink].mutex);
					tryCount++;
					if (pthread_ret == EBUSY)
						sched_yield();
				} while (pthread_ret == EBUSY);
				if (tryCount > 5)
				{
					fprintf(stderr, "thread_mc_receiver: buffer_q_available #3 busy %d times\n", tryCount);
				}
#else
				pthread_ret = pthread_mutex_lock(&mc_receiver_arg->buffer_q_available[mc_receiver_arg->no_sink].mutex);
				LOG_PTHREAD_ERROR_BREAK(pthread_ret, mc_receiver_arg->name, __LINE__);
#endif
				returnBufferElement(&mc_receiver_arg->buffer_q_available[mc_receiver_arg->no_sink].buffer_elements, current_bufelement, 1);
				// thread_mc_receiver
				mc_receiver_arg->buffer_q_available[mc_receiver_arg->no_sink].no_buffer_elements++;
				pthread_ret = pthread_mutex_unlock(&mc_receiver_arg->buffer_q_available[mc_receiver_arg->no_sink].mutex);
				LOG_PTHREAD_ERROR_BREAK(pthread_ret, mc_receiver_arg->name, __LINE__);
				close(fd_mc_input);
				break;
			}

			//if not mpeg-ts sync byte, assume rtp
			if (current_bufelement->packet_buffer[RUDP_HEADER_SIZE] != 0x47)
			{
				current_bufelement->rtp = 1;
			}

#if 0
			struct raw_rtp_packet *packet = (struct raw_rtp_packet *)(current_bufelement->packet_buffer+RUDP_HEADER_SIZE);
			uint16_t new_sequence = ntohs(packet->sequence_number);
			if (old_sequence+1 != new_sequence && new_sequence != 0)
			{
				LOGMSG(LOG_WARNING, "%s: old_sequence %u new_sequence %u\n",
					streaminterface.name, old_sequence, new_sequence);
			}
			old_sequence = new_sequence;
#endif

			// Parse header
			current_bufelement->header.sendtime = addHeader(current_bufelement->packet_buffer, RUDP_PACK_DATA, 0, recv_bytes, 0, current_packet_counter);
			current_bufelement->header.typeofpacket = RUDP_PACK_DATA;
			current_bufelement->header.payloadlength = recv_bytes;
			current_bufelement->header.counter = current_packet_counter;

			if (current_packet_counter > MAX_PACKET_COUNTER)
			{
				current_packet_counter = 1;
			}

			// --------- LIVE COPY --------------------
#if 1
			// Original version
			// Bellow are a trylock version which jumps over if the lock cant be acquired
#if 1
			if (mc_receiver_arg->state[mc_receiver_arg->no_sink] == DELAYER_STATE_LIVESTREAMING)
			{
				// mc_receiver_arg->no_sink include the disc buffer - need to cleanup and dont confuse.
				for (int i = 0; i < (mc_receiver_arg->no_sink) && pstate != STATE_SHUTDOWN; i++)
				{
					// ==== IF this path are not in livestreaming - DONT copy buffer!!
					if (mc_receiver_arg->state[i] != DELAYER_STATE_LIVESTREAMING)
					{
						continue;
					}
					if (mc_receiver_arg->buffer_q_available[i].buffer_elements == NULL)
					{
						continue;
					}

					// Copy bufelement to sendqueue
					bufferelement *send_bufelement;
#ifdef TRYLOCK
					tryCount = 0;
					do {
						pthread_ret = pthread_mutex_trylock(&mc_receiver_arg->buffer_q_available[i].mutex);
						tryCount++;
						if (pthread_ret == EBUSY)
							sched_yield();

					} while (pthread_ret == EBUSY);
					if (tryCount > 5)
					{
						fprintf(stderr, "thread_mc_receiver: availible[%d] busy %d times\n", i, tryCount);
					}
#else
					pthread_ret = pthread_mutex_lock(&mc_receiver_arg->buffer_q_available[i].mutex);
					LOG_PTHREAD_ERROR_BREAK(pthread_ret, mc_receiver_arg->name, __LINE__);
#endif
					send_bufelement = getAvailableBufferElement(&mc_receiver_arg->buffer_q_available[i].buffer_elements);
					if (send_bufelement != NULL)
					{
						mc_receiver_arg->buffer_q_available[i].no_buffer_elements--;
					}
					pthread_ret = pthread_mutex_unlock(&mc_receiver_arg->buffer_q_available[i].mutex);
					LOG_PTHREAD_ERROR_BREAK(pthread_ret, mc_receiver_arg->name, __LINE__);

					if (send_bufelement == NULL)
					{
						// if no buffer are availible for this path - just skip it and move on to next path.
						continue;
					}

					// ----- GOT AN VALID BUFFER TO THE PATH -----

					// ----- COPY PACKET --------
					memcpy(send_bufelement, current_bufelement, sizeof(bufferelement));
					send_bufelement->prev = NULL;
					send_bufelement->last = NULL;
					send_bufelement->next = NULL;

					// ------ PUT IN SEND Q ------
#ifdef TRYLOCK
					tryCount = 0;
					do {
						pthread_ret = pthread_mutex_trylock(&mc_receiver_arg->buffer_q_send[i].mutex);
						tryCount++;
						if (pthread_ret == EBUSY)
							sched_yield();
					} while (pthread_ret == EBUSY);
					if (tryCount > 5)
					{
						fprintf(stderr, "thread_mc_receiver: buffer_q_send[%d] busy %d times\n", i, tryCount);
					}
#else
					pthread_ret = pthread_mutex_lock(&mc_receiver_arg->buffer_q_send[i].mutex);
					LOG_PTHREAD_ERROR_BREAK(pthread_ret, mc_receiver_arg->name, __LINE__);
#endif
					putInBuffer(&mc_receiver_arg->buffer_q_send[i].buffer_elements, send_bufelement);
					mc_receiver_arg->buffer_q_send[i].no_buffer_elements++;
					pthread_ret = pthread_mutex_unlock(&mc_receiver_arg->buffer_q_send[i].mutex);
					LOG_PTHREAD_ERROR_BREAK(pthread_ret, mc_receiver_arg->name, __LINE__);
				}
			}
#else
			// Some of the path(s) are in live mode
			if (mc_receiver_arg->state[mc_receiver_arg->no_sink] == DELAYER_STATE_LIVESTREAMING)
			{
				int num_output_path = mc_receiver_arg->no_sink;
				if (num_output_path < 0)
					LOGMSG(LOG_WARNING, "%s no_sink-1 = %d",
						mc_receiver_arg->name, mc_receiver_arg->no_sink);

				bufferelement *avail[num_output_path];
				int avail_try_count[num_output_path];
				int send_try_count[num_output_path];
				bool error_avail_lock = false, error_send_lock;

				memset(avail, 0, num_output_path*sizeof(avail[0]));
				memset(avail_try_count, 0, num_output_path*sizeof(int));
				memset(send_try_count, 0, num_output_path*sizeof(int));
				do {
					error_avail_lock = false;
					for (int i = 0; i < num_output_path; i++)
					{
						// ==== IF this path are not in livestreaming - DONT copy buffer!!
						if (mc_receiver_arg->state[i] != DELAYER_STATE_LIVESTREAMING)
						{
							continue;
						}

						if (avail[i] != NULL)
						{
							continue;
						}

						avail_try_count[i]++;
						pthread_ret = pthread_mutex_trylock(&mc_receiver_arg->buffer_q_available[i].mutex);
						if (pthread_ret == EBUSY)
						{
							error_avail_lock = true;
							continue;
						}
						avail[i] = getAvailableBufferElement(&mc_receiver_arg->buffer_q_available[i].buffer_elements);
						if (avail[i] != NULL)
						{
							mc_receiver_arg->buffer_q_available[i].no_buffer_elements--;
						}
						pthread_mutex_unlock(&mc_receiver_arg->buffer_q_available[i].mutex);

						if (avail[i] == NULL)
						{
							// if no buffer are availible for this path - just skip it and move on to next path.
							error_avail_lock = true;
							continue;
						}

						memcpy(avail[i], current_bufelement, sizeof(bufferelement));
						avail[i]->prev = NULL;
						avail[i]->last = NULL;
						avail[i]->next = NULL;

					}
					if (error_avail_lock)
						//usleep(1);
						sched_yield();
				} while (error_avail_lock);

				// ----- GOT AN VALID BUFFER TO THE PATH -----

#if 0
				// ----- COPY PACKET --------
				for (int i = 0; i < num_output_path; i++)
				{
					memcpy(avail[i], current_bufelement, sizeof(bufferelement));
					avail[i]->prev = NULL;
					avail[i]->last = NULL;
					avail[i]->next = NULL;
				}
#endif
				// ------ PUT IN SEND Q ------
				do {
					error_send_lock = false;
					for (int i = 0; i < num_output_path; i++)
					{
						// ==== IF this path are not in livestreaming - DONT copy buffer!!
						if (mc_receiver_arg->state[i] != DELAYER_STATE_LIVESTREAMING)
						{
							continue;
						}

						if (avail[i] == NULL)
						{
							continue;
						}

						send_try_count[i]++;
						pthread_ret = pthread_mutex_trylock(&mc_receiver_arg->buffer_q_send[i].mutex);
						if (pthread_ret == EBUSY)
						{
							error_send_lock = true;
							continue;
						}
						putInBuffer(&mc_receiver_arg->buffer_q_send[i].buffer_elements, avail[i]);
						mc_receiver_arg->buffer_q_send[i].no_buffer_elements++;
						pthread_mutex_unlock(&mc_receiver_arg->buffer_q_send[i].mutex);
						avail[i] = NULL;
					}
					if (error_send_lock)
						sched_yield();
				} while (error_send_lock);

				for (int i = 0; i < num_output_path; i++)
				{
					if (avail_try_count[i] > 1)
						fprintf(stderr, "thread_mc_receiver: Avail[%d] %d\n", i, avail_try_count[i]);
					if (send_try_count[i] > 1)
						fprintf(stderr, "thread_mc_receiver: send[%d] %d\n", i, send_try_count[i]);
				}
			}
#endif
#endif

			// ------- put element in disc buffer -----
#ifdef TRYLOCK
			tryCount = 0;
			do {
				pthread_ret = pthread_mutex_trylock(&mc_receiver_arg->buffer_q_disc->mutex);
				tryCount++;
				if (pthread_ret == EBUSY)
					sched_yield();
			} while (pthread_ret == EBUSY);
			if (tryCount > 5)
			{
				fprintf(stderr, "thread_mc_receiver: buffer_q_disc busy %d times\n", tryCount);
			}
#else
			pthread_ret = pthread_mutex_lock(&mc_receiver_arg->buffer_q_disc->mutex);
			LOG_PTHREAD_ERROR_BREAK(pthread_ret, mc_receiver_arg->name, __LINE__);
#endif

			putInBuffer(&mc_receiver_arg->buffer_q_disc->buffer_elements, current_bufelement);

#if 0
			addHeaderAndPutInBuffer(&mc_receiver_arg->buffer_q_disc->buffer_elements, current_bufelement, RUDP_PACK_DATA,
					0, recv_bytes, 0, &current_packet_counter);
#endif

			// thread_mc_receiver
			mc_receiver_arg->buffer_q_disc->no_buffer_elements++;
			pthread_ret = pthread_mutex_unlock(&mc_receiver_arg->buffer_q_disc->mutex);
			LOG_PTHREAD_ERROR_BREAK(pthread_ret, mc_receiver_arg->name, __LINE__);

			// Increment semaphore to get it off from blocking state and deliver the packet over rudp path
			sem_post(&semaphore);

			dropped = stat_handle_packet_rtp_get_dropped(mc_receiver_arg->rudp_sequence_list, current_bufelement->packet_buffer, recv_bytes);

#ifdef TRYLOCK
			tryCount = 0;
			do {
				pthread_ret = pthread_mutex_trylock(mc_receiver_arg->mutex_stat);
				tryCount++;
				if (pthread_ret == EBUSY)
					sched_yield();
			} while (pthread_ret == EBUSY);
			if (tryCount > 5)
			{
				fprintf(stderr, "thread_mc_receiver: mutex_stat busy %d times\n", tryCount);
			}
#else
			pthread_ret = pthread_mutex_lock(mc_receiver_arg->mutex_stat);
			LOG_PTHREAD_ERROR_BREAK(pthread_ret, mc_receiver_arg->name, __LINE__);
#endif

			mc_receiver_arg->rudp_sequence_list->transfered_bytes_mc_udp += recv_bytes;
			mc_receiver_arg->rudp_sequence_list->dropped_10_sec += dropped;
			mc_receiver_arg->rudp_sequence_list->dropped_1_sec += dropped;
			mc_receiver_arg->rt_stats->stat_inbytes += recv_bytes;

			pthread_ret = pthread_mutex_unlock(mc_receiver_arg->mutex_stat);
			LOG_PTHREAD_ERROR_BREAK(pthread_ret, mc_receiver_arg->name, __LINE__);
		} // end receiver
		if (current_bufelement != NULL)
		{
#ifdef TRYLOCK
			tryCount = 0;
			do {
				pthread_ret = pthread_mutex_trylock(&mc_receiver_arg->buffer_q_available[mc_receiver_arg->no_sink].mutex);
				tryCount++;
				if (pthread_ret == EBUSY)
					sched_yield();
			} while (pthread_ret == EBUSY);
			if (tryCount > 5)
			{
				fprintf(stderr, "thread_mc_receiver:  buffer_q_available #2 busy %d times\n", tryCount);
			}
#else
			pthread_ret = pthread_mutex_lock(&mc_receiver_arg->buffer_q_available[mc_receiver_arg->no_sink].mutex);
			LOG_PTHREAD_ERROR_BREAK(pthread_ret, mc_receiver_arg->name, __LINE__);
#endif
			returnBufferElement(&mc_receiver_arg->buffer_q_available[mc_receiver_arg->no_sink].buffer_elements, current_bufelement, 1);
			// thread_mc_receiver
			mc_receiver_arg->buffer_q_available[mc_receiver_arg->no_sink].no_buffer_elements++;
			pthread_ret = pthread_mutex_unlock(&mc_receiver_arg->buffer_q_available[mc_receiver_arg->no_sink].mutex);
			LOG_PTHREAD_ERROR_BREAK(pthread_ret, mc_receiver_arg->name, __LINE__);
#ifdef NODAEMON_DEBUG
			if (pstate == STATE_SHUTDOWN)
			{
				LOGMSG(LOG_WARNING, "%s: pstate == STATE_SHUTDOWN after pselect",
					streaminterface.name);
			}
#endif
		}
	} // end setup socket

	pstate = STATE_SHUTDOWN;
	close(fd_mc_input);
	LOGMSG(LOG_NOTICE, "%s: EXIT thread_mc_receiver",
		streaminterface.name);

	return NULL;
}

// For example:
// if we know the first file has chunk ID = 0
// and we know the starttime are 1448037260
// we also know how long the CHUNK time are like 10s
// so approximatley we know in which file to look in
// *problem if we dont receive any multicast ?

void *thread_discwriter(void *arg)
{
#if BINARY_STREAM_MODE
	FILE *fp = NULL;
#else
	int fd = 0;
#endif
	char filename[VIDEO_PATH_LEN+512];
	size_t ret;
#if NO_BUFFERED_STREAM
	int ret_val;
#endif
	uint64_t time_diff=0;
	// stat variable in a particular file
	uint64_t first_time_in_file;
	uint64_t last_time_in_file;
#ifdef NODAEMON_DEBUG
	uint32_t last_written_counter;
	uint32_t first_written_counter;
#endif
	// time in this session
	bufferelement *current_bufelement;
	thread_disc_writer_arg *writer_arg = (thread_disc_writer_arg *) arg;
	int pthread_ret = 0;

#ifdef TRYLOCK
	int tryCount = 0;
#endif

#if TIME_FWRITE_CALL
	struct timeval before_fwrite, after_fwrite;
	double elapsedTime;
#endif


	while (pstate != STATE_SHUTDOWN)
	{
		snprintf(filename, VIDEO_PATH_LEN+511, "%s/%s_chunk_%u.bin",
			video_path, streaminterface.name, writer_arg->writer_file->id);
		LOGMSG(LOG_NOTICE, "thread_discwriter: %s\n",
			filename);

#ifdef NODAEMON_DEBUG
		fprintf(stderr, "==== thread_discwriter: %s =====\n",
				filename);
#endif
#if BINARY_STREAM_MODE
		fp = fopen(filename, "w+");
		if (fp == NULL)
		{
			LOGMSG(LOG_WARNING, "%s: fopen failed: %d %s",
				streaminterface.name, errno, strerror(errno));
			sleep(1);
			continue;
		}
#if NO_BUFFERED_STREAM
		ret_val = setvbuf(fp, NULL, _IONBF, 0);
		if (ret_val != 0)
		{
			LOGMSG(LOG_WARNING, "%s: thread_discwriter: Cant set unbuffered stream",
				streaminterface.name);
		}
#endif
#else
		// ======= UNBUFFERED =======
		fd = open(filename, O_CREAT|O_WRONLY);
		if (fd == -1)
		{
			LOGMSG(LOG_WARNING, "%s: open failed: %d %s",
				streaminterface.name, errno, strerror(errno));
			sleep(1);
			continue;
		}
#endif

		time_diff = 0;
		first_time_in_file = 0;
#ifdef NODAEMON_DEBUG
		last_written_counter = MAX_PACKET_COUNTER+1;
		first_written_counter = 0;
#endif
	//	no_retry_chop = 0;

		while (time_diff < CHUNK_TIME_DIFF && pstate != STATE_SHUTDOWN)
		{
			if (writer_arg->buffer_q_disc->buffer_elements == NULL)
			{
				// need to bee kicked at exit
				int sval;
				int ret = sem_getvalue(&semaphore, &sval);

				if (ret != 0 && sval > 2)
				{
#ifdef NODAEMON_DEBUG
					fprintf(stderr, "sem_getvalue: %d ret %d\n",
						sval, ret);
#endif
					for (; sval > 0; sval--)
					{
						ret = sem_trywait(&semaphore);
					}
				}

				sem_wait(&semaphore);
				continue;
			}

#ifdef TRYLOCK
			tryCount = 0;
			do {
				pthread_ret = pthread_mutex_trylock(&writer_arg->buffer_q_disc->mutex);
				tryCount++;
				if (pthread_ret == EBUSY)
					sched_yield();
			} while (pthread_ret == EBUSY);
			if (tryCount > 5)
			{
				fprintf(stderr, "thread_discwriter: q_disc busy %d times\n", tryCount);
			}
#else
			pthread_ret = pthread_mutex_lock(&writer_arg->buffer_q_disc->mutex);
			LOG_PTHREAD_ERROR_BREAK(pthread_ret, writer_arg->name, __LINE__);
#endif

			// Otherwise see if a head is availble to write to disk
			current_bufelement = chop_head_of_buffer(&writer_arg->buffer_q_disc->buffer_elements);
			if (current_bufelement != NULL)
			{
				// thread_discwriter
				writer_arg->buffer_q_disc->no_buffer_elements--;
			}
			pthread_ret = pthread_mutex_unlock(&writer_arg->buffer_q_disc->mutex);
			LOG_PTHREAD_ERROR_BREAK(pthread_ret, writer_arg->name, __LINE__);

			if (current_bufelement == NULL)
			{
#ifdef NODAEMON_DEBUG
				fprintf(stderr, "thread_discwriter current_buffer == NULL available %d disc %d\n",
						writer_arg->buffer_q_available->no_buffer_elements,
						writer_arg->buffer_q_disc->no_buffer_elements);
#endif
				// Loop back and wait on the semaphore
				continue;
			}

#ifdef TRYLOCK
			tryCount = 0;
			do {
				pthread_ret = pthread_mutex_trylock(&writer_arg->writer_file->mutex_file);
				tryCount++;
				if (pthread_ret == EBUSY)
					sched_yield();
			} while (pthread_ret == EBUSY);
			if (tryCount > 5)
			{
				fprintf(stderr, "thread_discwriter: writer_file busy %d times\n",  tryCount);
			}
#else
			pthread_ret = pthread_mutex_lock(&writer_arg->writer_file->mutex_file);
			LOG_PTHREAD_ERROR_BREAK(pthread_ret, writer_arg->name, __LINE__);
#endif
			if (first_time_in_file == 0)
			{
				writer_arg->writer_file->start_time = current_bufelement->header.sendtime;
				first_time_in_file = current_bufelement->header.sendtime;
#ifdef NODAEMON_DEBUG
				first_written_counter = current_bufelement->header.counter;
				LOGMSG(LOG_NOTICE, "first_time_in_file: %"PRIu64" first_written_counter %u",
					first_time_in_file, first_written_counter);
#endif
			}

			writer_arg->writer_file->end_time = current_bufelement->header.sendtime;
			pthread_ret = pthread_mutex_unlock(&writer_arg->writer_file->mutex_file);
			LOG_PTHREAD_ERROR_BREAK(pthread_ret, writer_arg->name, __LINE__);

			last_time_in_file = current_bufelement->header.sendtime;
			time_diff = last_time_in_file - first_time_in_file;

#if TIME_FWRITE_CALL
			gettimeofday(&before_fwrite, NULL);
#endif
#if BINARY_STREAM_MODE
			// size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
			ret = fwrite(current_bufelement->packet_buffer, PACKET_BUFFER_SIZE, 1, fp);
			if (ret == 0)
			{
				add_trap_to_queue(TRAP_DELAYER_STOP, "Disc error");
				LOGMSG(LOG_WARNING, "%s: fwrite returned error %zd != %d, %d, %s",
					streaminterface.name, ret, PACKET_BUFFER_SIZE,
					errno, strerror(errno));
				pstate = STATE_SHUTDOWN;
			}
#else
			// ======= UNBUFFERED =======
			ret = write(fd, current_bufelement->packet_buffer, PACKET_BUFFER_SIZE);
			if (ret == -1)
			{
				add_trap_to_queue(TRAP_DELAYER_STOP, "Disc error");
				LOGMSG(LOG_WARNING, "%s: write returned error %d, %s",
					streaminterface.name,
					errno, strerror(errno));
				pstate = STATE_SHUTDOWN;
			}
#endif

#if TIME_FWRITE_CALL
			gettimeofday(&after_fwrite, NULL);
			elapsedTime = (after_fwrite.tv_sec - before_fwrite.tv_sec) * 1000.0;
			elapsedTime += (after_fwrite.tv_usec - before_fwrite.tv_usec) / 1000.0;
			if (elapsedTime > 5)
			{
				LOGMSG(LOG_WARNING, "%s: write took %f ms\n",
					streaminterface.name, elapsedTime);
			}
#endif

#ifdef NODAEMON_DEBUG
			// Update last written counter and sendtime
			last_written_counter = current_bufelement->header.counter;
#endif

			// ------ PUT BACK IN AVAILABLE Q ------
#ifdef TRYLOCK
			tryCount = 0;
			do {
				pthread_ret = pthread_mutex_trylock(&writer_arg->buffer_q_available->mutex);
				tryCount++;
				if (pthread_ret == EBUSY)
					sched_yield();
			} while (pthread_ret == EBUSY);
			if (tryCount > 5)
			{
				fprintf(stderr, "thread_discwriter: q_available busy %d times\n", tryCount);
			}
#else
			pthread_ret = pthread_mutex_lock(&writer_arg->buffer_q_available->mutex);
			LOG_PTHREAD_ERROR_BREAK(pthread_ret, writer_arg->name, __LINE__);
#endif
			// Clear because this is a RUDP packet
			returnBufferElement(&writer_arg->buffer_q_available->buffer_elements, current_bufelement, 1);
			// thread_discwriter
			writer_arg->buffer_q_available->no_buffer_elements++;
			pthread_ret = pthread_mutex_unlock(&writer_arg->buffer_q_available->mutex);
			LOG_PTHREAD_ERROR_BREAK(pthread_ret, writer_arg->name, __LINE__);
		}
#ifdef NODAEMON_DEBUG
		fprintf(stderr, "first %"PRIu64" last %"PRIu64" diff %"PRIu64"\n",
				first_time_in_file, last_time_in_file, time_diff);
		fprintf(stderr, "first counter %u last counter %u\n",
				first_written_counter, last_written_counter);
#endif

#if BINARY_STREAM_MODE
		fclose(fp);
		fp = NULL;
#else
		// ======= UNBUFFERED =======
		close(fd);
		fd = -1;
#endif

		// ------ Increment writer file id ------
#ifdef TRYLOCK
		tryCount = 0;
		do {
			pthread_ret = pthread_mutex_trylock(&writer_arg->writer_file->mutex_file);
			tryCount++;
			if (pthread_ret == EBUSY)
				sched_yield();
		} while (pthread_ret == EBUSY);
		if (tryCount > 5)
		{
			fprintf(stderr, "thread_discwriter: writer_file busy %d times\n", tryCount);
		}
#else
		pthread_ret = pthread_mutex_lock(&writer_arg->writer_file->mutex_file);
		LOG_PTHREAD_ERROR_BREAK(pthread_ret, writer_arg->name, __LINE__);
#endif
		writer_arg->writer_file->id++;
		pthread_ret = pthread_mutex_unlock(&writer_arg->writer_file->mutex_file);
		LOG_PTHREAD_ERROR_BREAK(pthread_ret, writer_arg->name, __LINE__);
	}

	pstate = STATE_SHUTDOWN;
#if BINARY_STREAM_MODE
	if (fp != NULL)
	{
		fclose(fp);
	}
#else
	// ======= UNBUFFERED =======
	if (fd != -1)
	{
		close(fd);
	}
#endif

	LOGMSG(LOG_WARNING, "%s: EXIT thread_discwriter",
		streaminterface.name);
	return NULL;
}

void *thread_discreader(void *arg)
{
#if BINARY_STREAM_MODE
	FILE *fp=NULL;
#else
	int fd = 0;
#endif

	char filename[VIDEO_PATH_LEN+256];
	bufferelement *current_bufelement;
	size_t read_ret=0;
	int noOfRead, ret_val, pthread_ret, is_eof = 0;
	uint8_t count_missed_acquire_buffer = 0;

	uint32_t first_read_counter;
	long discreader_file_pos;
	discreader_file_pos = 0;

	thread_disc_reader_arg *reader_arg = (thread_disc_reader_arg *)arg;
	e_process_state last_state = *reader_arg->state;
	bool state_change = false;
#ifdef TRYLOCK
	int tryCount = 0;
#endif

#if TIME_FREAD_CALL
	struct timeval before_fread, after_fread;
	double elapsedTime;
#endif

	char interfacename[16];
	snprintf(interfacename, 16, "%s[%d]", reader_arg->name, reader_arg->pathno);

#if 0
	if (reader_arg->writer_file->id == reader_arg->reader_file->id)
	{
		// Wait for a kick from discwritern if writer and reader are on the same file
		pthread_cond_wait(&condt_disc_signal, &mutex_disc_signal);
	}
#endif

	while (pstate != STATE_SHUTDOWN)
	{
		// ------ HANDLE STATECHANGE ------
		if (state_change || *reader_arg->state != last_state)
		{
			state_change = false;
#ifdef NODAEMON_DEBUG
			fprintf(stderr, "thread_discreader[%d]: last_state %d new_state %d no_buffer_elements %d\n",
					reader_arg->pathno,
					*reader_arg->state,
					last_state,
					reader_arg->buffer_q_send->no_buffer_elements);
#endif

			// Was in paused mode
			if (*reader_arg->state == DELAYER_STATE_DELAYED)
			{
				// And now start to play at the pause position
				// Move parameters from Pause struct to reader_file struct
				pthread_ret = pthread_mutex_lock(&reader_arg->pause->mutex_pause);
				LOG_PTHREAD_ERROR_BREAK(pthread_ret, interfacename, __LINE__);
				uint32_t pause_id = reader_arg->pause->pause_file_id;
				discreader_file_pos = reader_arg->pause->pause_pos;
				pthread_ret = pthread_mutex_unlock(&reader_arg->pause->mutex_pause);
				LOG_PTHREAD_ERROR_BREAK(pthread_ret, interfacename, __LINE__);
#ifdef NODAEMON_DEBUG
				fprintf(stderr, "thread_discreader: DELAYER_STATE_PAUSE -> DELAYER_STATE_DELAYED\n");
				fprintf(stderr, "reader_arg->pause->pause_file_id %u\n",
						pause_id);
#endif

				pthread_ret = pthread_mutex_lock(&reader_arg->reader_file->mutex_file);
				LOG_PTHREAD_ERROR_BREAK(pthread_ret, interfacename, __LINE__);
				reader_arg->reader_file->id = pause_id;
				pthread_ret = pthread_mutex_unlock(&reader_arg->reader_file->mutex_file);
				LOG_PTHREAD_ERROR_BREAK(pthread_ret, interfacename, __LINE__);
				// Can switch state between here and next if state != DELAYER_STATE_DELAYED so need to unlock
			}
			last_state = *reader_arg->state;
		}

		// ----------------------------------------------------------
		// IF STATE
		// * Livestreaming
		// * Paused
		// if anything in send Q clean it
		// Dont do anything - just loop and wait for state change
		// ----------------------------------------------------------
		if (*reader_arg->state != DELAYER_STATE_DELAYED)
		{
			if (*reader_arg->state != last_state)
			{
				// todo: fix!
				// Race condition - switched in between the two ifs..
				state_change = true;
				continue;
			}

			// ------ Clean send Q --------
			if (*reader_arg->state == DELAYER_STATE_PAUSE && reader_arg->buffer_q_send->buffer_elements != NULL)
			{
#ifdef NODAEMON_DEBUG
				fprintf(stderr, "%s[%d]: Clean send_buffer_elements no_available_buffer_elements: %d\n",
						reader_arg->name, reader_arg->pathno,
						reader_arg->buffer_q_available->no_buffer_elements);
#endif
				pthread_ret = pthread_mutex_lock(&reader_arg->buffer_q_available->mutex);
				LOG_PTHREAD_ERROR_BREAK(pthread_ret, interfacename, __LINE__);
				pthread_ret = pthread_mutex_lock(&reader_arg->buffer_q_send->mutex);
				LOG_PTHREAD_ERROR_BREAK(pthread_ret, interfacename, __LINE__);
				while (reader_arg->buffer_q_send->buffer_elements != NULL)
				{
					current_bufelement = chop_head_of_buffer(&reader_arg->buffer_q_send->buffer_elements);
#ifdef NODAEMON_DEBUG
					fprintf(stderr, "%s[%d]: counter: %u\n",
							reader_arg->name, reader_arg->pathno,
							current_bufelement->header.counter);
#endif
					returnBufferElement(&reader_arg->buffer_q_available->buffer_elements, current_bufelement, 0);
					// thread_discreader
					reader_arg->buffer_q_available->no_buffer_elements++;
					reader_arg->buffer_q_send->no_buffer_elements--;
				}
				pthread_ret = pthread_mutex_unlock(&reader_arg->buffer_q_send->mutex);
				LOG_PTHREAD_ERROR_BREAK(pthread_ret, interfacename, __LINE__);

				pthread_ret = pthread_mutex_unlock(&reader_arg->buffer_q_available->mutex);
				LOG_PTHREAD_ERROR_BREAK(pthread_ret, interfacename, __LINE__);
#ifdef NODAEMON_DEBUG
				fprintf(stderr, "%s[%d]: after Clean no_available_buffer_elements: %d\n",
						reader_arg->name, reader_arg->pathno,
						reader_arg->buffer_q_available->no_buffer_elements);
#endif
			}

			// TODO Wait on condition variable
			usleep(2000);
			continue;
		}

		pthread_ret = pthread_mutex_lock(&reader_arg->reader_file->mutex_file);
		LOG_PTHREAD_ERROR_BREAK(pthread_ret, interfacename, __LINE__);
		ret_val = taste_file(reader_arg->name, reader_arg->reader_file);
		pthread_ret = pthread_mutex_unlock(&reader_arg->reader_file->mutex_file);
		LOG_PTHREAD_ERROR_BREAK(pthread_ret, interfacename, __LINE__);

		if (ret_val != 0)
		{
			LOGMSG(LOG_CRIT, "%s[%d]: discreader taste (%d) cant parse file(%d)!",
				reader_arg->name,
				reader_arg->pathno,
				ret_val, reader_arg->reader_file->id);
			if (reader_arg->reader_file->id >= reader_arg->writer_file->id)
			{
				// if it wraps?
				if (reader_arg->writer_file->id <= 1)
				{
					pthread_ret = pthread_mutex_lock(&reader_arg->reader_file->mutex_file);
					LOG_PTHREAD_ERROR_BREAK(pthread_ret, interfacename, __LINE__);

					reader_arg->reader_file->id = 0;

					pthread_ret = pthread_mutex_unlock(&reader_arg->reader_file->mutex_file);
					LOG_PTHREAD_ERROR_BREAK(pthread_ret, interfacename, __LINE__);
				}
				
				// otherwise assume reader are on the same file as the writer - give it some time
				sleep(1);
				continue;
			}

			// Otherwise assume the file are corrupt and take the next one! 
			pthread_ret = pthread_mutex_lock(&reader_arg->reader_file->mutex_file);
			LOG_PTHREAD_ERROR_BREAK(pthread_ret, interfacename, __LINE__);

			reader_arg->reader_file->id++;

			pthread_ret = pthread_mutex_unlock(&reader_arg->reader_file->mutex_file);
			LOG_PTHREAD_ERROR_BREAK(pthread_ret, interfacename, __LINE__);
			continue;
		}

		first_read_counter = 0;
		snprintf(filename, VIDEO_PATH_LEN+255, "%s/%s_chunk_%u.bin",
				 video_path, reader_arg->name, reader_arg->reader_file->id);

// #ifdef NODAEMON_DEBUG
#if 1
		LOGMSG(LOG_NOTICE, "%s[%d]: thread_discreader: Open filename: %s\n",
			reader_arg->name,
			reader_arg->pathno,
			filename);
#endif

#if BINARY_STREAM_MODE
		fp = fopen(filename, "r");

		if (fp == NULL)
		{
			LOGMSG(LOG_WARNING, "%s[%d]: fopen failed: %d %s",
				reader_arg->name,
				reader_arg->pathno,
				errno,
				strerror(errno));
			sleep(1);
			continue;
		}
#if NO_BUFFERED_STREAM
		ret_val = setvbuf(fp, NULL, _IONBF, 0);
		if (ret_val != 0)
		{
			LOGMSG(LOG_WARNING, "%s: thread_discreader: Cant set unbuffered stream",
				streaminterface.name);
		}
#endif
#else
		fd = open(filename, O_RDONLY);
		if (fd == -1)
		{
			LOGMSG(LOG_WARNING, "%s[%d]: fopen failed: %d %s",
				reader_arg->name,
				reader_arg->pathno,
				errno,
				strerror(errno));
			sleep(1);
			continue;
		}
#endif
		noOfRead = 0;

		//noUsleep = 0;

		if (discreader_file_pos > 0)
		{
#if BINARY_STREAM_MODE
			ret_val = fseek(fp, discreader_file_pos, SEEK_SET);
#else
			ret_val = lseek(fd, discreader_file_pos, SEEK_SET);
#endif
			if (ret_val == -1)
			{
#ifdef NODAEMON_DEBUG
				fprintf(stderr, "%s[%d]: fseek failed %d %s\n",
						reader_arg->name,
						reader_arg->pathno,
						errno, strerror(errno));
#endif
				pstate = STATE_SHUTDOWN;
#if BINARY_STREAM_MODE
				fclose(fp);
#else
				close(fd);
#endif
				continue;
			}
			discreader_file_pos = 0;
		}
#ifdef NODAEMON_DEBUG
		struct timeval start_tv;
		gettimeofday(&start_tv, NULL);
#endif
		is_eof = 0;
		while (is_eof == 0 && pstate != STATE_SHUTDOWN && last_state == *reader_arg->state)
		{
			// Assumption 50 mbps stream ~5000pps
			// dT between packets 0.2ms
			// read a head 250ms => 250/0.2ms = 1250packets
			if (reader_arg->buffer_q_send->no_buffer_elements > 1250)
			{
				usleep(10000);
				continue;
			}

#ifdef TRYLOCK
			tryCount = 0;
			do {
				pthread_ret = pthread_mutex_trylock(&reader_arg->buffer_q_available->mutex);
				tryCount++;
				if (pthread_ret == EBUSY)
					sched_yield();
			} while (pthread_ret == EBUSY);
			if (tryCount > 5)
			{
				fprintf(stderr, "thread_discreader[%d]: buffer_q_available#1 busy %d times\n", reader_arg->pathno, tryCount);
			}
#else
			pthread_ret = pthread_mutex_lock(&reader_arg->buffer_q_available->mutex);
			LOG_PTHREAD_ERROR_BREAK(pthread_ret, interfacename, __LINE__);
#endif

			current_bufelement = getAvailableBufferElement(&reader_arg->buffer_q_available->buffer_elements);
			if (current_bufelement != NULL)
			{
				// thread_discreader
				reader_arg->buffer_q_available->no_buffer_elements--;
			}

			pthread_ret = pthread_mutex_unlock(&reader_arg->buffer_q_available->mutex);
			LOG_PTHREAD_ERROR_BREAK(pthread_ret, interfacename, __LINE__);

			if (current_bufelement == NULL)
			{
				LOGMSG(LOG_WARNING, "%s[%d]: discreader current_bufelement == NULL A %d S %d\n",
					reader_arg->name,
					reader_arg->pathno,
					reader_arg->buffer_q_available->no_buffer_elements,
					reader_arg->buffer_q_send->no_buffer_elements);
				sched_yield();
				usleep(10000);
				count_missed_acquire_buffer++;
				if (count_missed_acquire_buffer > 1)
				{
					add_trap_to_queue(TRAP_DELAYER_LOST_INPUT, "Buffer exhausted");
					logtodbsys(reader_arg->mutex_dbconn, *reader_arg->dbconn,
							streaminterface.id, POVLOGLEVEL_ERR,
							"%s[%d]: discreader buffer exhausted",
							reader_arg->name,
							reader_arg->pathno);

					pstate = STATE_SHUTDOWN;
				}
				continue;
			}
			count_missed_acquire_buffer = 0;
#if TIME_FREAD_CALL
			gettimeofday(&before_fread, NULL);
#endif

#if BINARY_STREAM_MODE
			read_ret = fread(current_bufelement->packet_buffer, PACKET_BUFFER_SIZE, 1, fp);
#else
			read_ret = read(fd, current_bufelement->packet_buffer, PACKET_BUFFER_SIZE);
#endif
#if TIME_FREAD_CALL
			gettimeofday(&after_fread, NULL);
			elapsedTime = (after_fread.tv_sec - before_fread.tv_sec) * 1000.0;
			elapsedTime += (after_fread.tv_usec - before_fread.tv_usec) / 1000.0;
			if (elapsedTime > 5)
			{
				LOGMSG(LOG_WARNING, "%s[%d]: fread took %f ms\n",
					reader_arg->name, reader_arg->pathno, elapsedTime);
			}
#endif

			// fread == 0 on both error and EOF
			// read == 0 EOF read == -1 error
			if (read_ret <= 0)
			{
				// === LOG ERROR ===
#if BINARY_STREAM_MODE
				if (ferror(fp) != 0)
				{
#ifdef NODAEMON_DEBUG
					fprintf(stderr, "%s[%d]: discreader(%s) ferror(fp) %d %s\n",
							reader_arg->name,
							reader_arg->pathno,
							filename,
							errno, strerror(errno));
#endif
					clearerr(fp);
				}
#else
#ifdef NODAEMON_DEBUG
				if (read_ret == -1)
				{
					fprintf(stderr, "%s[%d]: discreader(%s) %d %s\n",
							reader_arg->name,
							reader_arg->pathno,
							filename,
							errno, strerror(errno));
				}
#endif
#endif

				// === RETURN BUFFER ===
#ifdef TRYLOCK
				tryCount = 0;
				do {
					pthread_ret = pthread_mutex_trylock(&reader_arg->buffer_q_available->mutex);
					tryCount++;
					if (pthread_ret == EBUSY)
						sched_yield();
				} while (pthread_ret == EBUSY);
				if (tryCount > 5)
				{
					fprintf(stderr, "thread_discreader[%d]: buffer_q_available#2 busy %d times\n",
						reader_arg->pathno, tryCount);
				}
#else
				pthread_ret = pthread_mutex_lock(&reader_arg->buffer_q_available->mutex);
				LOG_PTHREAD_ERROR_BREAK(pthread_ret, interfacename, __LINE__);
#endif
				returnBufferElement(&reader_arg->buffer_q_available->buffer_elements, current_bufelement, 0);
				// thread_discreader
				reader_arg->buffer_q_available->no_buffer_elements++;
				pthread_ret = pthread_mutex_unlock(&reader_arg->buffer_q_available->mutex);
				LOG_PTHREAD_ERROR_BREAK(pthread_ret, interfacename, __LINE__);
				
#ifdef NODAEMON_DEBUG
				fprintf(stderr, "%s[%d]: discreader(%s) fread ret == 0\n",
						reader_arg->name,
						reader_arg->pathno, filename);
#endif
				// if reading on the same file the writer are writing to.
				// sometimes the read call will return EOF but the file are not "finished"
				// Adjust the filepos and try again 

				// Decide if eof
#if BINARY_STREAM_MODE
				is_eof = (feof(fp) != 0);
#else
				is_eof = (read_ret == 0);
#endif
				if (is_eof && (reader_arg->reader_file->id == reader_arg->writer_file->id))
				{
					int ret;
					long offset;

					offset = noOfRead*PACKET_BUFFER_SIZE;
#if BINARY_STREAM_MODE
					ret = fseek(fp, offset, SEEK_SET);
#else
					ret = lseek(fd, offset, SEEK_SET);
#endif
					if (ret == -1)
					{
						LOGMSG(LOG_WARNING, "%s: Cant set position %d %s",
							reader_arg->name, errno, strerror(errno));
						// if this happens the while will take care of it and another EOF will occur
					}
					else
					{
						// succeeded to move file pointer
						is_eof = 0;
					}
					
					usleep(10000);
				}

				continue;
			}

			noOfRead++;

			parseHeader(current_bufelement->packet_buffer, &(current_bufelement->header));

			if (first_read_counter == 0)
			{
				first_read_counter = current_bufelement->header.counter;
#ifdef NODAEMON_DEBUG
				fprintf(stderr, "%s[%d]: discreader first time: %"PRIu64" first counter %u\n",
					reader_arg->name, reader_arg->pathno,
					current_bufelement->header.sendtime, current_bufelement->header.counter);
				struct timeval tv;
				gettimeofday(&tv, NULL);
				uint64_t now = tv.tv_sec*1000000 + tv.tv_usec;
				fprintf(stderr, "%s[%d]: pause_diff %"PRIu64"  now %"PRIu64" first counter %u\n",
					reader_arg->name,
					reader_arg->pathno,
					reader_arg->pause->pause_diff,
					now,
					current_bufelement->header.counter);
				fprintf(stderr, "%s[%d] discreader A %d S %d\n",
					reader_arg->name,
					reader_arg->pathno,
					reader_arg->buffer_q_available->no_buffer_elements,
					reader_arg->buffer_q_send->no_buffer_elements);
#endif

			}

#ifdef TRYLOCK
			tryCount = 0;
			do {
				pthread_ret = pthread_mutex_trylock(&reader_arg->buffer_q_send->mutex);
				tryCount++;
				if (pthread_ret == EBUSY)
					sched_yield();
			} while (pthread_ret == EBUSY);
			if (tryCount > 5)
			{
				fprintf(stderr, "thread_discreader[%d]: buffer_q_send busy %d times\n", reader_arg->pathno, tryCount);
			}
#else
			pthread_ret = pthread_mutex_lock(&reader_arg->buffer_q_send->mutex);
			LOG_PTHREAD_ERROR_BREAK(pthread_ret, interfacename, __LINE__);
#endif
			putInBuffer(&reader_arg->buffer_q_send->buffer_elements, current_bufelement);
			// thread_discreader
			reader_arg->buffer_q_send->no_buffer_elements++;
			pthread_ret = pthread_mutex_unlock(&reader_arg->buffer_q_send->mutex);
			LOG_PTHREAD_ERROR_BREAK(pthread_ret, interfacename, __LINE__);
		}

		if (last_state != *reader_arg->state)
		{
			state_change = true;
		}


#ifdef NODAEMON_DEBUG
		struct timeval tv;
		gettimeofday(&tv, NULL);
		int64_t timeDiff = calc_timeval_diff(&tv, &start_tv);
		fprintf(stderr, "%s[%d]: errno %d strerror %s start WHILE\n",
				reader_arg->name,
				reader_arg->pathno,
				errno, strerror(errno));
		fprintf(stderr, "%s[%d]: noOfRead %d after while, total time %"PRId64"\n",
				reader_arg->name,
				reader_arg->pathno,
				noOfRead, timeDiff);
#endif
#if BINARY_STREAM_MODE
		if (ferror(fp) != 0)
		{
#ifdef NODAEMON_DEBUG
			fprintf(stderr, "ferror(fp) %d %s\n",
				errno, strerror(errno));
#endif
			clearerr(fp);
		}
		fclose(fp);
#else
		close(fd);
#endif
#ifdef NODAEMON_DEBUG
		fprintf(stderr, "%s[%d] discreader A %d S %d\n",
				reader_arg->name,
				reader_arg->pathno,
				reader_arg->buffer_q_available->no_buffer_elements,
				reader_arg->buffer_q_send->no_buffer_elements);
#endif

		while ((state_change == false) &&
			(last_state == *reader_arg->state) &&
			(reader_arg->reader_file->id >= reader_arg->writer_file->id) &&
			pstate != STATE_SHUTDOWN)
		{
			// reader file cant be higher than the writer file
			// can happen if there are no multicast traffic
#ifdef NODAEMON_DEBUG
			fprintf(stderr, "%s[%d]: reader_file %u >= writer_file %u\n",
				reader_arg->name, reader_arg->pathno,
				reader_arg->reader_file->id, reader_arg->writer_file->id);
#endif
			usleep(100000);
		}
		if (state_change || last_state != *reader_arg->state)
		{
			continue;
		}
		pthread_ret = pthread_mutex_lock(&reader_arg->reader_file->mutex_file);
		LOG_PTHREAD_ERROR_BREAK(pthread_ret, interfacename, __LINE__);
		reader_arg->reader_file->id++;
		pthread_ret = pthread_mutex_unlock(&reader_arg->reader_file->mutex_file);
		LOG_PTHREAD_ERROR_BREAK(pthread_ret, interfacename, __LINE__);
	}

	pstate = STATE_SHUTDOWN;
	LOGMSG(LOG_WARNING, "%s[%d]: EXIT thread_discreader",
		reader_arg->name,
		reader_arg->pathno);

	return NULL;
}

// From rudp_receiver.c
// void *thread_sender(void *arg)
void *thread_mc_sender(void *arg)
{
	thread_mc_arg *sender_arg = (thread_mc_arg *)arg;

	bufferelement *current_bufelement=NULL;
	bufferelement *return_sent_buf_to_avail=NULL;
	bufferelement *sendready_bufelements=NULL;

	struct sockaddr_in sockaddr_mc_output;
	ssize_t sent_bytes;
	uint64_t total_sent_bytes = 0;
	int fd_mc_output;
	struct in_addr multicast_interface;
	if (multicast_interface_address(&multicast_interface) != 0)
	{
		pstate = STATE_SHUTDOWN;
		return NULL;
	}

	uint64_t sleeptime = 10000;
	uint64_t diff = 0;
	uint64_t buffer_timeout_us = sender_arg->buffer_timeout * 1000;
	struct timeval curtime;

	int pthread_ret = 0, retval, setsize, getsize;
	int strip_rtp = 0;
#ifdef TRYLOCK
	int tryCount = 0;
#endif
	socklen_t len = sizeof(int);

	char interfacename[16];
	snprintf(interfacename, 16, "%s[%d]", sender_arg->name, sender_arg->pathno);

	// SETUP MULTICAST SOCKET
	if ((fd_mc_output=socket(AF_INET,SOCK_DGRAM,0)) < 0)
	{
		LOGMSG(LOG_ERR, "%s[%d]: failed to create multicast socket() %s",
			sender_arg->name,
			sender_arg->pathno,
			strerror(errno));
		pstate = STATE_SHUTDOWN;
		return NULL;
	}

#if 1
	for (setsize = 10524287; getsize < setsize; setsize -= 32768)
	{
		retval = setsockopt(fd_mc_output, SOL_SOCKET, SO_SNDBUF, (char*)&setsize, len);
		if (retval != 0)
		{
			LOGMSG(LOG_WARNING, "%s: setsockopt %s (%d)",
				interfacename, strerror(errno), errno);
			break;
		}

		retval = getsockopt(fd_mc_output, SOL_SOCKET, SO_SNDBUF, (char*)&getsize, &len);
		if (retval != 0)
		{
			LOGMSG(LOG_WARNING, "%s: getsockopt %s (%d)",
				interfacename, strerror(errno), errno);
			break;
		}
	}
#else
	retval = getsockopt(fd_mc_output, SOL_SOCKET, SO_SNDBUF, (char*)&getsize, &len);
	if (retval != 0)
	{
		LOGMSG(LOG_WARNING, "%s: getsockopt %s (%d)",
			interfacename, strerror(errno), errno);
	}
#endif

	LOGMSG(LOG_NOTICE, "%s: SO_SNDBUF=%d",
		interfacename, getsize);

	memset(&sockaddr_mc_output, 0, sizeof(struct sockaddr_in));
	sockaddr_mc_output.sin_family=AF_INET;
	sockaddr_mc_output.sin_addr.s_addr=inet_addr(sender_arg->multicast_address);
	sockaddr_mc_output.sin_port=htons(sender_arg->multicast_port);
	
	if (multicast_interface.s_addr != htonl(INADDR_ANY))
	{
		if (setsockopt(fd_mc_output, IPPROTO_IP, IP_MULTICAST_IF, &multicast_interface, sizeof(multicast_interface)) == -1)
		{
			LOGMSG(LOG_ERR, "%s: cannot select multicast output interface: %s", interfacename, strerror(errno));
			close(fd_mc_output);
			pstate = STATE_SHUTDOWN;
			return NULL;
		}
		LOGMSG(LOG_NOTICE, "%s: multicast output interface %s", interfacename, getenv("DELAYER_MULTICAST_INTERFACE"));
	}

	// set TTL
	if (setsockopt(fd_mc_output, IPPROTO_IP, IP_MULTICAST_TTL, (void *) &sender_arg->multicast_ttl,  sizeof(sender_arg->multicast_ttl)) == -1)
	{
#ifdef NODAEMON_DEBUG
		LOGMSG(LOG_NOTICE, "%s[%d]: Can not set ttl! %s %d",
			sender_arg->name, sender_arg->pathno,
			strerror(errno), errno);
#endif
	}

#ifdef NODAEMON_DEBUG
	LOGMSG(LOG_NOTICE, "%s[%d]: Starting thread loop\n",
		sender_arg->name, sender_arg->pathno);
#endif

	int sendready_count;

	while (pstate != STATE_SHUTDOWN)
	{
		if (*sender_arg->state == DELAYER_STATE_PAUSE)
		{
			usleep(1000);
			sleeptime=0;
			continue;
		}
		if (sleeptime > 1000000)
		{
#if 0
			pthread_ret = pthread_mutex_lock(&mutex_thread_sender_signal);
			LOG_PTHREAD_ERROR_BREAK(pthread_ret, interfacename, __LINE__);
			clock_gettime(CLOCK_REALTIME, &thread_sender_timeout);
			thread_sender_timeout.tv_sec += sleeptime/1000000;

			pthread_cond_timedwait(&condt_thread_sender_signal,
					&mutex_thread_sender_signal, &thread_sender_timeout);
			pthread_ret = pthread_mutex_unlock(&mutex_thread_sender_signal);
			LOG_PTHREAD_ERROR_BREAK(pthread_ret, interfacename, __LINE__);
#endif
			sleep(sleeptime/1000000);
		}
		else
		{
			if (sleeptime > 500000)
			{
				sleeptime = 500000;
			}
			usleep(sleeptime);
		}

#ifdef TRYLOCK
		tryCount = 0;
		do {
			pthread_ret = pthread_mutex_trylock(&sender_arg->buffer_q->mutex);
			tryCount++;
			if (pthread_ret == EBUSY)
				sched_yield();
		} while (pthread_ret == EBUSY);
		if (tryCount > 5)
		{
			fprintf(stderr, "thread_mc_sender: %s[%d]: buffer_q busy %d times\n",
					sender_arg->name, sender_arg->pathno, tryCount);
		}
#else
		pthread_ret = pthread_mutex_lock(&sender_arg->buffer_q->mutex);
		LOG_PTHREAD_ERROR_BREAK(pthread_ret, interfacename, __LINE__);
#endif
		sendready_bufelements = getTimedoutPackets(&sender_arg->buffer_q->buffer_elements, sender_arg->buffer_timeout);
		sendready_count = countList(sendready_bufelements);
		sender_arg->buffer_q->no_buffer_elements -= sendready_count;
		pthread_ret = pthread_mutex_unlock(&sender_arg->buffer_q->mutex);
		LOG_PTHREAD_ERROR_BREAK(pthread_ret, interfacename, __LINE__);

#ifdef NODAEMON_DEBUG
		if (sendready_count > 100)
		{
			fprintf(stderr, "thread_mc_sender: %s[%d]: %d sleeptime %"PRIu64" sendready_bufelements != NULL A: %d S: %d\n",
				sender_arg->name, sender_arg->pathno, sendready_count, sleeptime,
				sender_arg->buffer_q_available->no_buffer_elements,	sender_arg->buffer_q->no_buffer_elements);
		}
#endif

		total_sent_bytes = 0;
		while (sendready_bufelements != NULL &&
				pstate != STATE_SHUTDOWN &&
				*sender_arg->state != DELAYER_STATE_PAUSE)
		{
			current_bufelement = chop_head_of_buffer(&sendready_bufelements);

			if (sender_arg->rawudp && current_bufelement->rtp)
			{
				strip_rtp = RTP_MIN_HEADER_SIZE;
			}
			else
			{
				strip_rtp = 0;
			}

			sent_bytes = sendto(fd_mc_output, current_bufelement->packet_buffer + RUDP_HEADER_SIZE + strip_rtp,
						current_bufelement->header.payloadlength-strip_rtp, 0,
						(struct sockaddr *) &sockaddr_mc_output,
						sizeof(struct sockaddr_in));
			if (sent_bytes < 0)
			{
				syslog(LOG_ERR, "%s[%d]: sendto(fd_mc_output) %zd != %d (%s)",
					sender_arg->name, sender_arg->pathno,
					sent_bytes,
					current_bufelement->header.payloadlength,
					strerror(errno));
				close(fd_mc_output);
				pstate = STATE_SHUTDOWN;
				continue;
			}
			else if (sent_bytes != (current_bufelement->header.payloadlength - strip_rtp))
			{
				LOGMSG(LOG_ERR, "%s[%d]: sendto(fd_mc_output) %zd != %d (%s)",
					sender_arg->name, sender_arg->pathno,
					sent_bytes,
					current_bufelement->header.payloadlength,
					strerror(errno));

				close(fd_mc_output);
				pstate = STATE_SHUTDOWN;
				continue;
			}

			total_sent_bytes += sent_bytes;

			sender_arg->stat_last_timestamp = current_bufelement->header.sendtime;

			putInBuffer(&return_sent_buf_to_avail, current_bufelement);
		}
#ifdef TRYLOCK
		tryCount = 0;
		do {
			pthread_ret = pthread_mutex_trylock(sender_arg->mutex_stat);
			tryCount++;
			if (pthread_ret == EBUSY)
				sched_yield();
		} while (pthread_ret == EBUSY);
		if (tryCount > 5)
		{
			fprintf(stderr, "thread_mc_sender: %s[%d]: mutex_stat busy %d times\n",
					sender_arg->name, sender_arg->pathno, tryCount);
		}
#else
		pthread_ret = pthread_mutex_lock(sender_arg->mutex_stat);
		LOG_PTHREAD_ERROR_BREAK(pthread_ret, interfacename, __LINE__);
#endif

		// reuse RUDP bytes per path to "save" a variable in the struct..
		sender_arg->rudp_sequence_list->transfered_bytes_rudp_per_path[sender_arg->pathno] += total_sent_bytes;
		sender_arg->rt_stats->stat_outbytes += total_sent_bytes;
		pthread_ret = pthread_mutex_unlock(sender_arg->mutex_stat);
		LOG_PTHREAD_ERROR_BREAK(pthread_ret, interfacename, __LINE__);

		// Long delay - return to availble buffer
#ifdef TRYLOCK
		tryCount = 0;
		do {
			pthread_ret = pthread_mutex_trylock(&sender_arg->buffer_q_available->mutex);
			tryCount++;
			if (pthread_ret == EBUSY)
				sched_yield();
		} while (pthread_ret == EBUSY);
		if (tryCount > 5)
		{
			fprintf(stderr, "thread_mc_sender: %s[%d]: buffer_q_available busy %d times\n",
					sender_arg->name, sender_arg->pathno, tryCount);
		}
#else
		pthread_ret = pthread_mutex_lock(&sender_arg->buffer_q_available->mutex);
		LOG_PTHREAD_ERROR_BREAK(pthread_ret, interfacename, __LINE__);
#endif
		while (return_sent_buf_to_avail != NULL)
		{
			current_bufelement = chop_head_of_buffer(&return_sent_buf_to_avail);
			// In long delay this will not clear!
			// In streaming case this will clear
			returnBufferElement(&sender_arg->buffer_q_available->buffer_elements, current_bufelement, 0);
			// thread_mc_sender
			sender_arg->buffer_q_available->no_buffer_elements++;
		}
		pthread_ret = pthread_mutex_unlock(&sender_arg->buffer_q_available->mutex);
		LOG_PTHREAD_ERROR_BREAK(pthread_ret, interfacename, __LINE__);

		if (sendready_bufelements != NULL || *sender_arg->state == DELAYER_STATE_PAUSE)
		{
			// ---- CLEAN Seandreadybufelements - got from getTimedoutPackets -------
#ifdef NODAEMON_DEBUG
			fprintf(stderr, "%s[%d]: sendready_bufelements != NULL A: %d S: %d\n",
				sender_arg->name, sender_arg->pathno,
				sender_arg->buffer_q_available->no_buffer_elements, sender_arg->buffer_q->no_buffer_elements);
#endif
			pthread_ret = pthread_mutex_lock(&sender_arg->buffer_q_available->mutex);
			LOG_PTHREAD_ERROR_BREAK(pthread_ret, interfacename, __LINE__);
			while (sendready_bufelements != NULL)
			{
				current_bufelement = chop_head_of_buffer(&sendready_bufelements);
#ifdef NODAEMON_DEBUG
				fprintf(stderr, "%s[%d]: sendready_bufelements: counter: %u sendtime %"PRIu64"\n",
					sender_arg->name, sender_arg->pathno,
					current_bufelement->header.counter, current_bufelement->header.sendtime);
#endif
				returnBufferElement(&sender_arg->buffer_q_available->buffer_elements, current_bufelement, 0);
				// thread_mc_sender
				sender_arg->buffer_q_available->no_buffer_elements++;
			}

			// ------ CLEAN rest of elements in buffer Q  (from discreader) -------
			if (sender_arg->buffer_q->buffer_elements != NULL)
			{
				pthread_ret = pthread_mutex_lock(&sender_arg->buffer_q->mutex);
				LOG_PTHREAD_ERROR_BREAK(pthread_ret, interfacename, __LINE__);
				while (sender_arg->buffer_q->buffer_elements != NULL)
				{
					current_bufelement = chop_head_of_buffer(&sender_arg->buffer_q->buffer_elements);
					returnBufferElement(&sender_arg->buffer_q_available->buffer_elements, current_bufelement, 0);
					// thread_mc_sender
					sender_arg->buffer_q_available->no_buffer_elements++;
					sender_arg->buffer_q->no_buffer_elements--;
				}
				pthread_ret = pthread_mutex_unlock(&sender_arg->buffer_q->mutex);
				LOG_PTHREAD_ERROR_BREAK(pthread_ret, interfacename, __LINE__);
			}
			pthread_ret = pthread_mutex_unlock(&sender_arg->buffer_q_available->mutex);
			LOG_PTHREAD_ERROR_BREAK(pthread_ret, interfacename, __LINE__);
#ifdef NODAEMON_DEBUG
			fprintf(stderr, "%s[%d]: after A: %d S %d\n",
				sender_arg->name, sender_arg->pathno,
				sender_arg->buffer_q_available->no_buffer_elements, sender_arg->buffer_q->no_buffer_elements);
#endif
		}

		// ------ Calculate new sleeptime from thats first in send Q ------
		sleeptime = 100;

#ifdef TRYLOCK
		tryCount = 0;
		do {
			pthread_ret = pthread_mutex_trylock(&sender_arg->buffer_q->mutex);
			tryCount++;
			if (pthread_ret == EBUSY)
				sched_yield();
		} while (pthread_ret == EBUSY);
		if (tryCount > 5)
		{
			fprintf(stderr, "thread_mc_sender: %s[%d]: buffer_q busy %d times\n",
					sender_arg->name, sender_arg->pathno, tryCount);
		}
#else
		pthread_ret = pthread_mutex_lock(&sender_arg->buffer_q->mutex);
		LOG_PTHREAD_ERROR_BREAK(pthread_ret, interfacename, __LINE__);
#endif
		if (sender_arg->buffer_q->buffer_elements != NULL)
		{
			gettimeofday(&curtime, NULL);
			diff = curtime.tv_sec*1000000 + curtime.tv_usec - sender_arg->buffer_q->buffer_elements->header.sendtime;
			if (diff > buffer_timeout_us)
			{
				sleeptime = 1;
			}
			else
			{
				sleeptime = buffer_timeout_us - diff;
			}
		}
		pthread_ret = pthread_mutex_unlock(&sender_arg->buffer_q->mutex);
		LOG_PTHREAD_ERROR_BREAK(pthread_ret, interfacename, __LINE__);
	}

	pstate = STATE_SHUTDOWN;
	close(fd_mc_output);

	LOGMSG(LOG_WARNING, "%s[%d]: EXIT thread_mc_sender",
		sender_arg->name, sender_arg->pathno);

	return NULL;
}
