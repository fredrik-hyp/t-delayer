#include "dbdelayers.h"
#include "dbsysteminfo.h"
#include "common-db.h"
#include "thread_statistic.h"
#include "thread_traps.h"
#include "thread_pokepovmanager.h"
#include "buffer.h"

#include "stat_func.h"
#include "print_struct.h"

#include "states.h"

#include <assert.h>
#ifdef __linux__
#include <syscall.h>
#endif
#include <semaphore.h>
#include <sched.h>

#include "delayer.h"

// =====  Variables shared between files  =====
// buffer.c
uint8_t packetcountersinlist[MAX_PACKET_COUNTER+1];

// states.h
volatile PROCESS_STATE pstate = STATE_STARTING;

// thread_statistic.h
pthread_mutex_t mutex_stat;
s_runtimestat rt_stats;
sequence_statistics rudp_sequence_list;

// thread_traps.h
pthread_mutex_t mutex_traps;
trapelement *trapqueue;

// pov-database repository dbstreaminterfaces.h
s_interface streaminterface;

// shared with delayer_other.c
char video_path[VIDEO_PATH_LEN];

// ===== Global variabel in this file =====
char	pidfilename[MAX_PID_FILENAME];
int	pidfd;

//pthread_mutex_t mutex_disc_signal;
//pthread_cond_t condt_disc_signal;

// To communicate between multicast and rudp / talkback thread 
sem_t semaphore;

// Connection to Postgresql database
PGconn *dbconn;
pthread_mutex_t mutex_dbconn;

static int run_as_daemon = 1;

// Common between interfaces

// Stat variables

static uint64_t stat_print;

// === Private functions for rudp_transmitter.c ===

// New function
static int reread_config_from_database;
static void reread_config_signal_func(int sig, siginfo_t *siginfo, void *context)
{
	reread_config_from_database = 1;
}

static int reread_delay_from_database = 0;
static void read_delay_while_running(int sig, siginfo_t *siginfo, void *context)
{
	reread_delay_from_database = 1;
}

// From rudp transmitter.c
static void term_func(int sig, siginfo_t *siginfo, void *context)
{
	// Syslog shall be removed - not appropriate in a sig handler
	syslog(LOG_NOTICE, "%s: term_func: %d", streaminterface.name, sig);
	syslog(LOG_NOTICE, "Sending PID: %ld, UID: %ld\n",
		   (long)siginfo->si_pid, (long)siginfo->si_uid);
	pstate = STATE_SHUTDOWN;
}

void check_for_long_delay_files(uint32_t *first_id, uint32_t *last_id, char *name)
{
	DIR *directory;
	struct dirent *dent;
	char pattern[30];
	int len;
	uint32_t min = 0, max = 0;
	bool found_file = false;

	if (first_id == NULL || last_id == NULL || name == NULL)
		return;
	len = snprintf(pattern, 30, "%s_chunk_", name);
	directory = opendir(video_path);

	if (directory == NULL)
	{
		// Make sure this are zero to indicate no files are fond.
		*first_id = *last_id = 0;
		return;
	}

	// Loop through every filename in the /home/video/ directory
	while ((dent = readdir(directory)) != NULL)
	{
		// Test if some filename starts for example with st1_chunk_
		if (strncmp(dent->d_name, pattern, len) != 0)
		{
			// If it doesnt loop to next file
			continue;
		}

		int dot_pos = len;
		char number[255];
		for (int i = 0; dot_pos < 255; dot_pos++, i++)
		{
			number[i] = dent->d_name[dot_pos];
			if (dent->d_name[dot_pos] == '.')
			{
				number[i] = 0;
				break;
			}
		}

		uint32_t tmpValue = (uint32_t)strtol(number, (char **)NULL, 10);
		if (tmpValue == 0 && errno != 0)
		{
			LOGMSG(LOG_WARNING,
				"check_for_long_delay_files(%s): conversion failed for %s (%s %d)",
				number, dent->d_name,
				strerror(errno), errno);
		}
		if (found_file == false)
		{
			min = max = tmpValue;
			found_file = true;
		}
		else if (tmpValue > max)
		{
			max = tmpValue;
		}
		else if (tmpValue <= min)
		{
			min = tmpValue;
		}
	}

	if (found_file == false)
	{
		// If no files for this interface are found - make sure write&read_chunk_id are zero
		*first_id = *last_id = 0;
	}
	else
	{
		*last_id = max+1;
		*first_id = min;
	}

#ifdef NODAEMON_DEBUG
	fprintf(stderr, "-----------------------------\n");
	fprintf(stderr, "first_id file: %s%u\n",
		pattern, *first_id);
	fprintf(stderr, "last_id file %s%u\n",
		pattern, *last_id);
	fprintf(stderr, "-----------------------------\n");

	if (*first_id != 0)
	{
		s_file_info f;
		int ret_val;
		f.id = *first_id;
		ret_val = taste_file(name, &f);
		fprintf(stderr, 
			"check_for_long_delay_files %"PRIu64" - %"PRIu64" %d\n",
			f.start_time, f.end_time, ret_val);
	}
	if (*last_id != 0)
	{
		s_file_info f;
		int ret_val;
		f.id = *last_id;
		ret_val = taste_file(name, &f);
		fprintf(stderr,
			"check_for_long_delay_files %"PRIu64" - %"PRIu64" %d\n",
			f.start_time, f.end_time, ret_val);
	}
#endif

	closedir (directory);
}

void copy_process_state_string(e_process_state dstate, char *dest)
{
	if (dest == NULL)
	{
		return;
	}

	switch (dstate)
	{
	case DELAYER_STATE_PAUSE:
		strncpy(dest, "Pause\0", DB_MAX_MODE_LENGTH);
		return;
	case DELAYER_STATE_LIVESTREAMING:
		strncpy(dest, "Live\0", DB_MAX_MODE_LENGTH);
		return;
	case DELAYER_STATE_DELAYED:
		strncpy(dest, "Play\0", DB_MAX_MODE_LENGTH);
		return;
	}
	return;
}

const char *process_state_to_string(e_process_state dstate)
{
	switch (dstate)
	{
	case DELAYER_STATE_PAUSE:
		return "Pause";
	case DELAYER_STATE_LIVESTREAMING:
		return "Live";
	case DELAYER_STATE_DELAYED:
		return "Play";
	}
	return "";
}

#ifdef NODAEMON_DEBUG
const char *timestamp_to_str(uint64_t time, char *buf, int size)
{
	time_t epoch = time;
	struct tm *epoch_tm;

	epoch_tm = localtime(&epoch);
	strftime(buf, size, "%y-%m-%d %H:%M:%S", epoch_tm);
	return buf;
}
const char *micro_timestamp_to_str(uint64_t time, char *buf, int size)
{
	time_t epoch = time/1000000;
	struct tm *epoch_tm;

	epoch_tm = localtime(&epoch);
	strftime(buf, size, "%y-%m-%d %H:%M:%S", epoch_tm);
	return buf;
}
#endif


