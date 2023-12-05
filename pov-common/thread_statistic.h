#ifndef __THREAD_STATISTIC_H__
#define __THREAD_STATISTIC_H__

#include "common.h"
#include <syslog.h>
#include "interfaces_common.h"
#include "checklicense.h"
#include "stat_func.h"
#include "dbstreaminterfaces.h"

#ifdef DELAYERD
#include "dbdelayers.h"
#include "delayer_common.h"
#endif
#if defined(RECORDERD)
#include "dbrecplay.h"
#endif
#if defined(PLAYERD)
#include "dbrecplay.h"
#include "recplay.h"
#endif

typedef struct _runtimestat
{
	uint64_t stat_bufferlevel;
	uint64_t stat_inbytes;
	uint64_t stat_outbytes;
	uint64_t laststattime;
	uint64_t stat_average_rtt[MAX_INTERFACE_PATH];
	uint64_t stat_udpdrops;
#ifdef RUDP_FILE
	uint64_t rf_id;
	uint64_t rf_transferredbytes;
#endif
} s_runtimestat;
//pthread_mutex_t mutex_stat;

#define STAT_AVERAGE_SECONDS 5
void *thread_statistic(void *arg);

// move to common or database later
typedef enum _process_mode
{
	TRANSMITTER,
	RECEIVER,
	FORWARDER,
	DELAYER,
	RECORDER,
	PLAYER,
	FILE_TRANSMITTER,
	FILE_RECEIVER,
	SRTTRANSMITTER,
	SRTRECEIVER
} e_process_mode;

typedef struct _statarg
{
	e_process_mode		process_mode;
	uint64_t		id;
	char			mode[DB_MAX_MODE_LENGTH];
	char			name[DB_MAX_NAME_LENGTH];
	int			num_paths;
	s_interfaceconfig 	*config;
	s_interfacesink		*sink;
	s_interfacesource	*source;
	PGconn			**dbconn;
	pthread_mutex_t		*mutex_dbconn;
	pthread_mutex_t		*mutex_stat;
	s_runtimestat		*rt_stats;
	sequence_statistics	*rudp_sequence_list;

#ifdef DELAYERD
	s_delayerstats		*d_stats;
	e_process_state		*dstate;
#endif
#ifdef RECORDERD
	s_recorderstats		*r_stats;
#endif
#ifdef PLAYERD
	s_playerstats		*p_stats;
	e_process_state		*dstate;
#endif
} s_statarg;


#endif
