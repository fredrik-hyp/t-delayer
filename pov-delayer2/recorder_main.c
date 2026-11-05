#include "dbrecplay.h"
#include "dbsysteminfo.h"
#include "common-db.h"
#include "thread_statistic.h"
#include "thread_traps.h"
#include "thread_pokepovmanager.h"
#include "buffer.h"

#include "stat_func.h"
#include "states.h"

#include <assert.h>
#ifdef __linux__
#include <syscall.h>
#endif
#include <semaphore.h>
#include <sched.h>

#include "delayer.h"

uint8_t packetcountersinlist[MAX_PACKET_COUNTER+1];

volatile PROCESS_STATE pstate = STATE_STARTING;

pthread_mutex_t mutex_stat;
s_runtimestat rt_stats;
sequence_statistics rudp_sequence_list;

pthread_mutex_t mutex_traps;
trapelement *trapqueue;

s_interface streaminterface;

char video_path[VIDEO_PATH_LEN];

char pidfilename[MAX_PID_FILENAME];
int pidfd;

sem_t semaphore;

PGconn *dbconn;
pthread_mutex_t mutex_dbconn;

static int run_as_daemon = 1;

static void term_func(int sig, siginfo_t *siginfo, void *context)
{
	(void)context;
	syslog(LOG_NOTICE, "%s: recorder term_func: %d", streaminterface.name, sig);
	if (siginfo != NULL)
	{
		syslog(LOG_NOTICE, "Sending PID: %ld, UID: %ld",
			(long)siginfo->si_pid, (long)siginfo->si_uid);
	}
	pstate = STATE_SHUTDOWN;
}

static uint64_t now_usec(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec*1000000ULL + tv.tv_usec;
}

static void sanitize_name(char *dst, size_t dst_len, const char *src)
{
	size_t pos = 0;

	if (dst_len == 0)
	{
		return;
	}

	for (size_t i = 0; src[i] != '\0' && pos < dst_len - 1; i++)
	{
		if (isalnum((unsigned char)src[i]) || src[i] == '_' || src[i] == '-')
		{
			dst[pos++] = src[i];
		}
		else
		{
			dst[pos++] = '_';
		}
	}
	dst[pos] = '\0';
}

static int build_recording_name(char *dst, size_t dst_len, const char *interfacename)
{
	time_t rawtime;
	struct tm tm_time;
	char timebuf[32];
	char clean_interface[64];

	time(&rawtime);
	if (localtime_r(&rawtime, &tm_time) == NULL)
	{
		return 1;
	}
	strftime(timebuf, sizeof(timebuf), "%Y%m%d_%H%M%S", &tm_time);
	sanitize_name(clean_interface, sizeof(clean_interface), interfacename);

	int len = snprintf(dst, dst_len, "%s_rec_%s_%ld",
		clean_interface, timebuf, (long)getpid());
	if (len < 0 || (size_t)len >= dst_len)
	{
		return 2;
	}
	return 0;
}

static int create_recording_directory(char *base_video_path,
	char *recording_name, char *recording_path, size_t recording_path_len)
{
	int len = snprintf(recording_path, recording_path_len, "%s/%s",
		base_video_path, recording_name);
	if (len < 0 || (size_t)len >= recording_path_len)
	{
		return 1;
	}

	if (mkdir(recording_path, 0775) == -1)
	{
		LOGMSG(LOG_ERR, "%s: mkdir(%s) failed: %s (%d)",
			streaminterface.name, recording_path, strerror(errno), errno);
		return 2;
	}

	return 0;
}

static int setup_signal_handlers(char *interfacename)
{
	struct sigaction sterm;
	memset(&sterm, 0, sizeof(struct sigaction));
	sterm.sa_sigaction = &term_func;
	sterm.sa_flags = SA_SIGINFO;

	if (sigaction(SIGTERM, &sterm, NULL) == -1)
	{
		LOGMSG(LOG_ERR, "%s: Error to setup SIGTERM signal handler (%s)",
			interfacename, strerror(errno));
		return 1;
	}

	struct sigaction sint;
	memset(&sint, 0, sizeof(struct sigaction));
	sint.sa_sigaction = &term_func;
	sint.sa_flags = SA_SIGINFO;

	if (sigaction(SIGINT, &sint, NULL) == -1)
	{
		LOGMSG(LOG_ERR, "%s: Error to setup SIGINT signal handler (%s)",
			interfacename, strerror(errno));
		return 2;
	}

	return 0;
}