int main(int argc, char **argv)
{
	e_process_state dstate[MAX_INTERFACE_PATH+1];

	char interfacename[41];
	pthread_t thrd_mc_receiver=0;
	pthread_t thrd_statistic=0;
	pthread_t thrd_pokepovmanager=0;
	pthread_t thrd_traps=0;
	pthread_t thrd_discwriter=0;
	pthread_t thrd_disccleaner=0;
	pthread_t thrd_mc_sender[MAX_INTERFACE_PATH];
	pthread_t thrd_discreader[MAX_INTERFACE_PATH];
	s_pause_arg pause_info[MAX_INTERFACE_PATH];

	uint64_t total_disc_buffer_time = 0;

	pthread_mutex_t mutex_discclean_signal;
	pthread_cond_t condt_discclean_signal;
	thread_mc_arg mc_sender_arg[MAX_INTERFACE_PATH];
	thread_disc_reader_arg reader_arg[MAX_INTERFACE_PATH];

	buffer_queue buffer_q_send[MAX_INTERFACE_PATH];
	buffer_queue buffer_q_disc;
	buffer_queue buffer_q_available[MAX_INTERFACE_PATH+1];

	//retval_mutex_disc_signal=1,
//	retval_condt_disc_signal=1,

	int retval_mutex_stat=1,
			retval_mutex_traps=1,
			retval_mutex_available_buffer[MAX_INTERFACE_PATH+1],
			retval_mutex_send_buffer[MAX_INTERFACE_PATH],
			retval_mutex_disc_buffer=1,
			retval_mutex_dbconn=1,
			retval_thrd_discwriter=1,
			retval_thrd_disccleaner=1,
			retval_condt_discclean_signal=1,
			retval_mutex_discclean_signal=1,
			retval_thrd_mc_receiver=1,
			retval_thrd_statistic=1,
			retval_thrd_pokepovmanager=1,
			retval_thrd_traps=1,
			retval_semaphore=1,
			retval_mutex_newest_file=1,
			retval_mutex_oldest_file=1,
			retval_mutex_pause_info[MAX_INTERFACE_PATH],
			retval_thrd_mc_sender[MAX_INTERFACE_PATH],
			retval_thrd_discreader[MAX_INTERFACE_PATH],
			retval_mutex_read_file[MAX_INTERFACE_PATH];

	int i;
	unsigned int command_poll_ticks = 0;
	int retval, pthread_ret;
	struct timeval tv;
	uint64_t now;

	s_file_info oldest_file, newest_file, read_file[MAX_INTERFACE_PATH];
	s_delayerstats d_stats[MAX_INTERFACE_PATH];
	s_delayercoms delayercom[1];

	uint64_t start_play_at[MAX_INTERFACE_PATH], play_to[MAX_INTERFACE_PATH];

#ifdef NODAEMON_DEBUG
	char buf1[80];
	char buf2[80];
	char buf3[80];
#endif

	// === PROGRAM START ===
	memset(&oldest_file, 0, sizeof(s_file_info));
	memset(&newest_file, 0, sizeof(s_file_info));
	memset(read_file, 0, MAX_INTERFACE_PATH*sizeof(s_file_info));
	memset(thrd_mc_sender, 0, MAX_INTERFACE_PATH*sizeof(pthread_t));
	memset(thrd_discreader, 0, MAX_INTERFACE_PATH*sizeof(pthread_t));
	memset(pause_info, 0, MAX_INTERFACE_PATH*sizeof(s_pause_arg));
	memset(mc_sender_arg, 0, MAX_INTERFACE_PATH*sizeof(thread_mc_arg));
	memset(reader_arg, 0, MAX_INTERFACE_PATH*sizeof(thread_disc_reader_arg));
	memset(delayercom, 0, sizeof(delayercom));

	memset(buffer_q_available, 0, (MAX_INTERFACE_PATH+1)*sizeof(buffer_queue));
	memset(&buffer_q_disc, 0, sizeof(buffer_queue));
	memset(buffer_q_send, 0, MAX_INTERFACE_PATH*sizeof(buffer_queue));
	for (i = 0; i < MAX_INTERFACE_PATH; i++)
	{
		dstate[i] = DELAYER_STATE_LIVESTREAMING;
		retval_mutex_available_buffer[i] = 1;
		retval_mutex_send_buffer[i] = 1;
		retval_thrd_mc_sender[i] = 1;
		retval_thrd_discreader[i] = 1;
		retval_mutex_read_file[i] = 1;
		retval_mutex_pause_info[i] = 1;
		start_play_at[i] = UINT64_MAX;
		play_to[i] = UINT64_MAX ;

	}
	dstate[MAX_INTERFACE_PATH] = DELAYER_STATE_LIVESTREAMING;
	retval_mutex_available_buffer[MAX_INTERFACE_PATH] =  1;


	reread_config_from_database = 0;

	// Setup rudp_sequence_list
	memset(&rudp_sequence_list, 0, sizeof(sequence_statistics));
	rudp_sequence_list.uninit = 1;
	gettimeofday(&tv, NULL);
	rudp_sequence_list.last_tv = tv;

	// Statistic variables
	rt_stats.stat_bufferlevel = 0;
	rt_stats.stat_inbytes = 0;
	rt_stats.stat_outbytes = 0;
	rt_stats.stat_udpdrops = 0;
	rt_stats.laststattime = 0;

	stat_print=0;

	pstate = STATE_STARTING;

	// === Parse argument list ===

	parse_argument(argc, argv, VERSION, &run_as_daemon);
	strncpy(interfacename, argv[1], 40);

	// === PID File ===
	snprintf(pidfilename, MAX_PID_FILENAME, PID_DELAYER_FILENAME_PATTERN, interfacename);

	// Check if PID file exists (already running)
	if (access(pidfilename, F_OK) != -1)
	{
		syslog(LOG_ERR, "PID file %s already exists!",
			   pidfilename);
		trap_send_exit(TRAP_DELAYER_STOP, "stopped.");
		exit(1);
	}

	// === Daemonize ===
	if (run_as_daemon)
	{
		daemonize();
	}

	// === Create pid file ===
	pidfd = create_pid(pidfilename);
	if (pidfd == -1)
	{
		syslog(LOG_ERR, "Cant create pid file %s",
			   pidfilename);
		trap_send_exit(TRAP_DELAYER_STOP, "stopped.");
		exit(1);
	}

	// === Connect to database and fetch this interface config ===
	dbconn = DBConnect();
	if (dbconn == NULL)
	{
		LOGMSG(LOG_ERR, "%s: failed to connect to database",
			   interfacename);

		close_pid_file(pidfd, pidfilename);
		trap_send_exit(TRAP_DELAYER_STOP, "stopped.");
		exit(0);
	}

	// === Fetch video path ===
	dbfetch_video_path(&dbconn, video_path);

	// === Fetch streaminterface ===
	memset(&streaminterface, 0, sizeof(s_interface));
	retval = dbfetchstreaminterfacebyname(&dbconn, &streaminterface, interfacename);
	if (retval)
	{
		LOGMSG(LOG_ERR, "%s: failed to fetch from database (dbfetchstreaminterfacebyname %d)",
			   interfacename, retval);

		DBClose(dbconn);
		close_pid_file(pidfd, pidfilename);
		trap_send_exit(TRAP_DELAYER_STOP, "stopped.");
		exit(0);
	}

	if (streaminterface.id == 0)
	{
		LOGMSG(LOG_ERR, "%s: Error fetching interface from database (streaminterface.id == 0)",
			   interfacename);

		DBClose(dbconn);
		close_pid_file(pidfd, pidfilename);
		trap_send_exit(TRAP_DELAYER_STOP, "stopped.");
		exit(0);
	}

	// Cleanup statistic from previous session
	retval = db_delete_streaminterfacelivestatistic(&dbconn, streaminterface.id);
	retval = db_delete_delayerstats(&dbconn, streaminterface.id);

	// Cleanup delayercoms from previous configuration
	retval = db_delete_delayercoms(&dbconn, streaminterface.id);

	// Dont log if this fails. 
	

	// === Setup trap queue ===
	retval_mutex_traps = pthread_mutex_init(&mutex_traps, NULL);
	if (retval_mutex_traps != 0)
	{
		LOGMSG(LOG_ERR, "%s: Can not create trap mutex (%s %d)",
			   interfacename,
			   strerror(retval_mutex_traps),
			   retval_mutex_traps);

		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}

	init_trap_queue();
	retval_thrd_traps = pthread_create(&thrd_traps, NULL, thread_traps, NULL);
	if (retval_thrd_traps != 0)
	{
		LOGMSG(LOG_ERR, "%s: Can not create traps thread (%s %d)",
			   interfacename,
			   strerror(retval_thrd_traps),
			   retval_thrd_traps);

		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}

	// === Create povmanager thread ===
	retval_thrd_pokepovmanager = pthread_create(&thrd_pokepovmanager, NULL, thread_pokepovmanager, (void *)&streaminterface);
	if (retval_thrd_pokepovmanager != 0)
	{
		add_trap_to_queue(TRAP_DELAYER_FAILED_SETUP, "Can not setup povmanager communication");
		LOGMSG(LOG_ERR, "%s: Can not create pokepovmanager thread (%s - %d)",
			   interfacename,
			   strerror(retval_thrd_pokepovmanager),
			   retval_thrd_pokepovmanager);

		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}

	// === Init dbmutex ===
	retval_mutex_dbconn = pthread_mutex_init(&mutex_dbconn, NULL);
	if (retval_mutex_dbconn != 0)
	{
		add_trap_to_queue(TRAP_DELAYER_FAILED_SETUP, "Can not setup mutex");
		LOGMSG(LOG_ERR, "%s: Can not create database mutex (%s %d)",
			   interfacename,
			   strerror(retval_mutex_dbconn),
			   retval_mutex_dbconn);

		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}

	// === Add STARTUP database record ===
	logtodb(&mutex_dbconn, dbconn, streaminterface.id, POVLOGLEVEL_INFO,
				   "%s starting as delayer.", interfacename);

	if (!PQisthreadsafe())
	{
		logtodb(&mutex_dbconn, dbconn, streaminterface.id, POVLOGLEVEL_INFO,
					   "Database handling is NOT thread safe!");
	}

	// === Send STARTUP SNMP message ===
	add_trap_to_queue(TRAP_DELAYER_START, "starting as UDP Delayer");

#ifdef NODAEMON_DEBUG
	LOGMSG(LOG_NOTICE, "(%" PRIu64 ") (%s) (%s) (%s)\n", streaminterface.id,
		   streaminterface.interfacetype, streaminterface.name, streaminterface.usertext);
#endif

	// === Populate config ===
	memset(&streaminterface.config, 0, sizeof(s_interfaceconfig));
	pthread_ret = pthread_mutex_lock(&mutex_dbconn);
	LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
	retval = dbfetchstreaminterfaceconfigbystreaminterfaceid(&dbconn, &streaminterface.config, streaminterface.id);
	pthread_ret = pthread_mutex_unlock(&mutex_dbconn);
	LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
	if (retval)
	{
		add_trap_to_queue(TRAP_DELAYER_FAILED_SETUP, "Can not fetch config from database");
		syslog(LOG_ERR, "%s: failed to fetch from database (dbfetchstreaminterfaceconfigbystreaminterfaceid %d)",
			   interfacename, retval);

		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}

	// === Check config content ===
	if (streaminterface.config.id == 0)
	{
		add_trap_to_queue(TRAP_DELAYER_FAILED_SETUP, "Can not fetch config from database");
		LOGMSG(LOG_ERR, "%s: Error fetching interface config from database",
			   interfacename);

		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}

	// === Check mode is delayer ===
	if (strncmp(streaminterface.config.mode, "delayer", DB_MAX_MODE_LENGTH) != 0)
	{
		add_trap_to_queue(TRAP_DELAYER_FAILED_SETUP, "Mode is not delayer");

		LOGMSG(LOG_ERR, "%s: Mode is not delayer",
			   interfacename);

		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}

	// === Get source from database ===
	memset(&streaminterface.source, 0, DB_MAX_SOURCE*sizeof(s_interfacesource));
	pthread_ret = pthread_mutex_lock(&mutex_dbconn);
	LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
	retval = dbfetchstreaminterfacesourcebystreaminterfaceconfigid(&dbconn,
			&streaminterface.source[0], streaminterface.config.id);
	pthread_ret = pthread_mutex_unlock(&mutex_dbconn);
	LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
	if (retval)
	{
		add_trap_to_queue(TRAP_DELAYER_FAILED_SETUP, "Can not fetch source config from database");
		LOGMSG(LOG_ERR, "%s: failed to fetch from database (dbfetchstreaminterfacesourcebystreaminterfaceconfigid %d)",
			   interfacename, retval);

		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}

	if (streaminterface.source[0].priority > 1000)
	{
		LOGMSG(LOG_WARNING, "%s: Warning - use priority as constant RTT %d",
			interfacename, streaminterface.source[0].priority);
	}

#ifdef NODAEMON_DEBUG
	LOGMSG(LOG_NOTICE, "Source: %s:%d\n",
		   streaminterface.source[0].host,
			streaminterface.source[0].port);
#endif

	// === Get sink from database ===
	memset(&streaminterface.sink, 0, DB_MAX_SINK*sizeof(s_interfacesink));
	pthread_ret = pthread_mutex_lock(&mutex_dbconn);
	LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
	retval = dbfetchstreaminterfacesinkbystreaminterfaceconfigid(&dbconn, &streaminterface.sink[0], streaminterface.config.id);
	pthread_ret = pthread_mutex_unlock(&mutex_dbconn);
	LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
	if (retval)
	{
		add_trap_to_queue(TRAP_DELAYER_FAILED_SETUP, "Can not fetch sink config from database");
		LOGMSG(LOG_ERR, "%s: failed to fetch from database (dbfetchstreaminterfacesinkbystreaminterfaceconfigid %d)",
			   interfacename, retval);

		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}
#ifdef NODAEMON_DEBUG
	LOGMSG(LOG_NOTICE, "BASE INTERFACE Sink: %s:%d\n",
		   streaminterface.sink[0].host,
			streaminterface.sink[0].port);
#endif

	// === Get sink backup from database ===
	retval = dbfetch_streaminterfacebackupsink_by_streaminterfacesinkid(&dbconn, &streaminterface.sink[1],
		&streaminterface.num_backup_sink, streaminterface.sink[0].id);

	// Test if fetch failed or if number of configured streaminterface are more than MAX_INTERFACE_PATH exit
	if (retval || streaminterface.num_backup_sink+1 > MAX_INTERFACE_PATH)
	{
		if (retval)
		{
			add_trap_to_queue(TRAP_DELAYER_FAILED_SETUP,
				"Can not fetch backupsink config from database");
			logtodb_nolock(dbconn, streaminterface.id, POVLOGLEVEL_INFO,
				"Failed to fetch backupsink config");
			LOGMSG(LOG_ERR,
				"%s: failed dbfetch_streaminterfacebackupsink_by_streaminterfacesinkid %d",
				interfacename, retval);
		}
		else
		{
			add_trap_to_queue(TRAP_DELAYER_FAILED_SETUP,
				"Too many interfaces are configured");
			logtodb_nolock(dbconn, streaminterface.id, POVLOGLEVEL_INFO,
				"Too many interface are configured");
			LOGMSG(LOG_ERR,
				"%s: Too many interfaces are configured",
				interfacename);
		}

		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}

	// Sanity control of network parameters
	for (i = 0; i < streaminterface.num_backup_sink+1; i++)
	{
		if (streaminterface.sink[i].port == 0 ||
			strnlen(streaminterface.sink[i].host, DB_MAX_HOSTNAME_LENGTH) < 3)
		{
			if (streaminterface.sink[i].port == 0)
			{
				add_trap_to_queue(TRAP_DELAYER_FAILED_SETUP,
					"Error in Port configuration");

				logtodb_nolock(dbconn, streaminterface.id, POVLOGLEVEL_INFO,
					"%s[%d]: Error in Port configuration.",
					streaminterface.name, i);

				LOGMSG(LOG_ERR,
					"%s[%d]: Error in Port configuration.",
					streaminterface.name, i);
			}
			else
			{
				add_trap_to_queue(TRAP_DELAYER_FAILED_SETUP,
					"Error in IP configuration");

				logtodb_nolock(dbconn, streaminterface.id, POVLOGLEVEL_INFO,
					"%s[%d]: Error in IP configuration.",
					streaminterface.name, i);
				LOGMSG(LOG_ERR,
					"%s[%d]: Error in IP configuration.",
					streaminterface.name, i);
			}
			pstate = STATE_SHUTDOWN;
			goto SHUTDOWN;
		}
		// Fix maxbps to be in bytes instead of bits per second
		// to save conversiontime in streamingphase
		streaminterface.sink[i].maxbps = streaminterface.sink[i].maxbps / 8;
	}

#ifdef NODAEMON_DEBUG
	for (i = 1; i < streaminterface.num_backup_sink+1; i++)
	{
	LOGMSG(LOG_NOTICE, "Sink[%d]: %s:%d\n",
		i,
		streaminterface.sink[i].host,
		streaminterface.sink[i].port);
	}
#endif


	// Above most general read from database, setup signals, shared with rudp_*
	// ========== DELAYER STUFFS ==========

	// Setup mutex

	// === Init pause mutex ===
	for (i = 0; i < streaminterface.num_backup_sink + 1; i++)
	{
		retval_mutex_pause_info[i] = pthread_mutex_init(&pause_info[i].mutex_pause, NULL);
		if (retval_mutex_pause_info[i] != 0)
		{
			add_trap_to_queue(TRAP_DELAYER_FAILED_SETUP, "Can not setup mutex");
			LOGMSG(LOG_ERR, "%s: Can not create pause mutex[%d] (%s %d)",
				interfacename,
				i,
				strerror(retval_mutex_pause_info[i]),
				retval_mutex_pause_info[i]);

			pstate = STATE_SHUTDOWN;
			goto SHUTDOWN;
		}
	}

	// === Init oldest file mutex ===
	retval_mutex_oldest_file = pthread_mutex_init(&oldest_file.mutex_file, NULL);
	if (retval_mutex_oldest_file != 0)
	{
		add_trap_to_queue(TRAP_DELAYER_FAILED_SETUP, "Can not setup mutex");
		LOGMSG(LOG_ERR, "%s: Can not create oldest mutex (%s %d)",
			interfacename,
			strerror(retval_mutex_oldest_file),
			retval_mutex_oldest_file);

		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}

	// === Init newest file mutex ===
	retval_mutex_newest_file = pthread_mutex_init(&newest_file.mutex_file, NULL);
	if (retval_mutex_newest_file != 0)
	{
		add_trap_to_queue(TRAP_DELAYER_FAILED_SETUP, "Can not setup mutex");
		LOGMSG(LOG_ERR, "%s: Can not create newest mutex (%s %d)",
			interfacename,
			strerror(retval_mutex_newest_file),
			retval_mutex_newest_file);

		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}

	check_for_long_delay_files(&oldest_file.id, &newest_file.id, streaminterface.name);

	// === Init read file mutex ===
	for (i = 0; i < streaminterface.num_backup_sink + 1; i++)
	{
		retval_mutex_read_file[i] = pthread_mutex_init(&read_file[i].mutex_file, NULL);
		if (retval_mutex_read_file[i] != 0)
		{
			add_trap_to_queue(TRAP_DELAYER_FAILED_SETUP, "Can not setup mutex");
			LOGMSG(LOG_ERR, "%s: Can not create read mutex[%d] (%s %d)",
				interfacename,
				i,
				strerror(retval_mutex_read_file[i]),
				retval_mutex_read_file[i]);

			pstate = STATE_SHUTDOWN;
			goto SHUTDOWN;
		}
		read_file[i].id = newest_file.id;
		read_file[i].status = UNPARSED;
	}

	oldest_file.status = newest_file.status = UNPARSED;

	retval = 1;
	while (retval != 0 && oldest_file.id != newest_file.id)
	{
		// Will not taste the newest file due its gonna be created
		// if id test are less than (<) need to care about wrapping of id
		retval = taste_file(streaminterface.name, &oldest_file);
#ifdef NODAEMON_DEBUG
		fprintf(stderr, "oldest: %d (taste %d) newest: %d\n",
			oldest_file.id, retval, newest_file.id);
#endif
		if (retval != 0)
		{
			// Something wrong!
			// probably a trap?

			// remove file
			char filename[VIDEO_PATH_LEN+512];
			snprintf(filename, VIDEO_PATH_LEN+511, "%s/%s_chunk_%u.bin",
				video_path, streaminterface.name, oldest_file.id);

			LOGMSG(LOG_CRIT, "%s: something wrong (taste_file:%d) will remove %s",
				interfacename, retval, filename);

			unlink(filename);
			oldest_file.id++;
		}
	}

	delayercom[0].id=0;
	delayercom[0].streaminterface_id = streaminterface.id;
	delayercom[0].action[0]='\0';
	delayercom[0].e_action=PLAY;
	delayercom[0].jumptopos[0]='\0';
	delayercom[0].jumptopos_timestamp=0;
	delayercom[0].startplayat[0]='\0';
	delayercom[0].startplayat_timestamp=0;
	delayercom[0].playto[0]='\0';
	delayercom[0].playto_timestamp=0;

#ifdef NODAEMON_DEBUG
	fprintf(stderr, "streaminterface_id: %"PRIu64"\n",
		delayercom[0].streaminterface_id);
	fprintf(stderr, "action: %s (%d)\n",
		delayercom[0].action, delayercom[0].e_action);
	fprintf(stderr, "jumptopos: %s (%"PRIu64")\n",
		delayercom[0].jumptopos, delayercom[0].jumptopos_timestamp);
	fprintf(stderr, "startplayat: %s (%"PRIu64")\n",
		delayercom[0].startplayat, delayercom[0].startplayat_timestamp);
	fprintf(stderr, "playto: %s (%"PRIu64")\n",
		delayercom[0].playto, delayercom[0].playto_timestamp);
#endif

	// === Setup signal handler ===
	struct sigaction sterm;
	memset(&sterm, 0, sizeof(struct sigaction));
	sterm.sa_sigaction = &term_func;
	sterm.sa_flags = 0;// SA_SIGINFO;
	
	retval = sigaction(SIGTERM, &sterm, NULL);
	if (retval == -1)
	{
		add_trap_to_queue(TRAP_DELAYER_FAILED_SETUP, "Can not setup Signal handlers");

		LOGMSG(LOG_ERR, "%s: Error to setup SIGTERM signal handler (%s)",
			   interfacename, strerror(errno));

		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}

	struct sigaction sint;
	memset(&sint, 0, sizeof(struct sigaction));
	sint.sa_sigaction = &term_func;
	sint.sa_flags = 0;//SA_SIGINFO;
	
	retval = sigaction(SIGINT, &sint, NULL);
	if (retval == -1)
	{
		add_trap_to_queue(TRAP_DELAYER_FAILED_SETUP, "Can not setup Signal handlers");
		LOGMSG(LOG_ERR, "%s: Error to setup SIGINT signal handler (%s)",
			   interfacename, strerror(errno));

		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}

	struct sigaction susr2;
	memset(&susr2, 0, sizeof(struct sigaction));
	susr2.sa_sigaction = &read_delay_while_running;
	susr2.sa_flags = 0;//SA_SIGINFO;
	
	//if (signal(SIGUSR2, read_delay_while_running) == SIG_ERR)
	if (sigaction(SIGUSR2, &susr2, NULL) == -1)
	{
		add_trap_to_queue(TRAP_TRANSMITTER_FAILED_SETUP, "Can not setup Signal handlers");
		LOGMSG(LOG_ERR, "%s: Error to setup SIGUSR2 signal handler (%s)",
				interfacename, strerror(errno));

		pstate = STATE_SHUTDOWN;
		pthread_join(thrd_traps, NULL);
		pthread_join(thrd_pokepovmanager, NULL);

		DBClose(dbconn);
		deinit_trap_queue();
		close_pid_file(pidfd, pidfilename);
		trap_send_exit(TRAP_TRANSMITTER_STOP, "stopped.");
		exit(1);
	}

	struct sigaction susr;
	memset(&susr, 0, sizeof(struct sigaction));
	susr.sa_sigaction = &reread_config_signal_func;
	susr.sa_flags = 0;
	retval = sigaction(SIGUSR1, &susr, NULL);

	if (retval == -1)
	{
		// This is primarily a debug feature so if the signal() fails - just log it but no shutdown
		LOGMSG(LOG_ERR, "%s: Error to setup SIGUSR1 signal handler (%s)",
			interfacename, strerror(errno));
	}


#ifdef NODAEMON_DEBUG
	// Debug print
	int sid = syscall(SYS_gettid);
	syslog(LOG_NOTICE, "%s: main SID %d", streaminterface.name, sid);
#endif

	// Setup buffer

	for (i = 0; i < streaminterface.num_backup_sink+1; i++)
	{
		// ---- AVAILABLE BUFFERS -----
		initBufferElements(&buffer_q_available[i].buffer_elements);
		buffer_q_available[i].no_buffer_elements = BUFFER_SIZE;

		// === Initialize buffer mutexes ===
		retval_mutex_available_buffer[i] = pthread_mutex_init(&buffer_q_available[i].mutex, NULL);
		if (retval_mutex_available_buffer[i] != 0)
		{
			add_trap_to_queue(TRAP_DELAYER_FAILED_SETUP, "Can not create buffer mutex");

			LOGMSG(LOG_ERR, "%s[%d]: Can not create available buffer mutex (%s - %d)",
					interfacename,
					i,
					strerror(retval_mutex_available_buffer[i]),
					retval_mutex_available_buffer[i]);

			pstate = STATE_SHUTDOWN;
			goto SHUTDOWN;
		}

		// ---- Send Q ----

		buffer_q_send[i].buffer_elements = NULL;
		buffer_q_send[i].no_buffer_elements = 0;

		retval_mutex_send_buffer[i] = pthread_mutex_init(&buffer_q_send[i].mutex, NULL);
		if (retval_mutex_send_buffer[i] != 0)
		{
			add_trap_to_queue(TRAP_DELAYER_FAILED_SETUP, "Can not create buffer mutex");

			LOGMSG(LOG_ERR, "%s[%d]: Can not create main buffer mutex (%s - %d)",
					interfacename,
					i,
					strerror(retval_mutex_send_buffer[i]),
					retval_mutex_send_buffer[i]);

			pstate = STATE_SHUTDOWN;
			goto SHUTDOWN;
		}
	}

	// Available buffer for the disc Q between mc_receiver -> disc_writer
	initBufferElements(&buffer_q_available[streaminterface.num_backup_sink+1].buffer_elements);
	buffer_q_available[streaminterface.num_backup_sink+1].no_buffer_elements = BUFFER_SIZE;

	// === Initialize buffer mutexes ===
	retval_mutex_available_buffer[streaminterface.num_backup_sink+1] = pthread_mutex_init(&buffer_q_available[streaminterface.num_backup_sink+1].mutex, NULL);
	if (retval_mutex_available_buffer[streaminterface.num_backup_sink+1] != 0)
	{
		add_trap_to_queue(TRAP_DELAYER_FAILED_SETUP, "Can not create buffer mutex");

		LOGMSG(LOG_ERR, "%s: Can not create available buffer mutex[%d] (%s - %d)",
				interfacename,
				streaminterface.num_backup_sink+1,
				strerror(retval_mutex_available_buffer[i]),
				retval_mutex_available_buffer[i]);

		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}


	// === Initialize statistic mutex ===
	retval_mutex_stat = pthread_mutex_init(&mutex_stat, NULL);
	if (retval_mutex_stat != 0)
	{
		add_trap_to_queue(TRAP_DELAYER_FAILED_SETUP, "Can not create statistic mutex");

		LOGMSG(LOG_ERR, "%s: Can not create statistic mutex (%s - %d)",
			   interfacename, strerror(retval_mutex_stat), retval_mutex_stat);

		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}

	// === Add how many interface will report RUDP statistics ===
	rudp_sequence_list.num_paths = streaminterface.num_backup_sink+1;
	rudp_sequence_list.mutex_stat = &mutex_stat;

	/* Initialize before the statistics thread can read these records. */
	memset(d_stats, 0, sizeof(d_stats));
	pthread_ret = pthread_mutex_lock(&mutex_stat);
	LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
	for (i = 0; i < streaminterface.num_backup_sink+1; i++)
	{
		d_stats[i].streaminterface_id = streaminterface.id;
		d_stats[i].pathnumber = i;
		copy_process_state_string(dstate[i], d_stats[i].state);
	}
	pthread_ret = pthread_mutex_unlock(&mutex_stat);
	LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);

	// === Create statistic thread ===
	s_statarg statarg;
	statarg.process_mode = DELAYER;
	statarg.d_stats = d_stats;
	statarg.dstate = dstate;
	statarg.id = streaminterface.id;
	statarg.num_paths = streaminterface.num_backup_sink+1;
	strncpy(statarg.mode, streaminterface.config.mode, DB_MAX_MODE_LENGTH);
	strncpy(statarg.name, streaminterface.name, DB_MAX_NAME_LENGTH);
	statarg.config = &(streaminterface.config);
	statarg.sink = streaminterface.sink;
	statarg.source = streaminterface.source;
	statarg.dbconn = &dbconn;
	statarg.mutex_dbconn = &mutex_dbconn;

	statarg.mutex_stat = &mutex_stat;
	statarg.rt_stats = &rt_stats;
	statarg.rudp_sequence_list = &rudp_sequence_list;

	retval_thrd_statistic = pthread_create(&thrd_statistic, NULL, thread_statistic, (void *)&statarg);
	if (retval_thrd_statistic != 0)
	{
		add_trap_to_queue(TRAP_DELAYER_FAILED_SETUP, "Can not setup statistics");
		LOGMSG(LOG_ERR, "%s: Cant create statistic thread (%s - %d)",
			   interfacename, strerror(retval_thrd_statistic), retval_thrd_statistic);

		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}

	// === Initialize semaphore - thread_tsreceiver are dependent on the semaphore ===
	retval_semaphore = sem_init(&semaphore, 0, 0);
	if (retval_semaphore == -1)
	{
		add_trap_to_queue(TRAP_DELAYER_FAILED_SETUP, "Can not initiate semaphore");
		LOGMSG(LOG_ERR, "%s: cant initiate semaphore %s",
			   interfacename, strerror(errno));

		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}

	retval_mutex_disc_buffer = pthread_mutex_init(&buffer_q_disc.mutex, NULL);
	if (retval_mutex_disc_buffer != 0)
	{
		add_trap_to_queue(TRAP_DELAYER_FAILED_SETUP, "Can not create disc buffer mutex");

		LOGMSG(LOG_ERR, "%s: Can not create disc buffer mutex (%s - %d)",
			   interfacename,
			   strerror(retval_mutex_disc_buffer),
			   retval_mutex_disc_buffer);

		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}

#if 0
	retval_mutex_disc_signal = pthread_mutex_init(&mutex_disc_signal, NULL);
	if (retval_mutex_disc_signal != 0)
	{
		add_trap_to_queue(TRAP_DELAYER_FAILED_SETUP, "Can not create disc signal mutex");

		LOGMSG(LOG_ERR, "%s: Can not create buffer mutex (%s - %d)",
			   interfacename,
			   strerror(retval_mutex_disc_signal),
			   retval_mutex_disc_signal);

		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}

	retval_condt_disc_signal = pthread_cond_init(&condt_disc_signal, NULL);
	if (retval_condt_disc_signal != 0)
	{
		add_trap_to_queue(TRAP_DELAYER_FAILED_SETUP, "Can not create condt disc signal");

		LOGMSG(LOG_ERR, "%s: Can not create condt disc signal (%s - %d)",
			   interfacename,
			   strerror(retval_condt_disc_signal),
			   retval_condt_disc_signal);

		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}
#endif

	thread_disc_writer_arg writerarg;
	writerarg.state = dstate;
	writerarg.name = streaminterface.name;
	writerarg.streaminterface_id = streaminterface.config.streaminterface_id;
	writerarg.writer_file = &newest_file;

	writerarg.buffer_q_available = &buffer_q_available[streaminterface.num_backup_sink+1];
	writerarg.buffer_q_disc = &buffer_q_disc;
	retval_thrd_discwriter = pthread_create(&thrd_discwriter, NULL, thread_discwriter, (void *)&writerarg);

	if (retval_thrd_discwriter != 0)
	{
		add_trap_to_queue(TRAP_DELAYER_FAILED_SETUP, "Can not setup discwriter thread");
		LOGMSG(LOG_ERR, "%s: cant create discwriter thread (%s - %d)",
			   interfacename,
			   strerror(retval_thrd_discwriter),
			   retval_thrd_discwriter);

		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}

	for (i = 0; i < streaminterface.num_backup_sink+1; i++)
	{
		reader_arg[i].state = &dstate[i];
		reader_arg[i].name = streaminterface.name;
		reader_arg[i].streaminterface_id = streaminterface.config.streaminterface_id;
		reader_arg[i].pathno = i;
		reader_arg[i].writer_file = &newest_file;
		reader_arg[i].reader_file = &read_file[i];
		reader_arg[i].pause = &pause_info[i];
		reader_arg[i].buffer_q_available = &buffer_q_available[i];
		reader_arg[i].buffer_q_send = &buffer_q_send[i];
		reader_arg[i].dbconn = &dbconn;
		reader_arg[i].mutex_dbconn = &mutex_dbconn;

		retval_thrd_discreader[i] = pthread_create(&thrd_discreader[i], NULL, 
				thread_discreader, (void *)&reader_arg[i]);
		if (retval_thrd_discreader[i] != 0)
		{
			add_trap_to_queue(TRAP_DELAYER_FAILED_SETUP, "Can not setup discreader thread");
			LOGMSG(LOG_ERR, "%s[%d]: cant create discreader thread (%s - %d)",
				   interfacename,
				   i,
				   strerror(retval_thrd_discreader[i]),
				   retval_thrd_discreader[i]);

			pstate = STATE_SHUTDOWN;
			goto SHUTDOWN;
		}
	}

	// === Start multicast receiver thread ===
	thread_mc_receiver_arg mc_receiver_arg;
	memset((void*)&mc_receiver_arg, 0, sizeof(thread_mc_receiver_arg));
	strncpy(mc_receiver_arg.multicast_address, streaminterface.source[0].host, 16);
	mc_receiver_arg.multicast_port = streaminterface.source[0].port;
	mc_receiver_arg.multicast_ttl = 0;
	mc_receiver_arg.buffer_timeout = 10; // streaminterface.config.msbuffer;
	mc_receiver_arg.state = dstate;
	mc_receiver_arg.rawudp = 0;
	mc_receiver_arg.stat_last_timestamp = 0;
	mc_receiver_arg.name = streaminterface.name;
	mc_receiver_arg.pathno = 0;
	mc_receiver_arg.no_sink = streaminterface.num_backup_sink+1;
	mc_receiver_arg.buffer_q_available = buffer_q_available;
	mc_receiver_arg.buffer_q_disc = &buffer_q_disc;
	mc_receiver_arg.dbconn = &dbconn;
	mc_receiver_arg.mutex_dbconn = &mutex_dbconn;
	// Need every sendQ for livestreaming *for now*
	mc_receiver_arg.buffer_q_send = buffer_q_send;

	mc_receiver_arg.rudp_sequence_list = &rudp_sequence_list;
	mc_receiver_arg.mutex_stat = &mutex_stat;
	mc_receiver_arg.rt_stats = &rt_stats;


	retval_thrd_mc_receiver = pthread_create(&thrd_mc_receiver, NULL, thread_mc_receiver, (void*)&mc_receiver_arg);
	if (retval_thrd_mc_receiver != 0)
	{
		add_trap_to_queue(TRAP_DELAYER_FAILED_SETUP, "Can not setup multicast input");
		LOGMSG(LOG_ERR, "%s: cant create sender thread (%s - %d)",
			   interfacename,
			   strerror(retval_thrd_mc_receiver),
			   retval_thrd_mc_receiver);

		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}

	// === Start multicast sender thread ===
	for (i = 0; i < streaminterface.num_backup_sink+1; i++)
	{
		strncpy(mc_sender_arg[i].multicast_address, streaminterface.sink[i].host, 16);
		mc_sender_arg[i].multicast_port = streaminterface.sink[i].port;
		mc_sender_arg[i].multicast_ttl = streaminterface.sink[i].ttl;
		mc_sender_arg[i].buffer_timeout = 100; //streaminterface.config.msbuffer;
		mc_sender_arg[i].state = &dstate[i];
		mc_sender_arg[i].rawudp = streaminterface.sink[i].rawudp;
		mc_sender_arg[i].stat_last_timestamp = 0;
		mc_sender_arg[i].buffer_q_available = &buffer_q_available[i];
		mc_sender_arg[i].buffer_q = &buffer_q_send[i];
		mc_sender_arg[i].pathno = i;
		mc_sender_arg[i].name = streaminterface.name;
		mc_sender_arg[i].rudp_sequence_list = &rudp_sequence_list;
		mc_sender_arg[i].mutex_stat = &mutex_stat;
		mc_sender_arg[i].rt_stats = &rt_stats;

		syslog(LOG_NOTICE, "starting output %s:%d with ttl %d and rawudp=%d.", streaminterface.sink[i].host, streaminterface.sink[i].port, streaminterface.sink[i].ttl, streaminterface.sink[i].rawudp);
		
		retval_thrd_mc_sender[i] = pthread_create(&thrd_mc_sender[i], NULL, thread_mc_sender, (void *)&mc_sender_arg[i]);
		if (retval_thrd_mc_sender[i] != 0)
		{
			add_trap_to_queue(TRAP_DELAYER_FAILED_SETUP, "Can not setup multicast output");

			LOGMSG(LOG_ERR, "%s[%d]: cant create sender thread (%s - %d)",
				interfacename,
				i,
				strerror(retval_thrd_mc_sender[i]),
				retval_thrd_mc_sender[i]);

			pstate = STATE_SHUTDOWN;
			goto SHUTDOWN;
		}
	}


	// ------- DISCCLEANER ------
	retval_mutex_discclean_signal = pthread_mutex_init(&mutex_discclean_signal, NULL);
	if (retval_mutex_discclean_signal != 0)
	{
		add_trap_to_queue(TRAP_DELAYER_FAILED_SETUP, "Can not create discclean signal mutex");

		LOGMSG(LOG_ERR, "%s: Can not create discclean mutex (%s - %d)",
			   interfacename,
			   strerror(retval_mutex_discclean_signal),
			   retval_mutex_discclean_signal);

		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}

	retval_condt_discclean_signal = pthread_cond_init(&condt_discclean_signal, NULL);
	if (retval_condt_discclean_signal != 0)
	{
		add_trap_to_queue(TRAP_DELAYER_FAILED_SETUP, "Can not create condt discclean signal");

		LOGMSG(LOG_ERR, "%s: Can not create condt discclean signal (%s - %d)",
			   interfacename,
			   strerror(retval_condt_discclean_signal),
			   retval_condt_discclean_signal);

		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}

	// TODO - Read from database
	total_disc_buffer_time = streaminterface.config.msbuffer/1000; //TOTAL_DISC_BUFFER_TIME_IN_S;

	s_thread_disc_cleaner_arg disccleaner;
	disccleaner.state = &dstate[streaminterface.num_backup_sink+1];
	disccleaner.mutex_discclean_signal = &mutex_discclean_signal;
	disccleaner.condt_discclean_signal = &condt_discclean_signal;
	disccleaner.total_disc_buffer_time = &total_disc_buffer_time;
	disccleaner.streaminterface_id = streaminterface.config.streaminterface_id;
	disccleaner.name = streaminterface.name;
	disccleaner.writer_file = &newest_file;
	disccleaner.reader_file = &read_file[0];
	disccleaner.oldest_file = &oldest_file;
	disccleaner.pause = &pause_info[0];

	retval_thrd_disccleaner = pthread_create(&thrd_disccleaner, NULL, thread_disccleaner, (void *)&disccleaner);
	if (retval_thrd_disccleaner != 0)
	{
		add_trap_to_queue(TRAP_DELAYER_FAILED_SETUP, "Can not setup disccleaner thread");
		LOGMSG(LOG_ERR, "%s: cant create disccleaner thread (%s - %d)",
			   interfacename,
			   strerror(retval_thrd_disccleaner),
			   retval_thrd_disccleaner);

		pstate = STATE_SHUTDOWN;
		goto SHUTDOWN;
	}

	// ------------ ALL THREADS ARE STARTED ------------

	s_interfaceconfig tmpinterfaceconfig; //For passing on delay from database to streaminterface.config.msbuffer

	while (pstate != STATE_SHUTDOWN)
	{
		if(reread_delay_from_database)
		{
			memset(&tmpinterfaceconfig, 0, sizeof(s_interfaceconfig));
	
			//Read from database into temporary s_interfaceconfig
			if(dbfetchstreaminterfaceconfigbystreaminterfaceid(&dbconn, &tmpinterfaceconfig, streaminterface.id))
			{
				add_trap_to_queue(TRAP_TRANSMITTER_FAILED_SETUP, "Could not fetch config from database");
			}
			else
			{
				//Read new msbuffer value into streaminterface.config.msbuffer while interface is still running
				streaminterface.config.msbuffer = tmpinterfaceconfig.msbuffer;
				LOGMSG(LOG_NOTICE, "msbuffer updated from database. msbuffer = %d", streaminterface.config.msbuffer);
			}
			reread_delay_from_database = 0;
		}

		/* SIGUSR1 wakes us promptly. Poll as well: standard Unix signals can
		 * coalesce, and a second command must not remain stranded in the queue. */
		if (++command_poll_ticks >= 10)
		{
			command_poll_ticks = 0;
			pthread_ret = pthread_mutex_lock(&mutex_dbconn);
			LOG_PTHREAD_ERROR_CONTINUE(pthread_ret, interfacename, __LINE__);
			if (db_count_delayercoms(&dbconn, streaminterface.id) > 0)
				reread_config_from_database = 1;
			pthread_ret = pthread_mutex_unlock(&mutex_dbconn);
			LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
		}

		if (reread_config_from_database)
		{
			int num_fetched = 0;
			pthread_ret = pthread_mutex_lock(&mutex_dbconn);
			LOG_PTHREAD_ERROR_CONTINUE(pthread_ret, interfacename, __LINE__);
			retval = db_fetch_delayercoms(&dbconn, delayercom, 1, &num_fetched, streaminterface.id);
			if (retval || num_fetched == 0)
			{
				/* A coalesced or redundant wakeup may find an empty queue.
				 * Never delete other commands on a read/connection failure. */
				pthread_ret = pthread_mutex_unlock(&mutex_dbconn);
				LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
				if (retval != 3)
					LOGMSG(LOG_WARNING, "%s: Can not fetch delayercoms (fetch %d)",
						streaminterface.name, retval);
				reread_config_from_database = 0;
				continue;
			}

			if (delayercom[0].pathnumber > streaminterface.num_backup_sink)
			{
				int retval_delete = db_delete_delayercomsbyid(&dbconn, delayercom[0].id);
				logtodb_nolock(dbconn, streaminterface.id, POVLOGLEVEL_INFO,
							   "%s: pathnumber are greater than configured (%"PRIu64" > %d)",
							   interfacename, delayercom[0].pathnumber, streaminterface.num_backup_sink);

				pthread_ret = pthread_mutex_unlock(&mutex_dbconn);
				LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);

				add_trap_to_queue(TRAP_DELAYER_STOP, "Requested path doesnt exists");
				LOGMSG(LOG_WARNING, "%s: pathnumber are greater than configured (%"PRIu64" > %d) (%d %d)",
					   interfacename, delayercom[0].pathnumber, streaminterface.num_backup_sink, retval, retval_delete);

				continue;
			}
			// Already got the lock just pop the log
			logtodb_nolock(dbconn, streaminterface.id, POVLOGLEVEL_INFO,
					"%s Config: id %"PRIu64"[%"PRIu64"] action: %s jump: %s start play at: %s playto: %s",
					interfacename, delayercom[0].streaminterface_id,
					delayercom[0].pathnumber, delayercom[0].action,
					delayercom[0].jumptopos, delayercom[0].startplayat,
					delayercom[0].playto);
			pthread_ret = pthread_mutex_unlock(&mutex_dbconn);
			LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
			gettimeofday(&tv, NULL);
			now = tv.tv_sec*1000000 + tv.tv_usec;
			switch (delayercom[0].e_action)
			{
			case JUMP:
				// WAVE HANDS - assume that oldest_file and newest_file doesnt change during search...
				// solution make a copy?

				// strategy:
				// check requested time (jump_to) are not in the future 
				// look on the oldest file starttime is it less => error
				// set pause
				// calculate diff from oldest starttime and requested time
				{
#ifdef NODAEMON_DEBUG
				micro_timestamp_to_str(oldest_file.start_time, buf1, 80);
				micro_timestamp_to_str(delayercom[0].jumptopos_timestamp, buf2, 80);
				micro_timestamp_to_str(now, buf3, 80);
				fprintf(stderr, "%s[%"PRIu64"]: jump: oldest_file.start_time %s jumptopos_timestamp %s now %s\n",
						interfacename, delayercom[0].pathnumber,
						buf1, buf2, buf3);
				fprintf(stderr, "%s[%"PRIu64"]: jump: oldest_file.start_time %"PRIu64" jumptopos_timestamp %"PRIu64" now %"PRIu64"\n",
						interfacename, delayercom[0].pathnumber,
						oldest_file.start_time, delayercom[0].jumptopos_timestamp, now);
				
#endif
				if (delayercom[0].jumptopos_timestamp > now)
				{
					// Maybe this is how to put delayer from delayed state to livestreaming again.
					LOGMSG(LOG_INFO, "%s[%"PRIu64"]: Jump to future(jumpto: %"PRIu64" now: %"PRIu64")! Do nothing",
						interfacename, delayercom[0].pathnumber, delayercom[0].jumptopos_timestamp, now);
					logtodb(&mutex_dbconn, dbconn, streaminterface.id, POVLOGLEVEL_INFO,
						"%s[%"PRIu64"]: jump to future - no action",
							interfacename, delayercom[0].pathnumber);
					break;
				}

				if (oldest_file.start_time > delayercom[0].jumptopos_timestamp)
				{
					LOGMSG(LOG_INFO, "%s[%"PRIu64"]: Jump is older than the oldest file (jumpto: %"PRIu64" now: %"PRIu64")! Do nothing",
						interfacename, delayercom[0].pathnumber, oldest_file.start_time, delayercom[0].jumptopos_timestamp);
					logtodb(&mutex_dbconn, dbconn, streaminterface.id, POVLOGLEVEL_INFO,
							"%s[%"PRIu64"]: jump to older than buffer - no action",
							interfacename, delayercom[0].pathnumber);
					break;
				}

				// ------------------- Look for ID of file ----------------------
				uint32_t file_id_to_search_in = 0;

				pthread_ret = pthread_mutex_lock(&newest_file.mutex_file);
				LOG_PTHREAD_ERROR_CONTINUE(pthread_ret, interfacename, __LINE__);
				pthread_ret = pthread_mutex_lock(&oldest_file.mutex_file);
				LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
				if (delayercom[0].jumptopos_timestamp >= oldest_file.start_time &&
					oldest_file.end_time >= delayercom[0].jumptopos_timestamp)
				{
					// --- requested time are in the oldest file ---
					file_id_to_search_in = oldest_file.id;
#ifdef NODAEMON_DEBUG
					fprintf(stderr, "%s[%"PRIu64"] -> In oldest file - search for position in file!\n",
							interfacename, delayercom[0].pathnumber);
#endif
				}
				else if (delayercom[0].jumptopos_timestamp >= newest_file.start_time &&
					newest_file.end_time >= delayercom[0].jumptopos_timestamp)
				{
					// --- requested time are in the newest file ---
					file_id_to_search_in = newest_file.id;
#ifdef NODAEMON_DEBUG
					fprintf(stderr, "%s[%"PRIu64"]: -> In newest file - search for position in file!\n",
							interfacename, delayercom[0].pathnumber);
#endif
				}
				else
				{
					// --- requested time are somewhere in the cache - search! ---
					uint64_t timediff_from_oldest = delayercom[0].jumptopos_timestamp - oldest_file.start_time;
					uint32_t delta_file_id = timediff_from_oldest/CHUNK_TIME_DIFF;
#ifdef NODAEMON_DEBUG
					fprintf(stderr, "%s[%"PRIu64"]: oldest: ID %u start_time %"PRIu64"\n",
							interfacename, delayercom[0].pathnumber,
							oldest_file.id, oldest_file.start_time);
					fprintf(stderr, "%s[%"PRIu64"]: Newest: ID %u start_time %"PRIu64"\n",
							interfacename, delayercom[0].pathnumber,
							newest_file.id, newest_file.start_time);
					fprintf(stderr, "%s[%"PRIu64"]: Diff %"PRIu64" chunks %u\n",
							interfacename, delayercom[0].pathnumber,
							timediff_from_oldest, delta_file_id);
#endif

					uint32_t search_id = delta_file_id + oldest_file.id;
					if (search_id >= newest_file.id)
					{
						// Already checked the newest file - take one less.
						search_id = newest_file.id - 1;
					}

					uint32_t max_files = newest_file.id - oldest_file.id;
					uint32_t i = 0;
					bool dance_up = false, dance_down = false;
					for (i = 0; i < max_files; i++)
					{
					s_file_info calc_file;
					calc_file.id=search_id;
					retval = taste_file(streaminterface.name, &calc_file);
#ifdef NODAEMON_DEBUG
					fprintf(stderr, "%s[%"PRIu64"]: Look inside calculated file calc_file: id %u time %"PRIu64" - %"PRIu64"\n",
							interfacename, delayercom[0].pathnumber,
							calc_file.id, calc_file.start_time, calc_file.end_time);
#endif

					if (retval != 0)
					{
						// If this current file cant be analyzed - assume the right
						// thing to do are to take the next file and move on
						LOGMSG(LOG_CRIT, "%s: JUMP: taste_file:%d search_id %d",
							interfacename, retval, search_id);
						search_id++;
						dance_up = true;
					}
					else if (delayercom[0].jumptopos_timestamp >= calc_file.start_time &&
						calc_file.end_time >= delayercom[0].jumptopos_timestamp)
					{
						file_id_to_search_in = search_id;
#ifdef NODAEMON_DEBUG
						fprintf(stderr, "%s[%"PRIu64"]: -> Position are in the calculated file\n\n",
								interfacename, delayercom[0].pathnumber);
#endif
						break;
					}
					else if (calc_file.start_time > delayercom[0].jumptopos_timestamp)
					{
#ifdef NODAEMON_DEBUG
						fprintf(stderr, "%s[%"PRIu64"]: -> Position are in earlier file\n",
								interfacename, delayercom[0].pathnumber);
#endif
						search_id--;
						dance_down = true;
					}
					else if (delayercom[0].jumptopos_timestamp > calc_file.end_time)
					{
#ifdef NODAEMON_DEBUG
						fprintf(stderr, "%s[%"PRIu64"]: -> Position are in later file\n",
								interfacename, delayercom[0].pathnumber);
#endif
						search_id++;
						dance_up = true;
					}
					// Handle potential wraps!
					if (search_id < oldest_file.id)
					{
#ifdef NODAEMON_DEBUG
						fprintf(stderr, "%s[%"PRIu64"]: search_id wraps at the oldest -> newest\n",
								interfacename, delayercom[0].pathnumber);
#endif
						search_id = newest_file.id;
					}
					if (search_id > newest_file.id)
					{
#ifdef NODAEMON_DEBUG
						fprintf(stderr, "%s[%"PRIu64"]: search_id wraps at the newest -> oldest\n",
								interfacename, delayercom[0].pathnumber);
#endif
						search_id = oldest_file.id;
					}
					if (dance_up && dance_down)
					{
#ifdef NODAEMON_DEBUG
						fprintf(stderr, "%s[%"PRIu64"]: Search are gonna start dance!\n",
								interfacename, delayercom[0].pathnumber);
#endif
						break;
					}
					} // for

					if (i == max_files || (dance_up && dance_up))
					{
						add_trap_to_queue(TRAP_DELAYER_STOP, "Can not find file");
						if (i == max_files)
						{
							LOGMSG(LOG_INFO, "%s[%"PRIu64"]: Cant find time in any of the files (max filesearch)\n",
								interfacename, delayercom[0].pathnumber);
						}
						else
						{
							LOGMSG(LOG_INFO, "%s[%"PRIu64"]: Requested time are in a hole\n",
								interfacename, delayercom[0].pathnumber);
						}
						logtodb(&mutex_dbconn, dbconn, streaminterface.id, POVLOGLEVEL_INFO,
								"%s[%"PRIu64"]: cant find buffer file",
								interfacename, delayercom[0].pathnumber);
						pthread_ret = pthread_mutex_unlock(&oldest_file.mutex_file);
						LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
						pthread_ret = pthread_mutex_unlock(&newest_file.mutex_file);
						LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
						break;
					}
				}
				pthread_ret = pthread_mutex_unlock(&oldest_file.mutex_file);
				LOG_PTHREAD_ERROR_CONTINUE(pthread_ret, interfacename, __LINE__);
				pthread_ret = pthread_mutex_unlock(&newest_file.mutex_file);
				LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);

#ifdef NODAEMON_DEBUG
				fprintf(stderr, "%s[%"PRIu64"]: searched time are in file ID: %u\n\n",
						interfacename, delayercom[0].pathnumber,
						file_id_to_search_in);
#endif

				s_pause_arg parg;
				int ret = find_time(streaminterface.name, file_id_to_search_in,
					delayercom[0].jumptopos_timestamp, &parg);
				if (ret != 0)
				{
					add_trap_to_queue(TRAP_DELAYER_STOP, "Can not find timestamp in file");
					LOGMSG(LOG_INFO, "%s[%"PRIu64"]: Cant find the timestamp in the file (id: %u) -> FIX -> EXIT\n",
							interfacename, delayercom[0].pathnumber,
							file_id_to_search_in);
					logtodb(&mutex_dbconn, dbconn, streaminterface.id, POVLOGLEVEL_INFO,
							"%s[%"PRIu64"]: cant find timestamp in file",
							interfacename, delayercom[0].pathnumber);
					break;
				}

#ifdef NODAEMON_DEBUG
				fprintf(stderr, "%s[%"PRIu64"]: Set state DELAYER_STATE_PAUSE\n",
						interfacename, delayercom[0].pathnumber);
#endif

				// ------------------- PAUSE -------------------------------------
				dstate[delayercom[0].pathnumber] = DELAYER_STATE_PAUSE;
				sched_yield();

				// Fill the pause structure
				pthread_ret = pthread_mutex_lock(&pause_info[delayercom[0].pathnumber].mutex_pause);
				LOG_PTHREAD_ERROR_CONTINUE(pthread_ret, interfacename, __LINE__);
				pause_info[delayercom[0].pathnumber].pause_time = parg.pause_time;
				pause_info[delayercom[0].pathnumber].pause_pos = parg.pause_pos;
				pause_info[delayercom[0].pathnumber].pause_file_id = parg.pause_file_id;

#ifdef NODAEMON_DEBUG
				fprintf(stderr, "%s[%"PRIu64"]: pause_time: %"PRIu64" pos: %jd file_id: %u\n",
						interfacename, delayercom[0].pathnumber,
						pause_info[delayercom[0].pathnumber].pause_time,
						pause_info[delayercom[0].pathnumber].pause_pos,
						pause_info[delayercom[0].pathnumber].pause_file_id);
				fprintf(stderr, "---------------------------------------------------------\n");
#endif
				pthread_ret = pthread_mutex_unlock(&pause_info[delayercom[0].pathnumber].mutex_pause);
				LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);

				start_play_at[delayercom[0].pathnumber] = delayercom[0].startplayat_timestamp;
				if (start_play_at[delayercom[0].pathnumber] == 0)
				{
					start_play_at[delayercom[0].pathnumber] = UINT64_MAX;
				}

				play_to[delayercom[0].pathnumber] = delayercom[0].playto_timestamp;
				if (play_to[delayercom[0].pathnumber] == 0)
				{
					play_to[delayercom[0].pathnumber] = UINT64_MAX;
				}
#if 0
				if (delayercom[0].startplayat_timestamp == 0)
				{
					fprintf(stderr, "2.1.1) Om startplayat är NULL pausa, play skickas enligt 3 nedan\n");
					// 2.1.1) Om startplayat är NULL pausa, play skickas enligt 3 nedan
				}
				else if (delayercom[0].playto_timestamp < delayercom[0].startplayat_timestamp)
				{
					fprintf(stderr, "2.1.3_a) Om playto är satt innan startplayat spela upp i all framtid\n");
					// 2.1.3_a) Om playto är satt innan startplayat spela upp i all framtid
				}
#endif
				}
				break;
			case PLAY:
				if (dstate[delayercom[0].pathnumber] == DELAYER_STATE_DELAYED || dstate[delayercom[0].pathnumber] == DELAYER_STATE_LIVESTREAMING)
				{
					// Doesnt make sense to change state to play if its already in "playing" mode
					logtodb(&mutex_dbconn, dbconn, streaminterface.id, POVLOGLEVEL_INFO,
							"%s[%"PRIu64"] already in play mode",
							interfacename, delayercom[0].pathnumber);
					break;
				}
				{
				bool move_time = false;
				pthread_ret = pthread_mutex_lock(&pause_info[delayercom[0].pathnumber].mutex_pause);
				LOG_PTHREAD_ERROR_CONTINUE(pthread_ret, interfacename, __LINE__);
				pthread_ret = pthread_mutex_lock(&oldest_file.mutex_file);
				LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
				if (oldest_file.id > pause_info[delayercom[0].pathnumber].pause_file_id)
				{
#ifdef NODAEMON_DEBUG
					fprintf(stderr, "%s[%"PRIu64"]: MOVE pausetime was %d new %d!\n",
							interfacename, delayercom[0].pathnumber,
							pause_info[delayercom[0].pathnumber].pause_file_id, oldest_file.id);
#endif
					if (oldest_file.status == UNPARSED)
					{
#ifdef NODAEMON_DEBUG
						fprintf(stderr, "%s[%"PRIu64"]: oldest file are unparsed!\n",
								interfacename, delayercom[0].pathnumber);
#endif
						pthread_ret = pthread_mutex_unlock(&oldest_file.mutex_file);
						LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
						pthread_ret = pthread_mutex_unlock(&pause_info[delayercom[0].pathnumber].mutex_pause);
						LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
						logtodb(&mutex_dbconn, dbconn, streaminterface.id, POVLOGLEVEL_INFO,
								"%s[%"PRIu64"]: on oldest file are not parsed!",
								interfacename, delayercom[0].pathnumber);
						break;
					}
					// Cleaner has removed file this interface pointed at.
					pause_info[delayercom[0].pathnumber].pause_file_id = oldest_file.id;
					pause_info[delayercom[0].pathnumber].pause_time = oldest_file.start_time;
					move_time = true;
				}

				pause_info[delayercom[0].pathnumber].pause_diff = now - pause_info[delayercom[0].pathnumber].pause_time;
#ifdef NODAEMON_DEBUG
				fprintf(stderr, "%s[%"PRIu64"]: pasue_diff: %"PRIu64" - %"PRIu64" = %"PRIu64"\n",
						interfacename, delayercom[0].pathnumber,
						(tv.tv_sec * 1000000 + tv.tv_usec),
						pause_info[delayercom[0].pathnumber].pause_time,
						pause_info[delayercom[0].pathnumber].pause_diff);
#endif
				pthread_ret = pthread_mutex_unlock(&oldest_file.mutex_file);
				LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
				pthread_ret = pthread_mutex_unlock(&pause_info[delayercom[0].pathnumber].mutex_pause);
				LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);

				if (move_time)
				{
					// to avoid 3 layers of mutex locks
					logtodb(&mutex_dbconn, dbconn, streaminterface.id, POVLOGLEVEL_INFO,
							"%s[%"PRIu64"]: Due to pause position was older than buffer its now playing at the end of buffer!",
							interfacename, delayercom[0].pathnumber);
				}

				// trick getTimedoutPackets to get packets
				mc_sender_arg[delayercom[0].pathnumber].buffer_timeout = pause_info[delayercom[0].pathnumber].pause_diff/1000;
				
				dstate[delayercom[0].pathnumber] = DELAYER_STATE_DELAYED;
				start_play_at[delayercom[0].pathnumber] = UINT64_MAX;
				play_to[delayercom[0].pathnumber] = delayercom[0].playto_timestamp;
				if (play_to[delayercom[0].pathnumber] == 0)
				{
					play_to[delayercom[0].pathnumber] = UINT64_MAX;
				}
				}
				break;
			case PAUSE:
				{
				int find_time_ret = 0;
				switch (dstate[delayercom[0].pathnumber])
				{
				case DELAYER_STATE_PAUSE:
#ifdef NODAEMON_DEBUG
					fprintf(stderr, "%s[%"PRIu64"]: DELAYER_STATE_PAUSE -> PAUSE - do nothing\n",
							interfacename, delayercom[0].pathnumber);
#endif
					logtodb(&mutex_dbconn, dbconn, streaminterface.id, POVLOGLEVEL_INFO,
							"%s[%"PRIu64"] already in pause mode",
							interfacename, delayercom[0].pathnumber);
					// Do nothing - are already paused!
					break;
				case DELAYER_STATE_LIVESTREAMING:
					{
					if (mc_sender_arg[delayercom[0].pathnumber].stat_last_timestamp <= oldest_file.start_time)
					{
						pthread_ret = pthread_mutex_lock(&oldest_file.mutex_file);
						LOG_PTHREAD_ERROR_CONTINUE(pthread_ret, interfacename, __LINE__);
						// if this happens its probably due no multicast is received
						pthread_ret = pthread_mutex_lock(&pause_info[delayercom[0].pathnumber].mutex_pause);
						LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
						pause_info[delayercom[0].pathnumber].pause_time = oldest_file.start_time;
						pause_info[delayercom[0].pathnumber].pause_file_id = oldest_file.id;
						find_time_ret = find_time(streaminterface.name, pause_info[delayercom[0].pathnumber].pause_file_id,
							pause_info[delayercom[0].pathnumber].pause_time, &pause_info[delayercom[0].pathnumber]);
						pthread_ret = pthread_mutex_unlock(&pause_info[delayercom[0].pathnumber].mutex_pause);
						LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
						pthread_ret = pthread_mutex_unlock(&oldest_file.mutex_file);
						LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
						break;
					}
					if (mc_sender_arg[delayercom[0].pathnumber].stat_last_timestamp < newest_file.start_time)
					{
						// Dont know why this should happens yet
						pthread_ret = pthread_mutex_lock(&newest_file.mutex_file);
						LOG_PTHREAD_ERROR_CONTINUE(pthread_ret, interfacename, __LINE__);
						pthread_ret = pthread_mutex_lock(&pause_info[delayercom[0].pathnumber].mutex_pause);
						LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
						pause_info[delayercom[0].pathnumber].pause_time = newest_file.start_time;
						pause_info[delayercom[0].pathnumber].pause_file_id = newest_file.id;
						find_time_ret = find_time(streaminterface.name, pause_info[delayercom[0].pathnumber].pause_file_id,
							pause_info[delayercom[0].pathnumber].pause_time, &pause_info[delayercom[0].pathnumber]);
						pthread_ret = pthread_mutex_unlock(&pause_info[delayercom[0].pathnumber].mutex_pause);
						LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
						pthread_ret = pthread_mutex_unlock(&newest_file.mutex_file);
						LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
						break;
					}
#ifdef NODAEMON_DEBUG
					fprintf(stderr, "%s[%"PRIu64"]: from db: %"PRIu64" newwest_file.start_time %"PRIu64" < %d\n\n",
							interfacename, delayercom[0].pathnumber,
							mc_sender_arg[delayercom[0].pathnumber].stat_last_timestamp,
							newest_file.start_time,
							(mc_sender_arg[delayercom[0].pathnumber].stat_last_timestamp < newest_file.start_time));
#endif


					pthread_ret = pthread_mutex_lock(&pause_info[delayercom[0].pathnumber].mutex_pause);
					LOG_PTHREAD_ERROR_CONTINUE(pthread_ret, interfacename, __LINE__);
					pause_info[delayercom[0].pathnumber].pause_time = mc_sender_arg[delayercom[0].pathnumber].stat_last_timestamp;
					pause_info[delayercom[0].pathnumber].pause_file_id = newest_file.id;
					find_time_ret = find_time(streaminterface.name, pause_info[delayercom[0].pathnumber].pause_file_id,
						pause_info[delayercom[0].pathnumber].pause_time, &pause_info[delayercom[0].pathnumber]);
					pthread_ret = pthread_mutex_unlock(&pause_info[delayercom[0].pathnumber].mutex_pause);
					LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);

					}
#ifdef NODAEMON_DEBUG
					fprintf(stderr, "%s[%"PRIu64"]: DELAYER_STATE_LIVESTREAMING -> PAUSE(id: %u time: %"PRIu64") find_time_ret %d\n",
							interfacename, delayercom[0].pathnumber,
							pause_info[delayercom[0].pathnumber].pause_file_id,
							pause_info[delayercom[0].pathnumber].pause_time,
							find_time_ret);
#endif
					break;
				case DELAYER_STATE_DELAYED:
					{
					if (mc_sender_arg[delayercom[0].pathnumber].stat_last_timestamp <= oldest_file.start_time)
					{
						pthread_ret = pthread_mutex_lock(&oldest_file.mutex_file);
						LOG_PTHREAD_ERROR_CONTINUE(pthread_ret, interfacename, __LINE__);
						pthread_ret = pthread_mutex_lock(&pause_info[delayercom[0].pathnumber].mutex_pause);
						LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
						pause_info[delayercom[0].pathnumber].pause_time = oldest_file.start_time;
						pause_info[delayercom[0].pathnumber].pause_file_id = oldest_file.id;
						find_time_ret = find_time(streaminterface.name, pause_info[delayercom[0].pathnumber].pause_file_id,
							pause_info[delayercom[0].pathnumber].pause_time, &pause_info[delayercom[0].pathnumber]);
						pthread_ret = pthread_mutex_unlock(&pause_info[delayercom[0].pathnumber].mutex_pause);
						LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
						pthread_ret = pthread_mutex_unlock(&oldest_file.mutex_file);
						LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
						break;
					}
					pthread_ret = pthread_mutex_lock(&read_file[delayercom[0].pathnumber].mutex_file);
					LOG_PTHREAD_ERROR_CONTINUE(pthread_ret, interfacename, __LINE__);
					pthread_ret = pthread_mutex_lock(&pause_info[delayercom[0].pathnumber].mutex_pause);
					LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
					pause_info[delayercom[0].pathnumber].pause_time = mc_sender_arg[delayercom[0].pathnumber].stat_last_timestamp;
					find_time_ret = find_time_in_buffer(streaminterface.name, oldest_file.id, newest_file.id,
						pause_info[delayercom[0].pathnumber].pause_time, &pause_info[delayercom[0].pathnumber]);
					pthread_ret = pthread_mutex_unlock(&pause_info[delayercom[0].pathnumber].mutex_pause);
					LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
					pthread_ret = pthread_mutex_unlock(&read_file[delayercom[0].pathnumber].mutex_file);
					LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
					}
#ifdef NODAEMON_DEBUG
				fprintf(stderr, "%s[%"PRIu64"]: DELAYER_STATE_DELAYED -> PAUSE (id: %u time: %"PRIu64")\n",
						interfacename, delayercom[0].pathnumber,
						 pause_info[delayercom[0].pathnumber].pause_file_id, pause_info[delayercom[0].pathnumber].pause_time);
#endif
					break;
				default:
					break;
				}
				if (find_time_ret != 0)
				{
					add_trap_to_queue(TRAP_DELAYER_STOP, "Time are not in file");
					LOGMSG(LOG_INFO, "%s[%"PRIu64"]: Cant find the timestamp in the file (id: %u) -> FIX -> EXIT\n",
							interfacename, delayercom[0].pathnumber,
							pause_info[delayercom[0].pathnumber].pause_file_id);
					logtodb(&mutex_dbconn, dbconn, streaminterface.id, POVLOGLEVEL_INFO,
							"%s[%"PRIu64"]: cant find time in buffer file",
							interfacename, delayercom[0].pathnumber);
					break;
				}

				dstate[delayercom[0].pathnumber] = DELAYER_STATE_PAUSE;

				start_play_at[delayercom[0].pathnumber] = UINT64_MAX; //delayercom[0].startplayat_timestamp;
				play_to[delayercom[0].pathnumber] = UINT64_MAX; // delayercom[0].playto_timestamp;
				}
				break;
			case PLAYTO:
				play_to[delayercom[0].pathnumber] = delayercom[0].playto_timestamp;
				if (play_to[delayercom[0].pathnumber] == 0)
				{
					play_to[delayercom[0].pathnumber] = UINT64_MAX;
				}