int main(int argc, char **argv)
{
	char interfacename[41];
	char base_video_path[VIDEO_PATH_LEN];
	char recording_name[DB_MAX_NAME_LENGTH];
	char recording_path[VIDEO_PATH_LEN];
	pthread_t thrd_mc_receiver = 0;
	pthread_t thrd_statistic = 0;
	pthread_t thrd_pokepovmanager = 0;
	pthread_t thrd_traps = 0;
	pthread_t thrd_discwriter = 0;

	buffer_queue buffer_q_available[1];
	buffer_queue buffer_q_disc;
	e_process_state recorder_state[1];
	s_file_info writer_file;
	s_recorderstats recorderstats[1];
	s_recordings recording;

	int retval;
	int pthread_ret;
	int retval_mutex_stat = 1;
	int retval_mutex_traps = 1;
	int retval_mutex_available_buffer = 1;
	int retval_mutex_disc_buffer = 1;
	int retval_mutex_dbconn = 1;
	int retval_mutex_writer_file = 1;
	int retval_thrd_discwriter = 1;
	int retval_thrd_mc_receiver = 1;
	int retval_thrd_statistic = 1;
	int retval_thrd_pokepovmanager = 1;
	int retval_thrd_traps = 1;
	int retval_semaphore = 1;
	bool recording_started = false;

	memset(buffer_q_available, 0, sizeof(buffer_q_available));
	memset(&buffer_q_disc, 0, sizeof(buffer_q_disc));
	memset(&writer_file, 0, sizeof(writer_file));
	memset(recorderstats, 0, sizeof(recorderstats));
	memset(&recording, 0, sizeof(recording));
	memset(recording_name, 0, sizeof(recording_name));
	memset(recording_path, 0, sizeof(recording_path));
	memset(base_video_path, 0, sizeof(base_video_path));

	recorder_state[0] = DELAYER_STATE_LIVESTREAMING;
	pstate = STATE_STARTING;

	memset(&rudp_sequence_list, 0, sizeof(sequence_statistics));
	rudp_sequence_list.uninit = 1;
	gettimeofday(&rudp_sequence_list.last_tv, NULL);

	rt_stats.stat_bufferlevel = 0;
	rt_stats.stat_inbytes = 0;
	rt_stats.stat_outbytes = 0;
	rt_stats.stat_udpdrops = 0;
	rt_stats.laststattime = 0;

	parse_argument(argc, argv, VERSION, &run_as_daemon);
	strncpy(interfacename, argv[1], 40);
	interfacename[40] = '\0';

	snprintf(pidfilename, MAX_PID_FILENAME, PID_RECORDER_FILENAME_PATTERN, interfacename);
	if (access(pidfilename, F_OK) != -1)
	{
		syslog(LOG_ERR, "PID file %s already exists!", pidfilename);
		exit(1);
	}

	if (run_as_daemon)
	{
		daemonize();
	}

	pidfd = create_pid(pidfilename);
	if (pidfd == -1)
	{
		syslog(LOG_ERR, "Cant create pid file %s", pidfilename);
		exit(1);
	}

	dbconn = DBConnect();
	if (dbconn == NULL)
	{
		LOGMSG(LOG_ERR, "%s: failed to connect to database", interfacename);
		close_pid_file(pidfd, pidfilename);
		exit(0);
	}

	dbfetch_video_path(&dbconn, base_video_path);

	memset(&streaminterface, 0, sizeof(s_interface));
	retval = dbfetchstreaminterfacebyname(&dbconn, &streaminterface, interfacename);
	if (retval || streaminterface.id == 0)
	{
		LOGMSG(LOG_ERR, "%s: failed to fetch streaminterface (%d)",
			interfacename, retval);
		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}

	retval_mutex_traps = pthread_mutex_init(&mutex_traps, NULL);
	if (retval_mutex_traps != 0)
	{
		LOGMSG(LOG_ERR, "%s: Can not create trap mutex (%s %d)",
			interfacename, strerror(retval_mutex_traps), retval_mutex_traps);
		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}

	init_trap_queue();
	retval_thrd_traps = pthread_create(&thrd_traps, NULL, thread_traps, NULL);
	if (retval_thrd_traps != 0)
	{
		LOGMSG(LOG_ERR, "%s: Can not create traps thread (%s %d)",
			interfacename, strerror(retval_thrd_traps), retval_thrd_traps);
		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}

	retval_thrd_pokepovmanager = pthread_create(&thrd_pokepovmanager, NULL,
		thread_pokepovmanager, (void *)&streaminterface);
	if (retval_thrd_pokepovmanager != 0)
	{
		add_trap_to_queue(TRAP_RECORDER_FAILED_SETUP, "Can not setup povmanager communication");
		LOGMSG(LOG_ERR, "%s: Can not create pokepovmanager thread (%s - %d)",
			interfacename, strerror(retval_thrd_pokepovmanager),
			retval_thrd_pokepovmanager);
		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}

	retval_mutex_dbconn = pthread_mutex_init(&mutex_dbconn, NULL);
	if (retval_mutex_dbconn != 0)
	{
		add_trap_to_queue(TRAP_RECORDER_FAILED_SETUP, "Can not setup database mutex");
		LOGMSG(LOG_ERR, "%s: Can not create database mutex (%s %d)",
			interfacename, strerror(retval_mutex_dbconn), retval_mutex_dbconn);
		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}

	logtodb(&mutex_dbconn, dbconn, streaminterface.id, POVLOGLEVEL_INFO,
		"%s starting as recorder.", interfacename);
	add_trap_to_queue(TRAP_RECORDER_START, "starting as UDP Recorder");

	memset(&streaminterface.config, 0, sizeof(s_interfaceconfig));
	pthread_ret = pthread_mutex_lock(&mutex_dbconn);
	LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
	retval = dbfetchstreaminterfaceconfigbystreaminterfaceid(&dbconn,
		&streaminterface.config, streaminterface.id);
	pthread_ret = pthread_mutex_unlock(&mutex_dbconn);
	LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
	if (retval || streaminterface.config.id == 0)
	{
		add_trap_to_queue(TRAP_RECORDER_FAILED_SETUP, "Can not fetch config from database");
		LOGMSG(LOG_ERR, "%s: failed to fetch recorder config (%d)",
			interfacename, retval);
		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}

	if (strncmp(streaminterface.config.mode, "recorder", DB_MAX_MODE_LENGTH) != 0)
	{
		add_trap_to_queue(TRAP_RECORDER_FAILED_SETUP, "Mode is not recorder");
		LOGMSG(LOG_ERR, "%s: Mode is not recorder", interfacename);
		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}

	memset(&streaminterface.source, 0, DB_MAX_SOURCE*sizeof(s_interfacesource));
	pthread_ret = pthread_mutex_lock(&mutex_dbconn);
	LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
	retval = dbfetchstreaminterfacesourcebystreaminterfaceconfigid(&dbconn,
		&streaminterface.source[0], streaminterface.config.id);
	pthread_ret = pthread_mutex_unlock(&mutex_dbconn);
	LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
	if (retval)
	{
		add_trap_to_queue(TRAP_RECORDER_FAILED_SETUP, "Can not fetch source config from database");
		LOGMSG(LOG_ERR, "%s: failed to fetch source config (%d)",
			interfacename, retval);
		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}

	if (streaminterface.source[0].port == 0 ||
		strnlen(streaminterface.source[0].host, DB_MAX_HOSTNAME_LENGTH) < 3)
	{
		add_trap_to_queue(TRAP_RECORDER_FAILED_SETUP, "Error in source configuration");
		LOGMSG(LOG_ERR, "%s: Error in source configuration", interfacename);
		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}

	pthread_ret = pthread_mutex_lock(&mutex_dbconn);
	LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
	db_delete_streaminterfacelivestatistic(&dbconn, streaminterface.id);
	db_delete_recorderstats(&dbconn, streaminterface.config.id);
	pthread_ret = pthread_mutex_unlock(&mutex_dbconn);
	LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);

	if (build_recording_name(recording_name, sizeof(recording_name), interfacename))
	{
		add_trap_to_queue(TRAP_RECORDER_FAILED_SETUP, "Can not create recording name");
		LOGMSG(LOG_ERR, "%s: Can not create recording name", interfacename);
		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}

	if (create_recording_directory(base_video_path, recording_name,
		recording_path, sizeof(recording_path)))
	{
		add_trap_to_queue(TRAP_RECORDER_FAILED_SETUP, "Can not create recording directory");
		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}
	strncpy(video_path, recording_path, VIDEO_PATH_LEN - 1);
	video_path[VIDEO_PATH_LEN - 1] = '\0';

	recording.streaminterface_id = streaminterface.id;
	strncpy(recording.name, recording_name, DB_MAX_NAME_LENGTH - 1);
	recording.e_state = INIT;
	strncpy(recording.state, "init", DB_MAX_STATE - 1);
	strncpy(recording.sourceaddress, streaminterface.source[0].host,
		DB_MAX_HOSTNAME_LENGTH - 1);
	recording.sourceport = streaminterface.source[0].port;

	pthread_ret = pthread_mutex_lock(&mutex_dbconn);
	LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
	retval = db_insert_recordings(&dbconn, &recording);
	if (retval == 0)
	{
		retval = db_fetch_recordingbyname(&dbconn, &recording, recording_name);
		if (retval == 0 && recording.id == 0)
		{
			retval = 1;
		}
	}
	pthread_ret = pthread_mutex_unlock(&mutex_dbconn);
	LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
	if (retval)
	{
		add_trap_to_queue(TRAP_RECORDER_FAILED_SETUP, "Can not create recording database row");
		LOGMSG(LOG_ERR, "%s: Can not create recording database row (%d)",
			interfacename, retval);
		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}

	if (setup_signal_handlers(interfacename))
	{
		add_trap_to_queue(TRAP_RECORDER_FAILED_SETUP, "Can not setup signal handlers");
		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}

	initBufferElements(&buffer_q_available[0].buffer_elements);
	buffer_q_available[0].no_buffer_elements = BUFFER_SIZE;

	retval_mutex_available_buffer = pthread_mutex_init(&buffer_q_available[0].mutex, NULL);
	if (retval_mutex_available_buffer != 0)
	{
		add_trap_to_queue(TRAP_RECORDER_FAILED_SETUP, "Can not create available buffer mutex");
		LOGMSG(LOG_ERR, "%s: Can not create available buffer mutex (%s - %d)",
			interfacename, strerror(retval_mutex_available_buffer),
			retval_mutex_available_buffer);
		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}

	retval_mutex_stat = pthread_mutex_init(&mutex_stat, NULL);
	if (retval_mutex_stat != 0)
	{
		add_trap_to_queue(TRAP_RECORDER_FAILED_SETUP, "Can not create statistic mutex");
		LOGMSG(LOG_ERR, "%s: Can not create statistic mutex (%s - %d)",
			interfacename, strerror(retval_mutex_stat), retval_mutex_stat);
		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}

	rudp_sequence_list.num_paths = 1;
	rudp_sequence_list.mutex_stat = &mutex_stat;

	recorderstats[0].streaminterface_id = streaminterface.config.id;
	recorderstats[0].e_state = INIT;
	strncpy(recorderstats[0].state, "init", DB_MAX_STATE - 1);

	s_statarg statarg;
	memset(&statarg, 0, sizeof(statarg));
	statarg.process_mode = RECORDER;
	statarg.r_stats = recorderstats;
	statarg.id = streaminterface.id;
	statarg.num_paths = 1;
	strncpy(statarg.mode, streaminterface.config.mode, DB_MAX_MODE_LENGTH);
	strncpy(statarg.name, streaminterface.name, DB_MAX_NAME_LENGTH);
	statarg.config = &(streaminterface.config);
	statarg.source = streaminterface.source;
	statarg.sink = NULL;
	statarg.dbconn = &dbconn;
	statarg.mutex_dbconn = &mutex_dbconn;
	statarg.mutex_stat = &mutex_stat;
	statarg.rt_stats = &rt_stats;
	statarg.rudp_sequence_list = &rudp_sequence_list;

	retval_thrd_statistic = pthread_create(&thrd_statistic, NULL,
		thread_statistic, (void *)&statarg);
	if (retval_thrd_statistic != 0)
	{
		add_trap_to_queue(TRAP_RECORDER_FAILED_SETUP, "Can not setup statistics");
		LOGMSG(LOG_ERR, "%s: Cant create statistic thread (%s - %d)",
			interfacename, strerror(retval_thrd_statistic),
			retval_thrd_statistic);
		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}

	retval_semaphore = sem_init(&semaphore, 0, 0);
	if (retval_semaphore == -1)
	{
		add_trap_to_queue(TRAP_RECORDER_FAILED_SETUP, "Can not initiate semaphore");
		LOGMSG(LOG_ERR, "%s: cant initiate semaphore %s",
			interfacename, strerror(errno));
		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}

	retval_mutex_disc_buffer = pthread_mutex_init(&buffer_q_disc.mutex, NULL);
	if (retval_mutex_disc_buffer != 0)
	{
		add_trap_to_queue(TRAP_RECORDER_FAILED_SETUP, "Can not create disc buffer mutex");
		LOGMSG(LOG_ERR, "%s: Can not create disc buffer mutex (%s - %d)",
			interfacename, strerror(retval_mutex_disc_buffer),
			retval_mutex_disc_buffer);
		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}

	retval_mutex_writer_file = pthread_mutex_init(&writer_file.mutex_file, NULL);
	if (retval_mutex_writer_file != 0)
	{
		add_trap_to_queue(TRAP_RECORDER_FAILED_SETUP, "Can not create writer file mutex");
		LOGMSG(LOG_ERR, "%s: Can not create writer file mutex (%s - %d)",
			interfacename, strerror(retval_mutex_writer_file),
			retval_mutex_writer_file);
		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}
	writer_file.id = 0;
	writer_file.status = UNPARSED;

	thread_disc_writer_arg writerarg;
	memset(&writerarg, 0, sizeof(writerarg));
	writerarg.state = recorder_state;
	writerarg.name = streaminterface.name;
	writerarg.streaminterface_id = streaminterface.config.streaminterface_id;
	writerarg.writer_file = &writer_file;
	writerarg.buffer_q_available = &buffer_q_available[0];
	writerarg.buffer_q_disc = &buffer_q_disc;

	retval_thrd_discwriter = pthread_create(&thrd_discwriter, NULL,
		thread_discwriter, (void *)&writerarg);
	if (retval_thrd_discwriter != 0)
	{
		add_trap_to_queue(TRAP_RECORDER_FAILED_SETUP, "Can not setup discwriter thread");
		LOGMSG(LOG_ERR, "%s: cant create discwriter thread (%s - %d)",
			interfacename, strerror(retval_thrd_discwriter),
			retval_thrd_discwriter);
		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}

	thread_mc_receiver_arg mc_receiver_arg;
	memset(&mc_receiver_arg, 0, sizeof(thread_mc_receiver_arg));
	strncpy(mc_receiver_arg.multicast_address, streaminterface.source[0].host,
		sizeof(mc_receiver_arg.multicast_address) - 1);
	mc_receiver_arg.multicast_port = streaminterface.source[0].port;
	mc_receiver_arg.multicast_ttl = 0;
	mc_receiver_arg.buffer_timeout = 10;
	mc_receiver_arg.state = recorder_state;
	mc_receiver_arg.rawudp = 0;
	mc_receiver_arg.stat_last_timestamp = 0;
	mc_receiver_arg.name = streaminterface.name;
	mc_receiver_arg.pathno = 0;
	mc_receiver_arg.no_sink = 0;
	mc_receiver_arg.buffer_q_available = buffer_q_available;
	mc_receiver_arg.buffer_q_disc = &buffer_q_disc;
	mc_receiver_arg.buffer_q_send = NULL;
	mc_receiver_arg.dbconn = &dbconn;
	mc_receiver_arg.mutex_dbconn = &mutex_dbconn;
	mc_receiver_arg.rudp_sequence_list = &rudp_sequence_list;
	mc_receiver_arg.mutex_stat = &mutex_stat;
	mc_receiver_arg.rt_stats = &rt_stats;

	retval_thrd_mc_receiver = pthread_create(&thrd_mc_receiver, NULL,
		thread_mc_receiver, (void *)&mc_receiver_arg);
	if (retval_thrd_mc_receiver != 0)
	{
		add_trap_to_queue(TRAP_RECORDER_FAILED_SETUP, "Can not setup multicast input");
		LOGMSG(LOG_ERR, "%s: cant create receiver thread (%s - %d)",
			interfacename, strerror(retval_thrd_mc_receiver),
			retval_thrd_mc_receiver);
		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}

	while (pstate != STATE_SHUTDOWN)
	{
		pthread_ret = pthread_mutex_lock(&mutex_stat);
		LOG_PTHREAD_ERROR_CONTINUE(pthread_ret, interfacename, __LINE__);
		pthread_ret = pthread_mutex_lock(&writer_file.mutex_file);
		LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);

		if (writer_file.start_time != 0 && recording_started == false)
		{
			recording.recordingstart_timestamp = writer_file.start_time;
			recording.e_state = RECORDING;
			strncpy(recording.state, "recording", DB_MAX_STATE - 1);
			recorderstats[0].startrecordat_timestamp = writer_file.start_time;
			recorderstats[0].startpos_timestamp = writer_file.start_time;
			recorderstats[0].e_state = RECORDING;
			strncpy(recorderstats[0].state, "recording", DB_MAX_STATE - 1);

			pthread_ret = pthread_mutex_lock(&mutex_dbconn);
			LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
			db_update_recordings(&dbconn, &recording);
			pthread_ret = pthread_mutex_unlock(&mutex_dbconn);
			LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);

			recording_started = true;
		}

		if (recording_started)
		{
			recorderstats[0].lastpos_timestamp = writer_file.end_time;
			recorderstats[0].currpos_timestamp = writer_file.end_time;
		}

		pthread_ret = pthread_mutex_unlock(&writer_file.mutex_file);
		LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
		pthread_ret = pthread_mutex_unlock(&mutex_stat);
		LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);

		usleep(100000);
	}