#ifdef NODAEMON_DEBUG
				fprintf(stderr, "============ PLAYTO ================\n");
				fprintf(stderr, "%s:[%"PRIu64"]: playto %"PRIu64"\n",
						interfacename, delayercom[0].pathnumber,
						play_to[delayercom[0].pathnumber]);
#endif
				break;
			}
			
			pthread_ret = pthread_mutex_lock(&mutex_dbconn);
			LOG_PTHREAD_ERROR_CONTINUE(pthread_ret, interfacename, __LINE__);
			retval = db_delete_delayercomsbyid(&dbconn, delayercom[0].id);
			logtodb_nolock(dbconn, streaminterface.id, POVLOGLEVEL_INFO,
						"%s[%"PRIu64"] command has been executed",
						interfacename, delayercom[0].pathnumber);
			if (retval)
			{
				logtodb_nolock(dbconn, streaminterface.id, POVLOGLEVEL_INFO,
						"%s[%"PRIu64"]: failed to delete command",
						interfacename, delayercom[0].pathnumber);

				add_trap_to_queue(TRAP_DELAYER_STOP,
						"Can not delete command from database");
				LOGMSG(LOG_INFO, "%s[%"PRIu64"]: failed to delete (db_delete_delayercoms(%"PRIu64") %d)",
						interfacename, delayercom[0].pathnumber,
						delayercom[0].id,
						retval);

			}
			pthread_ret = pthread_mutex_unlock(&mutex_dbconn);
			LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
#ifdef NODAEMON_DEBUG
			fprintf(stderr, "%s:[%"PRIu64"]: PLAYTO %d == delayercom[0].e_action %d\n",
					interfacename, delayercom[0].pathnumber,
					PLAYTO, delayercom[0].e_action);
			fprintf(stderr, "%s:[%"PRIu64"]: streaminterface_id: %"PRIu64"\n",
					interfacename, delayercom[0].pathnumber,
					delayercom[0].streaminterface_id);
			fprintf(stderr, "%s:[%"PRIu64"]: patnumber: %"PRIu64"\n",
					interfacename, delayercom[0].pathnumber,
					delayercom[0].pathnumber);
			fprintf(stderr, "%s:[%"PRIu64"]: action: %s (%d)\n",
					interfacename, delayercom[0].pathnumber,
					delayercom[0].action, delayercom[0].e_action);
			fprintf(stderr, "%s:[%"PRIu64"]: jumptopos: %s (%"PRIu64")\n",
					interfacename, delayercom[0].pathnumber,
					delayercom[0].jumptopos, delayercom[0].jumptopos_timestamp);
			fprintf(stderr, "%s:[%"PRIu64"]: startplayat: %s (%"PRIu64")\n",
					interfacename, delayercom[0].pathnumber,
					delayercom[0].startplayat, delayercom[0].startplayat_timestamp);
			fprintf(stderr, "%s:[%"PRIu64"]: playto: %s (%"PRIu64")\n",
					interfacename, delayercom[0].pathnumber,
					delayercom[0].playto, delayercom[0].playto_timestamp);