SHUTDOWN:
	pstate = STATE_SHUTDOWN;

	if (retval_thrd_mc_receiver == 0)
	{
		pthread_join(thrd_mc_receiver, NULL);
	}

	if (retval_semaphore == 0)
	{
		sem_post(&semaphore);
	}

	if (retval_thrd_discwriter == 0)
	{
		pthread_join(thrd_discwriter, NULL);
	}

	if (retval_thrd_statistic == 0)
	{
		pthread_join(thrd_statistic, NULL);
	}
	if (retval_thrd_traps == 0)
	{
		pthread_join(thrd_traps, NULL);
	}
	if (retval_thrd_pokepovmanager == 0)
	{
		pthread_join(thrd_pokepovmanager, NULL);
	}

	if (recording.id != 0 && retval_mutex_dbconn == 0)
	{
		uint64_t end_time = now_usec();
		if (retval_mutex_writer_file == 0)
		{
			pthread_ret = pthread_mutex_lock(&writer_file.mutex_file);
			LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
			if (writer_file.end_time != 0)
			{
				end_time = writer_file.end_time;
			}
			pthread_ret = pthread_mutex_unlock(&writer_file.mutex_file);
			LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
		}

		recording.recordingend_timestamp = end_time;
		recording.e_state = DONE;
		strncpy(recording.state, "done", DB_MAX_STATE - 1);
		recorderstats[0].lastpos_timestamp = end_time;
		recorderstats[0].currpos_timestamp = end_time;
		recorderstats[0].e_state = DONE;
		strncpy(recorderstats[0].state, "done", DB_MAX_STATE - 1);

		pthread_ret = pthread_mutex_lock(&mutex_dbconn);
		LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
		db_update_recordings(&dbconn, &recording);
		db_insert_recorderstats(&dbconn, &recorderstats[0]);
		pthread_ret = pthread_mutex_unlock(&mutex_dbconn);
		LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
	}

	if (retval_mutex_stat == 0)
	{
		pthread_mutex_destroy(&mutex_stat);
	}
	if (retval_mutex_traps == 0)
	{
		deinit_trap_queue();
		pthread_mutex_destroy(&mutex_traps);
	}
	if (retval_mutex_dbconn == 0)
	{
		pthread_mutex_destroy(&mutex_dbconn);
	}
	if (retval_mutex_available_buffer == 0)
	{
		freeBufferElements(&buffer_q_available[0].buffer_elements);
		pthread_mutex_destroy(&buffer_q_available[0].mutex);
	}
	if (retval_mutex_disc_buffer == 0)
	{
		pthread_mutex_destroy(&buffer_q_disc.mutex);
	}
	if (retval_mutex_writer_file == 0)
	{
		pthread_mutex_destroy(&writer_file.mutex_file);
	}
	if (retval_semaphore == 0)
	{
		sem_destroy(&semaphore);
	}

	if (dbconn != NULL)
	{
		if (streaminterface.id != 0)
		{
			logtodb_nolock(dbconn, streaminterface.id, POVLOGLEVEL_INFO,
				"%s stopped as recorder. Recording: %s",
				interfacename, recording_name);
		}
		DBClose(dbconn);
	}
	if (pidfd > 0)
	{
		close_pid_file(pidfd, pidfilename);
	}

	if (streaminterface.id != 0)
	{
		trap_send_exit(TRAP_RECORDER_STOP, "stopped.");
	}

	return 0;
}