#endif


			// FIX!!!!!!!!!!!!!!!!
			bool live = false;
			for (i = 0; i < streaminterface.num_backup_sink+1; i++)
			{
				if (dstate[i] == DELAYER_STATE_LIVESTREAMING)
				{
					live = true;
					break;
				}
			}
			// FIX!!!!!!!!!!!!!!!!
			if (live == false)
				dstate[streaminterface.num_backup_sink+1] = dstate[0];
			// FIX!!!!!!!!!!!!!!!!

			// Update state in one place instead of every case
			pthread_ret = pthread_mutex_lock(&mutex_stat);
			LOG_PTHREAD_ERROR_CONTINUE(pthread_ret, interfacename, __LINE__);
			copy_process_state_string(dstate[delayercom[0].pathnumber], d_stats[delayercom[0].pathnumber].state);
			pthread_ret = pthread_mutex_unlock(&mutex_stat);
			LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);

			reread_config_from_database = 0;
			pthread_ret = pthread_mutex_lock(&mutex_dbconn);
			LOG_PTHREAD_ERROR_CONTINUE(pthread_ret, interfacename, __LINE__);
			if (db_count_delayercoms(&dbconn, streaminterface.id) > 0)
				reread_config_from_database = 1;
			pthread_ret = pthread_mutex_unlock(&mutex_dbconn);
			LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
		}


		// ========= HANDLE start_play_at AND play_to ============
		gettimeofday(&tv, NULL);
		now = tv.tv_sec*1000000 + tv.tv_usec;
		for (i = 0; i < streaminterface.num_backup_sink+1; i++)
		{
			if (dstate[i] == DELAYER_STATE_PAUSE)
			{
				if (now >= start_play_at[i])
				{
					pthread_ret = pthread_mutex_lock(&pause_info[i].mutex_pause);
					LOG_PTHREAD_ERROR_CONTINUE(pthread_ret, interfacename, __LINE__);
					pause_info[i].pause_diff = now - pause_info[i].pause_time;
					pthread_ret = pthread_mutex_unlock(&pause_info[i].mutex_pause);
					LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);

					// trick getTimedoutPackets to get packets
					mc_sender_arg[i].buffer_timeout = pause_info[i].pause_diff/1000;

#ifdef NODAEMON_DEBUG
					fprintf(stderr, "%s[%d]: pasue_diff: %"PRIu64" - %"PRIu64" = %"PRIu64"\n",
							interfacename, i,
							(tv.tv_sec * 1000000 + tv.tv_usec),
							pause_info[i].pause_time,
							pause_info[i].pause_diff);
#endif

					dstate[i] = DELAYER_STATE_DELAYED;
					start_play_at[i] = UINT64_MAX;
					// Update state in one place instead of every case
					pthread_ret = pthread_mutex_lock(&mutex_stat);
					LOG_PTHREAD_ERROR_CONTINUE(pthread_ret, interfacename, __LINE__);
					copy_process_state_string(dstate[i], d_stats[i].state);
					pthread_ret = pthread_mutex_unlock(&mutex_stat);
					LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
				}
			} else { // DELAYER_STATE_LIVESTREAMING || DELAYER_STATE_DELAYED
				if (now >= play_to[i])
				{
#ifdef NODAEMON_DEBUG
					fprintf(stderr, "%s[%d]: !!\n\nnow %"PRIu64" >= play_to[%d] %"PRIu64"\n!!!\n\n",
							interfacename, i, now, i, play_to[i]);
#endif

					int find_time_ret=0;
					// Pause!
					switch (dstate[i])
					{
					case DELAYER_STATE_PAUSE:
#ifdef NODAEMON_DEBUG
				fprintf(stderr, "%s[%d]: DELAYER_STATE_PAUSE -> Pause WILL NOT HAPPEN\n",
						interfacename, i);
#endif
						// Do nothing - are already paused!
						break;
					case DELAYER_STATE_LIVESTREAMING:
						{
						pthread_ret = pthread_mutex_lock(&newest_file.mutex_file);
						LOG_PTHREAD_ERROR_CONTINUE(pthread_ret, interfacename, __LINE__);
						pthread_ret = pthread_mutex_lock(&pause_info[i].mutex_pause);
						LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
						pause_info[i].pause_time = mc_sender_arg[i].stat_last_timestamp;
						pause_info[i].pause_file_id = newest_file.id;

						if (pause_info[i].pause_time < newest_file.start_time)
						{
							//TODO Check so we are not on the oldest file and do -- to a removed file
#ifdef NODAEMON_DEBUG
							fprintf(stderr, "%s[%d]: NOT IN THE newest_file.id - before\n",
									interfacename, i);
#endif
							pause_info[i].pause_file_id--;
						}
						else if (pause_info[i].pause_time > newest_file.end_time)
						{
#ifdef NODAEMON_DEBUG
							fprintf(stderr, "%s[%d]: NOT IN THE newest_file.id - ERROR!!! set to endtime \n",
									interfacename, i);
#endif
							add_trap_to_queue(TRAP_DELAYER_STOP,
								"pause time are in future!");
							LOGMSG(LOG_INFO, "%s[%d]: pause time are in future",
								   interfacename, i);
							logtodb(&mutex_dbconn, dbconn, streaminterface.id, POVLOGLEVEL_INFO,
									"%s[%d]: pause time are in future",
									interfacename, i);
							pause_info[i].pause_time = newest_file.end_time;
						}
						find_time_ret = find_time(streaminterface.name, pause_info[i].pause_file_id,
							pause_info[i].pause_time, &pause_info[i]);
						pthread_ret = pthread_mutex_unlock(&pause_info[i].mutex_pause);
						LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
						pthread_ret = pthread_mutex_unlock(&newest_file.mutex_file);
						LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
						}
#ifdef NODAEMON_DEBUG
					fprintf(stderr, "%s[%d]: DELAYER_STATE_LIVESTREAMING(id: %u time: %"PRIu64")->PAUSE\n",
							interfacename, i,
							pause_info[i].pause_file_id, pause_info[i].pause_time);
#endif
						break;
					case DELAYER_STATE_DELAYED:
						{
						pthread_ret = pthread_mutex_lock(&read_file[i].mutex_file);
						LOG_PTHREAD_ERROR_CONTINUE(pthread_ret, interfacename, __LINE__);
						pthread_ret = pthread_mutex_lock(&pause_info[i].mutex_pause);
						LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
						pause_info[i].pause_time = mc_sender_arg[i].stat_last_timestamp;
						find_time_ret = find_time_in_buffer(streaminterface.name, oldest_file.id, newest_file.id,
							pause_info[i].pause_time, &pause_info[i]);
						pthread_ret = pthread_mutex_unlock(&pause_info[i].mutex_pause);
						LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
						pthread_ret = pthread_mutex_unlock(&read_file[i].mutex_file);
						LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
						}
#ifdef NODAEMON_DEBUG
					fprintf(stderr, "%s[%d]: DELAYER_STATE_DELAYED (id: %u time: %"PRIu64") -> PAUSE\n",
							interfacename, i,
							pause_info[i].pause_file_id, pause_info[i].pause_time);
#endif
						break;
					default:
#ifdef NODAEMON_DEBUG
						fprintf(stderr, "%s[%d]: default %d -> PAUSE\n",
								interfacename, i,
								dstate[i]);
#endif
						break;
					}
					dstate[i] = DELAYER_STATE_PAUSE;
					if (find_time_ret != 0)
					{
						add_trap_to_queue(TRAP_DELAYER_STOP, "Time are not in file");
						LOGMSG(LOG_INFO, "%s[%d]: Cant find the timestamp in the file (id: %u) -> FIX -> EXIT\n",
								interfacename, i, pause_info[i].pause_file_id);
						logtodb(&mutex_dbconn, dbconn, streaminterface.id, POVLOGLEVEL_INFO,
								"%s[%d]: Cant find the timestamp in buffer file",
								interfacename, i);
					}
					// Update state in one place instead of every case
					pthread_ret = pthread_mutex_lock(&mutex_stat);
					LOG_PTHREAD_ERROR_CONTINUE(pthread_ret, interfacename, __LINE__);
					copy_process_state_string(dstate[i], d_stats[i].state);
					pthread_ret = pthread_mutex_unlock(&mutex_stat);
					LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
				}
			}
		}

		// ========= UPDATE STATISTIC STRUCT =============
#ifdef TRYLOCK
		int tryCount = 0;
		do {
			pthread_ret = pthread_mutex_trylock(&mutex_stat);
			tryCount++;
			if (pthread_ret == EBUSY)
				sched_yield();
		} while (pthread_ret == EBUSY);
		if (tryCount > 5)
		{
			fprintf(stderr, "main: mutex_stat busy %d times\n", tryCount);
		}
#else
		pthread_ret = pthread_mutex_lock(&mutex_stat);
		LOG_PTHREAD_ERROR_CONTINUE(pthread_ret, interfacename, __LINE__);
#endif
		for (i = 0; i < streaminterface.num_backup_sink+1; i++)
		{
			d_stats[i].startplayat_timestamp = start_play_at[i] == UINT64_MAX ? 0 : start_play_at[i];
			d_stats[i].playto_timestamp = play_to[i] == UINT64_MAX ? 0 : play_to[i];

			d_stats[i].startpos_timestamp = oldest_file.start_time;
			d_stats[i].lastpos_timestamp = newest_file.end_time;
			if (dstate[i] == DELAYER_STATE_PAUSE)
			{
				d_stats[i].currpos_timestamp = pause_info[i].pause_time;
				mc_sender_arg[i].stat_last_timestamp = pause_info[i].pause_time;
			}
			else
			{
				d_stats[i].currpos_timestamp = mc_sender_arg[i].stat_last_timestamp;
			}

		}
		pthread_ret = pthread_mutex_unlock(&mutex_stat);
		LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);

		usleep(100000);
	} // END pstate != STATE_SHUTDOWN

SHUTDOWN:

	// === Wait for thread to shutdown ===
	syslog(LOG_WARNING, "%s: Join thread", interfacename);
	if (retval_thrd_mc_receiver == 0)
		pthread_join(thrd_mc_receiver, NULL);
	for (i = 0; i < streaminterface.num_backup_sink+1; i++)
	{
		if (retval_thrd_mc_sender[i] == 0)
			pthread_join(thrd_mc_sender[i], NULL);
	}

	sem_post(&semaphore);

	if (retval_thrd_statistic == 0)
		pthread_join(thrd_statistic, NULL);
	if (retval_thrd_traps == 0)
		pthread_join(thrd_traps, NULL);
	if (retval_thrd_pokepovmanager == 0)
		pthread_join(thrd_pokepovmanager, NULL);
#if 0
	// Kick on the disc_reader if its stuck
	if (retval_condt_disc_signal == 0 && retval_mutex_disc_signal == 0)
	{
		pthread_ret = pthread_mutex_lock(&mutex_disc_signal);
		LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
		pthread_cond_signal(&condt_disc_signal);
		pthread_ret = pthread_mutex_unlock(&mutex_disc_signal);
		LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
	}
#endif

	if (retval_condt_discclean_signal == 0 && retval_mutex_discclean_signal == 0)
	{
		pthread_ret = pthread_mutex_lock(&mutex_discclean_signal);
		LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
		pthread_cond_signal(&condt_discclean_signal);
		pthread_ret = pthread_mutex_unlock(&mutex_discclean_signal);
		LOG_PTHREAD_ERROR(pthread_ret, interfacename, __LINE__);
	}

	if (retval_thrd_discwriter == 0)
		pthread_join(thrd_discwriter, NULL);

	for (i = 0; i < streaminterface.num_backup_sink+1; i++)
	{
		if (retval_thrd_discreader[i] == 0)
			pthread_join(thrd_discreader[i], NULL);
	}

	if (retval_thrd_disccleaner == 0)
		pthread_join(thrd_disccleaner, NULL);

	if (retval_mutex_stat == 0)
		pthread_mutex_destroy(&mutex_stat);
	if (retval_mutex_traps == 0)
	{
		deinit_trap_queue();
		pthread_mutex_destroy(&mutex_traps);
	}
	if (retval_mutex_dbconn == 0)
		pthread_mutex_destroy(&mutex_dbconn);

	for (i = 0; i < streaminterface.num_backup_sink+1; i++)
	{
		if (retval_mutex_available_buffer[i] == 0)
			pthread_mutex_destroy(&buffer_q_available[i].mutex);

		if (retval_mutex_send_buffer[i] == 0)
			pthread_mutex_destroy(&buffer_q_send[i].mutex);
	}
	if (retval_mutex_available_buffer[streaminterface.num_backup_sink+1] == 0)
		pthread_mutex_destroy(&buffer_q_available[streaminterface.num_backup_sink+1].mutex);

	if (retval_mutex_disc_buffer == 0)
		pthread_mutex_destroy(&buffer_q_disc.mutex);
#if 0
	if (retval_mutex_disc_signal == 0)
		pthread_mutex_destroy(&mutex_disc_signal);
	if (retval_condt_disc_signal == 0)
		pthread_cond_destroy(&condt_disc_signal);
#endif

	if (retval_mutex_discclean_signal == 0)
		pthread_mutex_destroy(&mutex_discclean_signal);
	if (retval_condt_discclean_signal == 0)
		pthread_cond_destroy(&condt_discclean_signal);

	if (retval_mutex_newest_file == 0)
		pthread_mutex_destroy(&newest_file.mutex_file);
	if (retval_mutex_oldest_file == 0)
		pthread_mutex_destroy(&oldest_file.mutex_file);

	for (i = 0; i < streaminterface.num_backup_sink+1; i++)
	{
		if (retval_mutex_pause_info[i] == 0)
			pthread_mutex_destroy(&pause_info[i].mutex_pause);

		if (retval_mutex_read_file[i] == 0)
			pthread_mutex_destroy(&read_file[i].mutex_file);
	}


	// =========================================
	// === NO OTHER THREADS ARE RUNNING HERE ===

	// === Free buffers ===
	for (i = 0; i < streaminterface.num_backup_sink+1; i++)
	{
#ifdef NODAEMON_DEBUG
		fprintf(stderr, "free available[%d]\n", i);
#endif
		freeBufferElements(&buffer_q_available[i].buffer_elements);
#ifdef NODAEMON_DEBUG
		fprintf(stderr, "free send[%d]\n", i);
#endif
		freeBufferElements(&buffer_q_send[i].buffer_elements);
	}
#ifdef NODAEMON_DEBUG
		fprintf(stderr, "free available[%d]\n", streaminterface.num_backup_sink+1);
#endif
	freeBufferElements(&buffer_q_available[streaminterface.num_backup_sink+1].buffer_elements);
	freeBufferElements(&buffer_q_disc.buffer_elements);

	// === Semaphore cleanup ===
	if (retval_semaphore == 0)
	{
		retval = sem_destroy(&semaphore);
		if (retval == -1)
		{
			LOGMSG(LOG_ERR, "%s: Semaphore cleanup failed %s",
				   interfacename, strerror(errno));
		}
	}

	logtodb_nolock(dbconn, streaminterface.id, POVLOGLEVEL_INFO,
				   "%s stopped.", streaminterface.name);

	// Cleanup statistic tables from previous session
	retval = db_delete_streaminterfacelivestatistic(&dbconn, streaminterface.id);
	retval = db_delete_delayerstats(&dbconn, streaminterface.id);
	// Dont log if this fails.

	DBClose(dbconn);
	close_pid_file(pidfd, pidfilename);

	trap_send_exit(TRAP_DELAYER_STOP, "stopped.");
	LOGMSG(LOG_NOTICE, "%s: EXIT", interfacename);
	return 0;
}

