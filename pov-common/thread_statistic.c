#include <sys/mman.h>
#include "thread_statistic.h"
#include "common.h"
#include "common-db.h"
#include "checklicense.h"
#include "states.h"
#include "buffer.h"
#include "thread_traps.h"

#ifdef DELAYERD
#include "dbdelayers.h"
#endif

#ifdef RUDP_FILE
#include "dbfile.h"
#endif

void *thread_statistic(void *arg)
{
	struct timeval tv;
	s_statarg *statarg = (s_statarg *)arg;

	s_interfacelivestatistic streaminterfacelivestatistic[statarg->num_paths+1];
	s_streamstats streamstats[statarg->num_paths+1];

	uint64_t last_inbytes_values[10], last_outbytes_values[10];
	uint64_t total_inbytes, total_outbytes;
	int curr_values_pos=0;
	int i;
	int pthread_ret;
#ifdef TRYLOCK
	int tryCount;
#endif
	uint64_t bufferlevel;
	uint64_t streamstat_time;
	uint64_t acc_rtp_drops = 0;
	bool	lost_input = false;
	bool	lost_output = false;
#ifdef RUDP_FILE
	s_file file_item;
#endif

	gettimeofday(&tv,NULL);
	statarg->rt_stats->laststattime=tv.tv_sec;
	streamstat_time=tv.tv_sec+10;
	
	memset(last_inbytes_values, 0, sizeof(uint64_t)*10);
	memset(last_outbytes_values, 0, sizeof(uint64_t)*10);
	memset(statarg->rt_stats->stat_average_rtt, 0, sizeof(uint64_t)*MAX_INTERFACE_PATH);

	switch (statarg->process_mode)
	{
	case TRANSMITTER:
	case SRTTRANSMITTER:
	case FILE_TRANSMITTER:
		{
		// on index streamstats[statarg->num_paths] the summary of all interfaces are located

		// sink_address TEXT, -- if transmitter use destination address
		strncpy(streamstats[statarg->num_paths].source_host,
				statarg->source[0].host,
				DB_MAX_HOSTNAME_LENGTH);
		strncpy(streamstats[statarg->num_paths].sink_host,
				statarg->sink[0].host,
				DB_MAX_HOSTNAME_LENGTH);

		streamstats[statarg->num_paths].streaminterface_id = statarg->id;
		strncpy(streamstats[statarg->num_paths].mode, statarg->mode, DB_MAX_MODE_LENGTH);
		streamstats[statarg->num_paths].source_port = statarg->source[0].port;
		streamstats[statarg->num_paths].sink_port = statarg->sink[0].port;
		streamstats[statarg->num_paths].pathnumber = 100;
		strncpy(streamstats[statarg->num_paths].pathname, statarg->source[0].name, DB_MAX_NAME_LENGTH);
		streamstats[statarg->num_paths].multipath = statarg->config->multipath;
		streamstats[statarg->num_paths].mploadbalance = statarg->config->mploadbalance;

		// ============ livestatistic =========
		snprintf(streaminterfacelivestatistic[statarg->num_paths].source,
			       	DB_MAX_HOSTNAME_PORT_LENGTH, "%s:%d",
				statarg->source[0].host,
				statarg->source[0].port);
		snprintf(streaminterfacelivestatistic[statarg->num_paths].destination,
			       	DB_MAX_HOSTNAME_PORT_LENGTH,
				"%s:%d",
				statarg->sink[0].host,
				statarg->sink[0].port);

		streaminterfacelivestatistic[statarg->num_paths].pathnumber = 100;
		strncpy(streaminterfacelivestatistic[statarg->num_paths].pathname, statarg->source[0].name, DB_MAX_NAME_LENGTH);
		strncpy(streaminterfacelivestatistic[statarg->num_paths].mode, statarg->mode, DB_MAX_MODE_LENGTH);
		streaminterfacelivestatistic[statarg->num_paths].streaminterface_id = statarg->id;
		streaminterfacelivestatistic[statarg->num_paths].ttl = statarg->sink[0].ttl;
		streaminterfacelivestatistic[statarg->num_paths].multipath = statarg->config->multipath;
		streaminterfacelivestatistic[statarg->num_paths].mploadbalance = statarg->config->mploadbalance;
		streaminterfacelivestatistic[statarg->num_paths].hostclientcertificate_id = 0;
		streaminterfacelivestatistic[statarg->num_paths].talkbackclientcertificate_id = 0;


		for (i = 0; i < statarg->num_paths; i++)
		{
			// sink_address TEXT, -- if transmitter use destination address
			strncpy(streamstats[i].source_host,
					statarg->source[i].host,
					DB_MAX_HOSTNAME_LENGTH);
			strncpy(streamstats[i].sink_host,
					statarg->sink[i].host,
					DB_MAX_HOSTNAME_LENGTH);

			streamstats[i].streaminterface_id = statarg->id;
			strncpy(streamstats[i].mode, statarg->mode, DB_MAX_MODE_LENGTH);
			streamstats[i].source_port = statarg->source[i].port;
			streamstats[i].sink_port = statarg->sink[i].port;
			streamstats[i].pathnumber = i;
			strncpy(streamstats[i].pathname, statarg->source[i].name, DB_MAX_NAME_LENGTH);
			streamstats[i].multipath = statarg->config->multipath;
			streamstats[i].mploadbalance = statarg->config->mploadbalance;

			// ============ livestatistic =========
			snprintf(streaminterfacelivestatistic[i].source, DB_MAX_HOSTNAME_PORT_LENGTH, "%s:%d",
					 statarg->source[0].host,
					statarg->source[0].port);
			snprintf(streaminterfacelivestatistic[i].destination, DB_MAX_HOSTNAME_PORT_LENGTH,
					 "%s:%d",
					 statarg->sink[i].host,
					 statarg->sink[i].port);
			streaminterfacelivestatistic[i].pathnumber = i;
			strncpy(streaminterfacelivestatistic[i].pathname, statarg->source[i].name, DB_MAX_NAME_LENGTH);
			strncpy(streaminterfacelivestatistic[i].mode, statarg->mode, DB_MAX_MODE_LENGTH);
			streaminterfacelivestatistic[i].streaminterface_id = statarg->id;
			streaminterfacelivestatistic[i].ttl = statarg->sink[0].ttl;
			streaminterfacelivestatistic[i].multipath = statarg->config->multipath;
			streaminterfacelivestatistic[i].mploadbalance = statarg->config->mploadbalance;
			streaminterfacelivestatistic[i].hostclientcertificate_id = statarg->sink[i].hostclientcertificate_id;
			streaminterfacelivestatistic[i].talkbackclientcertificate_id = statarg->sink[i].talkbackclientcertificate_id;
		}
		}
		break;
	case RECEIVER:
	case SRTRECEIVER:
	case FILE_RECEIVER:
		{
		// source_address TEXT, --if receiver use talkback address
		strncpy(streamstats[statarg->num_paths].source_host,
				statarg->source[0].talkbackhost,
				DB_MAX_HOSTNAME_LENGTH);
		strncpy(streamstats[statarg->num_paths].sink_host,
				statarg->sink[0].host,
				DB_MAX_HOSTNAME_LENGTH);

		streamstats[statarg->num_paths].streaminterface_id = statarg->id;
		strncpy(streamstats[statarg->num_paths].mode, statarg->mode, DB_MAX_MODE_LENGTH);
		streamstats[statarg->num_paths].source_port = statarg->source[0].port;
		streamstats[statarg->num_paths].sink_port = statarg->sink[0].port;
		streamstats[statarg->num_paths].pathnumber = 100;
		strncpy(streamstats[statarg->num_paths].pathname, statarg->source[0].name, DB_MAX_NAME_LENGTH);
		streamstats[statarg->num_paths].multipath = statarg->config->multipath;
		streamstats[statarg->num_paths].mploadbalance = statarg->config->mploadbalance;

		// ============ livestatistic =========
		snprintf(streaminterfacelivestatistic[statarg->num_paths].source, DB_MAX_HOSTNAME_PORT_LENGTH, "%s:%d",
				statarg->source[0].host,
				statarg->source[0].port);
		snprintf(streaminterfacelivestatistic[statarg->num_paths].destination, DB_MAX_HOSTNAME_PORT_LENGTH, "%s:%d",
				statarg->sink[0].host,
				statarg->sink[0].port);
		streaminterfacelivestatistic[statarg->num_paths].pathnumber = 100;
		strncpy(streaminterfacelivestatistic[statarg->num_paths].pathname, statarg->source[0].name, DB_MAX_NAME_LENGTH);
		strncpy(streaminterfacelivestatistic[statarg->num_paths].mode, statarg->mode, DB_MAX_MODE_LENGTH);
		streaminterfacelivestatistic[statarg->num_paths].streaminterface_id = statarg->id;
		streaminterfacelivestatistic[statarg->num_paths].ttl = statarg->sink[0].ttl;
		streaminterfacelivestatistic[statarg->num_paths].multipath = statarg->config->multipath;
		streaminterfacelivestatistic[statarg->num_paths].mploadbalance = statarg->config->mploadbalance;
		streaminterfacelivestatistic[statarg->num_paths].hostclientcertificate_id = 0;
		streaminterfacelivestatistic[statarg->num_paths].talkbackclientcertificate_id = 0;
		


		for (i = 0; i < statarg->num_paths; i++)
		{
			// source_address TEXT, --if receiver use talkback address
			strncpy(streamstats[i].source_host,
					statarg->source[i].talkbackhost,
					DB_MAX_HOSTNAME_LENGTH);
			strncpy(streamstats[i].sink_host,
					statarg->sink[i].host,
					DB_MAX_HOSTNAME_LENGTH);

			streamstats[i].streaminterface_id = statarg->id;
			strncpy(streamstats[i].mode, statarg->mode, DB_MAX_MODE_LENGTH);
			streamstats[i].source_port = statarg->source[i].port;
			streamstats[i].sink_port = statarg->sink[i].port;
			streamstats[i].pathnumber = i;
			strncpy(streamstats[i].pathname, statarg->source[i].name, DB_MAX_NAME_LENGTH);
			streamstats[i].multipath = statarg->config->multipath;
			streamstats[i].mploadbalance = statarg->config->mploadbalance;

			// ============ livestatistic =========
			snprintf(streaminterfacelivestatistic[i].source, DB_MAX_HOSTNAME_PORT_LENGTH, "%s:%d",
					 statarg->source[i].host,
					 statarg->source[i].port);
			snprintf(streaminterfacelivestatistic[i].destination, DB_MAX_HOSTNAME_PORT_LENGTH, "%s:%d",
					 statarg->sink[0].host,
					statarg->sink[0].port);

			streaminterfacelivestatistic[i].pathnumber = i;
			strncpy(streaminterfacelivestatistic[i].pathname, statarg->source[i].name, DB_MAX_NAME_LENGTH);
			strncpy(streaminterfacelivestatistic[i].mode, statarg->mode, DB_MAX_MODE_LENGTH);
			streaminterfacelivestatistic[i].streaminterface_id = statarg->id;
			streaminterfacelivestatistic[i].ttl = statarg->sink[0].ttl;
			streaminterfacelivestatistic[i].multipath = statarg->config->multipath;
			streaminterfacelivestatistic[i].mploadbalance = statarg->config->mploadbalance;
			streaminterfacelivestatistic[i].hostclientcertificate_id = statarg->source[i].hostclientcertificate_id;
			streaminterfacelivestatistic[i].talkbackclientcertificate_id = statarg->source[i].talkbackclientcertificate_id;
			
		}
		}
		break;
	case FORWARDER:
		{
		// TODO: Copied from transmitter above, needs to be adjusted for forwader
		
		// on index streamstats[statarg->num_paths] the summary of all interfaces are located

		// sink_address TEXT, -- if transmitter use destination address
		strncpy(streamstats[statarg->num_paths].source_host,
				statarg->source[0].host,
				DB_MAX_HOSTNAME_LENGTH);
		strncpy(streamstats[statarg->num_paths].sink_host,
				statarg->sink[0].host,
				DB_MAX_HOSTNAME_LENGTH);

		streamstats[statarg->num_paths].streaminterface_id = statarg->id;
		strncpy(streamstats[statarg->num_paths].mode, statarg->mode, DB_MAX_MODE_LENGTH);
		streamstats[statarg->num_paths].source_port = statarg->source[0].port;
		streamstats[statarg->num_paths].sink_port = statarg->sink[0].port;
		streamstats[statarg->num_paths].pathnumber = 100;
		strncpy(streamstats[statarg->num_paths].pathname, statarg->source[0].name, DB_MAX_NAME_LENGTH);
		streamstats[statarg->num_paths].multipath = statarg->config->multipath;
		streamstats[statarg->num_paths].mploadbalance = statarg->config->mploadbalance;

		// ============ livestatistic =========
		snprintf(streaminterfacelivestatistic[statarg->num_paths].source,
			       	DB_MAX_HOSTNAME_PORT_LENGTH, "%s:%d",
				statarg->source[0].host,
				statarg->source[0].port);
		snprintf(streaminterfacelivestatistic[statarg->num_paths].destination,
			       	DB_MAX_HOSTNAME_PORT_LENGTH,
				"%s:%d",
				statarg->sink[0].host,
				statarg->sink[0].port);

		streaminterfacelivestatistic[statarg->num_paths].pathnumber = 100;
		strncpy(streaminterfacelivestatistic[statarg->num_paths].pathname, statarg->source[0].name, DB_MAX_NAME_LENGTH);
		strncpy(streaminterfacelivestatistic[statarg->num_paths].mode, statarg->mode, DB_MAX_MODE_LENGTH);
		streaminterfacelivestatistic[statarg->num_paths].streaminterface_id = statarg->id;
		streaminterfacelivestatistic[statarg->num_paths].ttl = statarg->sink[0].ttl;
		streaminterfacelivestatistic[statarg->num_paths].multipath = statarg->config->multipath;
		streaminterfacelivestatistic[statarg->num_paths].mploadbalance = statarg->config->mploadbalance;
		streaminterfacelivestatistic[statarg->num_paths].hostclientcertificate_id = 0;
		streaminterfacelivestatistic[statarg->num_paths].talkbackclientcertificate_id = 0;


		for (i = 0; i < statarg->num_paths; i++)
		{
			// sink_address TEXT, -- if transmitter use destination address
			strncpy(streamstats[i].source_host,
					statarg->source[i].host,
					DB_MAX_HOSTNAME_LENGTH);
			strncpy(streamstats[i].sink_host,
					statarg->sink[i].host,
					DB_MAX_HOSTNAME_LENGTH);

			streamstats[i].streaminterface_id = statarg->id;
			strncpy(streamstats[i].mode, statarg->mode, DB_MAX_MODE_LENGTH);
			streamstats[i].source_port = statarg->source[i].port;
			streamstats[i].sink_port = statarg->sink[i].port;
			streamstats[i].pathnumber = i;
			strncpy(streamstats[i].pathname, statarg->source[i].name, DB_MAX_NAME_LENGTH);
			streamstats[i].multipath = statarg->config->multipath;
			streamstats[i].mploadbalance = statarg->config->mploadbalance;

			// ============ livestatistic =========
			snprintf(streaminterfacelivestatistic[i].source, DB_MAX_HOSTNAME_PORT_LENGTH,
				       	"%s:%d",
					 statarg->source[0].host,
					statarg->source[0].port);
			snprintf(streaminterfacelivestatistic[i].destination, DB_MAX_HOSTNAME_PORT_LENGTH,
					 "%s:%d",
					 statarg->sink[i].host,
					 statarg->sink[i].port);
			streaminterfacelivestatistic[i].pathnumber = i;
			strncpy(streaminterfacelivestatistic[i].pathname, statarg->source[i].name, DB_MAX_NAME_LENGTH);
			strncpy(streaminterfacelivestatistic[i].mode, statarg->mode, DB_MAX_MODE_LENGTH);
			streaminterfacelivestatistic[i].streaminterface_id = statarg->id;
			streaminterfacelivestatistic[i].ttl = statarg->sink[0].ttl;
			streaminterfacelivestatistic[i].multipath = statarg->config->multipath;
			streaminterfacelivestatistic[i].mploadbalance = statarg->config->mploadbalance;
			streaminterfacelivestatistic[i].hostclientcertificate_id = statarg->sink[i].hostclientcertificate_id;
			streaminterfacelivestatistic[i].talkbackclientcertificate_id = statarg->sink[i].talkbackclientcertificate_id;
		}
		}
		break;
	case DELAYER:
		{
		// From transmitter
		strncpy(streamstats[statarg->num_paths].source_host,
				statarg->source[0].host,
				DB_MAX_HOSTNAME_LENGTH);
		strncpy(streamstats[statarg->num_paths].sink_host,
				statarg->sink[0].host,
				DB_MAX_HOSTNAME_LENGTH);

		streamstats[statarg->num_paths].streaminterface_id = statarg->id;
		strncpy(streamstats[statarg->num_paths].mode, statarg->mode, DB_MAX_MODE_LENGTH);
		streamstats[statarg->num_paths].source_port = statarg->source[0].port;
		streamstats[statarg->num_paths].sink_port = statarg->sink[0].port;
		streamstats[statarg->num_paths].pathnumber = 100;
		strncpy(streamstats[statarg->num_paths].pathname, statarg->source[0].name, DB_MAX_NAME_LENGTH);
		streamstats[statarg->num_paths].multipath = statarg->config->multipath;
		streamstats[statarg->num_paths].mploadbalance = statarg->config->mploadbalance;

		// ============ livestatistic =========
		snprintf(streaminterfacelivestatistic[statarg->num_paths].source,
			       	DB_MAX_HOSTNAME_PORT_LENGTH, "%s:%d",
				statarg->source[0].host,
				statarg->source[0].port);
		snprintf(streaminterfacelivestatistic[statarg->num_paths].destination,
			       	DB_MAX_HOSTNAME_PORT_LENGTH,
				"%s:%d",
				statarg->sink[0].host,
				statarg->sink[0].port);

		streaminterfacelivestatistic[statarg->num_paths].pathnumber = 100;
		strncpy(streaminterfacelivestatistic[statarg->num_paths].pathname,
			statarg->source[0].name, DB_MAX_NAME_LENGTH);
		strncpy(streaminterfacelivestatistic[statarg->num_paths].mode,
			statarg->mode, DB_MAX_MODE_LENGTH);
		streaminterfacelivestatistic[statarg->num_paths].streaminterface_id = statarg->id;
		streaminterfacelivestatistic[statarg->num_paths].ttl = statarg->sink[0].ttl;
		streaminterfacelivestatistic[statarg->num_paths].multipath = statarg->config->multipath;
		streaminterfacelivestatistic[statarg->num_paths].mploadbalance = statarg->config->mploadbalance;
		streaminterfacelivestatistic[statarg->num_paths].hostclientcertificate_id = 0;
		streaminterfacelivestatistic[statarg->num_paths].talkbackclientcertificate_id = 0;
		for (i = 0; i < statarg->num_paths; i++)
		{
			// sink_address TEXT, -- if transmitter use destination address
			strncpy(streamstats[i].source_host,
					statarg->source[i].host,
					DB_MAX_HOSTNAME_LENGTH);
			strncpy(streamstats[i].sink_host,
					statarg->sink[i].host,
					DB_MAX_HOSTNAME_LENGTH);

			streamstats[i].streaminterface_id = statarg->id;
			strncpy(streamstats[i].mode, statarg->mode, DB_MAX_MODE_LENGTH);
			streamstats[i].source_port = statarg->source[i].port;
			streamstats[i].sink_port = statarg->sink[i].port;
			streamstats[i].pathnumber = i;
			strncpy(streamstats[i].pathname, statarg->source[i].name, DB_MAX_NAME_LENGTH);
			streamstats[i].multipath = statarg->config->multipath;
			streamstats[i].mploadbalance = statarg->config->mploadbalance;

			// ============ livestatistic =========
			snprintf(streaminterfacelivestatistic[i].source, DB_MAX_HOSTNAME_PORT_LENGTH,
				       	"%s:%d",
					statarg->source[0].host,
					statarg->source[0].port);
			snprintf(streaminterfacelivestatistic[i].destination, DB_MAX_HOSTNAME_PORT_LENGTH,
					"%s:%d",
					statarg->sink[i].host,
					statarg->sink[i].port);
			streaminterfacelivestatistic[i].pathnumber = i;
			strncpy(streaminterfacelivestatistic[i].pathname, statarg->source[i].name, DB_MAX_NAME_LENGTH);
			strncpy(streaminterfacelivestatistic[i].mode, statarg->mode, DB_MAX_MODE_LENGTH);
			streaminterfacelivestatistic[i].streaminterface_id = statarg->id;
			streaminterfacelivestatistic[i].ttl = statarg->sink[0].ttl;
			streaminterfacelivestatistic[i].multipath = statarg->config->multipath;
			streaminterfacelivestatistic[i].mploadbalance = statarg->config->mploadbalance;
			streaminterfacelivestatistic[i].hostclientcertificate_id = statarg->sink[i].hostclientcertificate_id;
			streaminterfacelivestatistic[i].talkbackclientcertificate_id = statarg->sink[i].talkbackclientcertificate_id;
		}
		}
		break;

	case RECORDER:
		{
		strncpy(streamstats[statarg->num_paths].source_host,
				statarg->source[0].host,
				DB_MAX_HOSTNAME_LENGTH);
		streamstats[statarg->num_paths].sink_host[0] = '\0';

		streamstats[statarg->num_paths].streaminterface_id = statarg->id;
		strncpy(streamstats[statarg->num_paths].mode, statarg->mode, DB_MAX_MODE_LENGTH);
		streamstats[statarg->num_paths].source_port = statarg->source[0].port;
		streamstats[statarg->num_paths].sink_port = 0;
		streamstats[statarg->num_paths].pathnumber = 100;
		strncpy(streamstats[statarg->num_paths].pathname, statarg->source[0].name, DB_MAX_NAME_LENGTH);
		streamstats[statarg->num_paths].multipath = statarg->config->multipath;
		streamstats[statarg->num_paths].mploadbalance = statarg->config->mploadbalance;

		snprintf(streaminterfacelivestatistic[statarg->num_paths].source,
				DB_MAX_HOSTNAME_PORT_LENGTH, "%s:%d",
				statarg->source[0].host,
				statarg->source[0].port);
		streaminterfacelivestatistic[statarg->num_paths].destination[0] = '\0';

		streaminterfacelivestatistic[statarg->num_paths].pathnumber = 100;
		strncpy(streaminterfacelivestatistic[statarg->num_paths].pathname,
			statarg->source[0].name, DB_MAX_NAME_LENGTH);
		strncpy(streaminterfacelivestatistic[statarg->num_paths].mode,
			statarg->mode, DB_MAX_MODE_LENGTH);
		streaminterfacelivestatistic[statarg->num_paths].streaminterface_id = statarg->id;
		streaminterfacelivestatistic[statarg->num_paths].ttl = 0;
		streaminterfacelivestatistic[statarg->num_paths].multipath = statarg->config->multipath;
		streaminterfacelivestatistic[statarg->num_paths].mploadbalance = statarg->config->mploadbalance;
		streaminterfacelivestatistic[statarg->num_paths].hostclientcertificate_id = 0;
		streaminterfacelivestatistic[statarg->num_paths].talkbackclientcertificate_id = 0;

		for (i = 0; i < statarg->num_paths; i++)
		{
			strncpy(streamstats[i].source_host,
					statarg->source[0].host,
					DB_MAX_HOSTNAME_LENGTH);
			streamstats[i].sink_host[0] = '\0';

			streamstats[i].streaminterface_id = statarg->id;
			strncpy(streamstats[i].mode, statarg->mode, DB_MAX_MODE_LENGTH);
			streamstats[i].source_port = statarg->source[0].port;
			streamstats[i].sink_port = 0;
			streamstats[i].pathnumber = i;
			strncpy(streamstats[i].pathname, statarg->source[0].name, DB_MAX_NAME_LENGTH);
			streamstats[i].multipath = statarg->config->multipath;
			streamstats[i].mploadbalance = statarg->config->mploadbalance;

			snprintf(streaminterfacelivestatistic[i].source,
					DB_MAX_HOSTNAME_PORT_LENGTH,
					"%s:%d",
					statarg->source[0].host,
					statarg->source[0].port);
			streaminterfacelivestatistic[i].destination[0] = '\0';
			streaminterfacelivestatistic[i].pathnumber = i;
			strncpy(streaminterfacelivestatistic[i].pathname, statarg->source[0].name, DB_MAX_NAME_LENGTH);
			strncpy(streaminterfacelivestatistic[i].mode, statarg->mode, DB_MAX_MODE_LENGTH);
			streaminterfacelivestatistic[i].streaminterface_id = statarg->id;
			streaminterfacelivestatistic[i].ttl = 0;
			streaminterfacelivestatistic[i].multipath = statarg->config->multipath;
			streaminterfacelivestatistic[i].mploadbalance = statarg->config->mploadbalance;
			streaminterfacelivestatistic[i].hostclientcertificate_id = 0;
			streaminterfacelivestatistic[i].talkbackclientcertificate_id = 0;
		}
		}
		break;

	case PLAYER:
		{
		// From transmitter
		strncpy(streamstats[statarg->num_paths].source_host,
				statarg->source[0].host,
				DB_MAX_HOSTNAME_LENGTH);
		strncpy(streamstats[statarg->num_paths].sink_host,
				statarg->sink[0].host,
				DB_MAX_HOSTNAME_LENGTH);

		streamstats[statarg->num_paths].streaminterface_id = statarg->id;
		strncpy(streamstats[statarg->num_paths].mode, statarg->mode, DB_MAX_MODE_LENGTH);
		streamstats[statarg->num_paths].source_port = statarg->source[0].port;
		streamstats[statarg->num_paths].sink_port = statarg->sink[0].port;
		streamstats[statarg->num_paths].pathnumber = 100;
		strncpy(streamstats[statarg->num_paths].pathname, statarg->source[0].name, DB_MAX_NAME_LENGTH);
		streamstats[statarg->num_paths].multipath = statarg->config->multipath;
		streamstats[statarg->num_paths].mploadbalance = statarg->config->mploadbalance;

		// ============ livestatistic =========
		snprintf(streaminterfacelivestatistic[statarg->num_paths].source,
			       	DB_MAX_HOSTNAME_PORT_LENGTH, "%s:%d",
				statarg->source[0].host,
				statarg->source[0].port);
		snprintf(streaminterfacelivestatistic[statarg->num_paths].destination,
			       	DB_MAX_HOSTNAME_PORT_LENGTH,
				"%s:%d",
				statarg->sink[0].host,
				statarg->sink[0].port);

		streaminterfacelivestatistic[statarg->num_paths].pathnumber = 100;
		strncpy(streaminterfacelivestatistic[statarg->num_paths].pathname,
			statarg->source[0].name, DB_MAX_NAME_LENGTH);
		strncpy(streaminterfacelivestatistic[statarg->num_paths].mode,
			statarg->mode, DB_MAX_MODE_LENGTH);
		streaminterfacelivestatistic[statarg->num_paths].streaminterface_id = statarg->id;
		streaminterfacelivestatistic[statarg->num_paths].ttl = statarg->sink[0].ttl;
		streaminterfacelivestatistic[statarg->num_paths].multipath = statarg->config->multipath;
		streaminterfacelivestatistic[statarg->num_paths].mploadbalance = statarg->config->mploadbalance;
		streaminterfacelivestatistic[statarg->num_paths].hostclientcertificate_id = 0;
		streaminterfacelivestatistic[statarg->num_paths].talkbackclientcertificate_id = 0;
		for (i = 0; i < statarg->num_paths; i++)
		{
			// sink_address TEXT, -- if transmitter use destination address
			strncpy(streamstats[i].source_host,
					statarg->source[i].host,
					DB_MAX_HOSTNAME_LENGTH);
			strncpy(streamstats[i].sink_host,
					statarg->sink[i].host,
					DB_MAX_HOSTNAME_LENGTH);

			streamstats[i].streaminterface_id = statarg->id;
			strncpy(streamstats[i].mode, statarg->mode, DB_MAX_MODE_LENGTH);
			streamstats[i].source_port = statarg->source[i].port;
			streamstats[i].sink_port = statarg->sink[i].port;
			streamstats[i].pathnumber = i;
			strncpy(streamstats[i].pathname, statarg->source[i].name, DB_MAX_NAME_LENGTH);
			streamstats[i].multipath = statarg->config->multipath;
			streamstats[i].mploadbalance = statarg->config->mploadbalance;

			// ============ livestatistic =========
			snprintf(streaminterfacelivestatistic[i].source, DB_MAX_HOSTNAME_PORT_LENGTH,
				       	"%s:%d",
					statarg->source[0].host,
					statarg->source[0].port);
			snprintf(streaminterfacelivestatistic[i].destination,
				       	DB_MAX_HOSTNAME_PORT_LENGTH,
					"%s:%d",
					statarg->sink[i].host,
					statarg->sink[i].port);
			streaminterfacelivestatistic[i].pathnumber = i;
			strncpy(streaminterfacelivestatistic[i].pathname, statarg->source[i].name, DB_MAX_NAME_LENGTH);
			strncpy(streaminterfacelivestatistic[i].mode, statarg->mode, DB_MAX_MODE_LENGTH);
			streaminterfacelivestatistic[i].streaminterface_id = statarg->id;
			streaminterfacelivestatistic[i].ttl = statarg->sink[0].ttl;
			streaminterfacelivestatistic[i].multipath = statarg->config->multipath;
			streaminterfacelivestatistic[i].mploadbalance = statarg->config->mploadbalance;
			streaminterfacelivestatistic[i].hostclientcertificate_id = statarg->sink[i].hostclientcertificate_id;
			streaminterfacelivestatistic[i].talkbackclientcertificate_id = statarg->sink[i].talkbackclientcertificate_id;
		}
		}
		break;

	default:
		break;
	} // switch (process_mode)

	while (pstate != STATE_SHUTDOWN)
	{
		gettimeofday(&tv, NULL);

		if (streamstat_time < tv.tv_sec)
		{
			streamstat_time = tv.tv_sec+9;
			double timediff = ((tv.tv_sec-statarg->rudp_sequence_list->last_tv.tv_sec)*1000000ULL+
							   tv.tv_usec-statarg->rudp_sequence_list->last_tv.tv_usec)/1000000.00;

			// === GRAB STATISTICS FROM SHARED VARIABLES ===
#ifdef TRYLOCK
			tryCount = 0;
			do {
				pthread_ret = pthread_mutex_trylock(statarg->mutex_stat);
				tryCount++;
				if (pthread_ret == EBUSY)
					sched_yield();
			} while (pthread_ret == EBUSY);
			if (tryCount > 5)
			{
				LOGMSG(LOG_WARNING, "thread_statistic: mutex_stat #1 busy %d times",
					tryCount);
			}
#else
			pthread_ret = pthread_mutex_lock(statarg->mutex_stat);
			if (pthread_ret != 0)
			{
				LOGMSG(LOG_WARNING, "pthread_mutex_lock failed (%d)",
					pthread_ret);
			}
#endif

			uint64_t transfered_bytes_mc_udp = statarg->rudp_sequence_list->transfered_bytes_mc_udp * 8 / timediff;
			uint64_t transfered_bytes_rudp = statarg->rudp_sequence_list->transfered_bytes_rudp * 8 / timediff;
			uint32_t dropped_10_sec = statarg->rudp_sequence_list->dropped_10_sec;
#if 0
			fprintf(stderr, "MC %"PRIu64 " rudp %"PRIu64,
				statarg->rudp_sequence_list->transfered_bytes_mc_udp,
				statarg->rudp_sequence_list->transfered_bytes_rudp);
#endif

			for (i = 0; i < statarg->num_paths + 1; i++)
			{
				streamstats[i].drops = statarg->rudp_sequence_list->dropped_10_sec;
				streamstats[i].resendcounter = statarg->rudp_sequence_list->resend_counter;
				streamstats[i].rudprtt = statarg->rt_stats->stat_average_rtt[i];

				streamstats[i].linkdrops = 0;
			}

			switch (statarg->process_mode)
			{
			case TRANSMITTER:
			case SRTTRANSMITTER:
			case FILE_TRANSMITTER:
				{
				for (i = 0; i < statarg->num_paths; i++)
				{
					statarg->rudp_sequence_list->acc_transfered_bytes_rudp_per_path[i] += statarg->rudp_sequence_list->transfered_bytes_rudp_per_path[i];
					streamstats[i].bitrate_out = statarg->rudp_sequence_list->acc_transfered_bytes_rudp_per_path[i] * 8 / timediff;
					streamstats[i].bitrate_in = transfered_bytes_mc_udp;
					streamstats[i].bitrate = streamstats[i].bitrate_out;
					statarg->rudp_sequence_list->acc_transfered_bytes_rudp_per_path[i] = 0;
				}
				}
				break;
			case RECEIVER:
			case SRTRECEIVER:
			case FILE_RECEIVER:
				{
				for (i = 0; i < statarg->num_paths; i++)
				{
					statarg->rudp_sequence_list->acc_transfered_bytes_rudp_per_path[i] += statarg->rudp_sequence_list->transfered_bytes_rudp_per_path[i];
					streamstats[i].bitrate_out = transfered_bytes_mc_udp;
					streamstats[i].bitrate_in = statarg->rudp_sequence_list->acc_transfered_bytes_rudp_per_path[i] * 8 / timediff;
					streamstats[i].bitrate = streamstats[i].bitrate_in;
					statarg->rudp_sequence_list->acc_transfered_bytes_rudp_per_path[i] = 0;
				}
				}
				break;
			case FORWARDER:
				{
				for (i = 0; i < statarg->num_paths; i++)
				{
					statarg->rudp_sequence_list->acc_transfered_bytes_rudp_per_path[i] += statarg->rudp_sequence_list->transfered_bytes_rudp_per_path[i];
					//streamstats[i].bitrate_out = statarg->rudp_sequence_list->acc_transfered_bytes_rudp_per_path[i] * 8 / timediff;
					streamstats[i].bitrate_out = statarg->rt_stats->stat_outbytes * 8 ;
					streamstats[i].bitrate_in = statarg->rt_stats->stat_inbytes * 8 ;
					//streamstats[i].bitrate_in = transfered_bytes_mc_udp;
					streamstats[i].bitrate = streamstats[i].bitrate_out;
					statarg->rudp_sequence_list->acc_transfered_bytes_rudp_per_path[i] = 0;
				}
				}
				break;
			case DELAYER:
				for (i = 0; i < statarg->num_paths; i++)
				{
					//statarg->rudp_sequence_list->acc_transfered_bytes_rudp_per_path[i] += statarg->rudp_sequence_list->transfered_bytes_rudp_per_path[i];
					streamstats[i].bitrate_out = statarg->rudp_sequence_list->acc_transfered_bytes_rudp_per_path[i] * 8 / timediff;
#ifdef DELAYERD
					if (statarg->dstate[i] == DELAYER_STATE_PAUSE)
					{
						streamstats[i].bitrate_out = 0;
					}
#endif
					streamstats[i].bitrate_in = statarg->rt_stats->stat_inbytes * 8 ;
					streamstats[i].bitrate = streamstats[i].bitrate_out;
					statarg->rudp_sequence_list->acc_transfered_bytes_rudp_per_path[i] = 0;
				}
				break;
			case RECORDER:
				for (i = 0; i < statarg->num_paths; i++)
				{
					streamstats[i].bitrate_out = 0;
					streamstats[i].bitrate_in = transfered_bytes_mc_udp;
					streamstats[i].bitrate = streamstats[i].bitrate_in;
					statarg->rudp_sequence_list->acc_transfered_bytes_rudp_per_path[i] = 0;
					statarg->rudp_sequence_list->transfered_bytes_rudp_per_path[i] = 0;
				}
				break;
			case PLAYER:
				for (i = 0; i < statarg->num_paths; i++)
				{

					//statarg->rudp_sequence_list->acc_transfered_bytes_rudp_per_path[i] += statarg->rudp_sequence_list->transfered_bytes_rudp_per_path[i];
					streamstats[i].bitrate_out = statarg->rudp_sequence_list->acc_transfered_bytes_rudp_per_path[i] * 8 / timediff;

#ifdef PLAYERD
					if (statarg->dstate[i] == PLAYER_STATE_PAUSE)
					{
						streamstats[i].bitrate_out = 0;
					}
#endif
					streamstats[i].bitrate_in = statarg->rt_stats->stat_inbytes * 8 ;
					streamstats[i].bitrate = streamstats[i].bitrate_out;
					statarg->rudp_sequence_list->acc_transfered_bytes_rudp_per_path[i] = 0;
				}
				break;
			default:
				{
				LOGMSG(LOG_ERR, "%s: Illegal mode!",
					statarg->name);
				pstate = STATE_SHUTDOWN;
				continue;
				}
				break;
			} // switch (process_mode)

			statarg->rudp_sequence_list->dropped_10_sec = 0;
			statarg->rudp_sequence_list->transfered_bytes_mc_udp = 0;
			statarg->rudp_sequence_list->transfered_bytes_rudp = 0;
			statarg->rudp_sequence_list->resend_counter = 0;
			statarg->rudp_sequence_list->last_tv = tv;
			pthread_mutex_unlock(statarg->mutex_stat);

			switch (statarg->process_mode)
			{
			case TRANSMITTER:
			case SRTTRANSMITTER:
				{
				streamstats[statarg->num_paths].bitrate_out = transfered_bytes_rudp;
				streamstats[statarg->num_paths].bitrate_in = transfered_bytes_mc_udp;
				streamstats[statarg->num_paths].bitrate = streamstats[statarg->num_paths].bitrate_out;

				if (streamstats[statarg->num_paths].bitrate_in == 0 && lost_input == false)
				{
					add_trap_to_queue(TRAP_TRANSMITTER_LOST_INPUT, "Video stream input lost");
					lost_input = true;
				}
				else if (lost_input && streamstats[statarg->num_paths].bitrate_in > 0)
				{
					add_trap_to_queue(TRAP_TRANSMITTER_RESTORED_INPUT, "Video stream input restored");
					lost_input = false;
				}

				if (dropped_10_sec)
				{
					add_trap_to_queue(TRAP_TRANSMITTER_FAULTY_INPUT, "Video stream input has sequence gaps");
				}
				}
				break;

			case RECEIVER:
			case SRTRECEIVER:
				{
				streamstats[statarg->num_paths].bitrate_out = transfered_bytes_mc_udp;
				streamstats[statarg->num_paths].bitrate_in = transfered_bytes_rudp;
				streamstats[statarg->num_paths].bitrate = streamstats[statarg->num_paths].bitrate_in;

				if (streamstats[statarg->num_paths].bitrate_out == 0 && lost_output == false)
				{
					add_trap_to_queue(TRAP_RECEIVER_LOST_OUTPUT, "Video stream output lost");
					lost_output = true;
				}
				else if (lost_output && streamstats[statarg->num_paths].bitrate_out > 0)
				{
					add_trap_to_queue(TRAP_RECEIVER_RESTORED_OUTPUT, "Video stream output restored");
					lost_output = false;
				}

				if (dropped_10_sec)
				{
					add_trap_to_queue(TRAP_RECEIVER_FAULTY_OUTPUT, "Video stream output has sequence gaps");
				}
				}
				break;
			case FORWARDER:
				{
				//TODO: Copied from transmitter above, needs to be adjusted for forwarder
				streamstats[statarg->num_paths].bitrate_out = statarg->rt_stats->stat_outbytes * 8 ;
				streamstats[statarg->num_paths].bitrate_in = statarg->rt_stats->stat_inbytes * 8 ;
				streamstats[statarg->num_paths].bitrate = streamstats[statarg->num_paths].bitrate_out;

				if (streamstats[statarg->num_paths].bitrate_in == 0 && lost_input == false)
				{
					add_trap_to_queue(TRAP_FORWARDER_LOST_INPUT, "Video stream input lost");
					lost_input = true;
				}
				else if (lost_input && streamstats[statarg->num_paths].bitrate_in > 0)
				{
					add_trap_to_queue(TRAP_FORWARDER_RESTORED_INPUT, "Video stream input restored");
					lost_input = false;
				}

				if (dropped_10_sec)
				{
					add_trap_to_queue(TRAP_FORWARDER_FAULTY_INPUT, "Video stream input has sequence gaps");
				}
				}
				break;
			case DELAYER:
				{
				streamstats[statarg->num_paths].bitrate_out = statarg->rt_stats->stat_outbytes * 8;
				streamstats[statarg->num_paths].bitrate_in = statarg->rt_stats->stat_inbytes * 8;
				streamstats[statarg->num_paths].bitrate = streamstats[statarg->num_paths].bitrate_out;

				if (streamstats[statarg->num_paths].bitrate_in == 0 && lost_input == false)
				{
					add_trap_to_queue(TRAP_DELAYER_LOST_INPUT, "Video stream input lost");
					lost_input = true;
				}
				else if (lost_input && streamstats[statarg->num_paths].bitrate_in > 0)
				{
					add_trap_to_queue(TRAP_DELAYER_RESTORED_INPUT, "Video stream input restored");
					lost_input = false;
				}

				if (dropped_10_sec)
				{
					add_trap_to_queue(TRAP_DELAYER_FAULTY_INPUT, "Video stream input has sequence gaps");
				}
				}
				break;
			case RECORDER:
				{
				streamstats[statarg->num_paths].bitrate_out = 0;
				streamstats[statarg->num_paths].bitrate_in = transfered_bytes_mc_udp;
				streamstats[statarg->num_paths].bitrate = streamstats[statarg->num_paths].bitrate_in;

				if (streamstats[statarg->num_paths].bitrate_in == 0 && lost_input == false)
				{
					add_trap_to_queue(TRAP_RECORDER_LOST_INPUT, "Video stream input lost");
					lost_input = true;
				}
				else if (lost_input && streamstats[statarg->num_paths].bitrate_in > 0)
				{
					add_trap_to_queue(TRAP_RECORDER_RESTORED_INPUT, "Video stream input restored");
					lost_input = false;
				}

				if (dropped_10_sec)
				{
					add_trap_to_queue(TRAP_RECORDER_FAULTY_INPUT, "Video stream input has sequence gaps");
				}
				}
				break;
			case PLAYER:
				{
				streamstats[statarg->num_paths].bitrate_out = statarg->rt_stats->stat_outbytes * 8;
				streamstats[statarg->num_paths].bitrate_in = statarg->rt_stats->stat_inbytes * 8;
				streamstats[statarg->num_paths].bitrate = streamstats[statarg->num_paths].bitrate_out;

				if (streamstats[statarg->num_paths].bitrate_in == 0 && lost_input == false)
				{
					add_trap_to_queue(TRAP_PLAYER_LOST_INPUT, "Video stream input lost");
					lost_input = true;
				}
				else if (lost_input && streamstats[statarg->num_paths].bitrate_in > 0)
				{
					add_trap_to_queue(TRAP_PLAYER_RESTORED_INPUT, "Video stream input restored");
					lost_input = false;
				}

				if (dropped_10_sec)
				{
					add_trap_to_queue(TRAP_PLAYER_FAULTY_INPUT, "Video stream input has sequence gaps");
				}
				}
				break;
			case FILE_TRANSMITTER:
				{
				streamstats[statarg->num_paths].bitrate_out = transfered_bytes_rudp;
				streamstats[statarg->num_paths].bitrate_in = transfered_bytes_mc_udp;
				streamstats[statarg->num_paths].bitrate = streamstats[statarg->num_paths].bitrate_out;
				}
				break;
			case FILE_RECEIVER:
				{
				streamstats[statarg->num_paths].bitrate_out = transfered_bytes_mc_udp;
				streamstats[statarg->num_paths].bitrate_in = transfered_bytes_rudp;
				streamstats[statarg->num_paths].bitrate = streamstats[statarg->num_paths].bitrate_in;
				}
				break;
			default:
				break;
			} // switch (process_mode)
			
			// === INSERT INTO DB ===
#ifdef TRYLOCK
			tryCount = 0;
			do {
				pthread_ret = pthread_mutex_trylock(statarg->mutex_dbconn);
				tryCount++;
				if (pthread_ret == EBUSY)
					sched_yield();
			} while (pthread_ret == EBUSY);
			if (tryCount > 5)
			{
				LOGMSG(LOG_WARNING, "thread_statistic: mutex_dbconn #1 busy %d times",
					tryCount);
			}
#else
			pthread_ret = pthread_mutex_lock(statarg->mutex_dbconn);
			if (pthread_ret != 0)
			{
				LOGMSG(LOG_WARNING, "pthread_mutex_lock failed (%d)",
					pthread_ret);
			}
#endif
			for (i = 0; i < statarg->num_paths+1; i++)
			{
				dbinsertstreamstats(statarg->dbconn, &streamstats[i]);
			}
			pthread_mutex_unlock(statarg->mutex_dbconn);
		}

		// one second between updating database record
		if (statarg->rt_stats->laststattime < tv.tv_sec)
		{
			// === UPDATE SHARED STATS VARIABLES ===
#ifdef TRYLOCK
			tryCount = 0;
			do {
				pthread_ret = pthread_mutex_trylock(statarg->mutex_stat);
				tryCount++;
				if (pthread_ret == EBUSY)
					sched_yield();
			} while (pthread_ret == EBUSY);
			if (tryCount > 5)
			{
				LOGMSG(LOG_WARNING, "thread_statistic: mutex_stat #2 busy %d times",
					tryCount);
			}
#else
			pthread_ret = pthread_mutex_lock(statarg->mutex_stat);
			if (pthread_ret != 0)
			{
				LOGMSG(LOG_WARNING, "pthread_mutex_lock failed (%d)",
					pthread_ret);
			}
#endif
			bufferlevel = statarg->rt_stats->stat_bufferlevel;
			last_inbytes_values[curr_values_pos] = statarg->rt_stats->stat_inbytes;
			last_outbytes_values[curr_values_pos] = statarg->rt_stats->stat_outbytes;
			statarg->rt_stats->stat_inbytes=0;
			statarg->rt_stats->stat_outbytes=0;

			acc_rtp_drops += statarg->rudp_sequence_list->dropped_1_sec;
			statarg->rudp_sequence_list->dropped_1_sec = 0;

#ifdef RUDP_FILE
			file_item.id = statarg->rt_stats->rf_id;
			file_item.transferredbytes = statarg->rt_stats->rf_transferredbytes;
#endif
			pthread_mutex_unlock(statarg->mutex_stat);

			// === RESET STAT COUNTERS ===
			curr_values_pos = (curr_values_pos + 1) %  STAT_AVERAGE_SECONDS;
			total_inbytes=0;
			total_outbytes=0;

			// === SUM LAST 10 SEC IN/OUT BYTES COUNT ===
			for (i = 0; i <  STAT_AVERAGE_SECONDS ; i++)
			{
				total_inbytes += last_inbytes_values[i];
				total_outbytes += last_outbytes_values[i];
			}

			// === FILL THE streaminterfacelivestatistic STRUCT ===
			switch (statarg->process_mode)
			{
			case FILE_TRANSMITTER:
				// Fall through
			case SRTTRANSMITTER:
			case TRANSMITTER:
				{
				for (i = 0; i < statarg->num_paths; i++)
				{
					streaminterfacelivestatistic[i].bitratein = total_inbytes*8 /  STAT_AVERAGE_SECONDS;
					streaminterfacelivestatistic[i].bitrateout = statarg->rudp_sequence_list->transfered_bytes_rudp_per_path[i]*8; // approx 1sec
					streaminterfacelivestatistic[i].bufferlevel = bufferlevel;
					streaminterfacelivestatistic[i].udpdrops = statarg->rt_stats->stat_udpdrops;
					streaminterfacelivestatistic[i].rtt = statarg->rt_stats->stat_average_rtt[i];
					streaminterfacelivestatistic[i].acc_rtp_drops = acc_rtp_drops;
					snprintf(streaminterfacelivestatistic[i].state, DB_MAX_STATE, "%s", "sending");
					statarg->rudp_sequence_list->acc_transfered_bytes_rudp_per_path[i] += statarg->rudp_sequence_list->transfered_bytes_rudp_per_path[i];
					statarg->rudp_sequence_list->transfered_bytes_rudp_per_path[i] = 0;
				}
				}
				break;

			case FILE_RECEIVER:
				// Fall through

			case SRTRECEIVER:
			case RECEIVER:
				{
				for (i = 0; i < statarg->num_paths; i++)
				{
					streaminterfacelivestatistic[i].bitratein = statarg->rudp_sequence_list->transfered_bytes_rudp_per_path[i]*8; // approx 1sec
					streaminterfacelivestatistic[i].bitrateout = total_outbytes*8 / STAT_AVERAGE_SECONDS;
					streaminterfacelivestatistic[i].bufferlevel = bufferlevel;
					streaminterfacelivestatistic[i].udpdrops = statarg->rt_stats->stat_udpdrops;
					streaminterfacelivestatistic[i].rtt = statarg->rt_stats->stat_average_rtt[0];
					streaminterfacelivestatistic[i].acc_rtp_drops = acc_rtp_drops;
					snprintf(streaminterfacelivestatistic[i].state, DB_MAX_STATE, "%s", "sending");
					statarg->rudp_sequence_list->acc_transfered_bytes_rudp_per_path[i] += statarg->rudp_sequence_list->transfered_bytes_rudp_per_path[i];
					statarg->rudp_sequence_list->transfered_bytes_rudp_per_path[i] = 0;
				}
				}
				break;
			case FORWARDER:
				{
				// TODO: Copied from transmitter above, needs to be adjusted for forwarder
				for (i = 0; i < statarg->num_paths; i++)
				{
					streaminterfacelivestatistic[i].bitratein = total_inbytes*8 /  STAT_AVERAGE_SECONDS;
					streaminterfacelivestatistic[i].bitrateout = statarg->rudp_sequence_list->transfered_bytes_rudp_per_path[i]*8; // approx 1sec
					streaminterfacelivestatistic[i].bufferlevel = bufferlevel;
					streaminterfacelivestatistic[i].udpdrops = statarg->rt_stats->stat_udpdrops;
					streaminterfacelivestatistic[i].rtt = statarg->rt_stats->stat_average_rtt[i];
					streaminterfacelivestatistic[i].acc_rtp_drops = acc_rtp_drops;
					snprintf(streaminterfacelivestatistic[i].state, DB_MAX_STATE, "%s", "sending");
					statarg->rudp_sequence_list->acc_transfered_bytes_rudp_per_path[i] += statarg->rudp_sequence_list->transfered_bytes_rudp_per_path[i];
					statarg->rudp_sequence_list->transfered_bytes_rudp_per_path[i] = 0;
				}
				
				}
				break;
			case DELAYER:
				for (i = 0; i < statarg->num_paths; i++)
				{
					streaminterfacelivestatistic[i].bitratein = total_inbytes*8 /  STAT_AVERAGE_SECONDS;
					streaminterfacelivestatistic[i].bitrateout = statarg->rudp_sequence_list->transfered_bytes_rudp_per_path[i]*8; // approx 1sec
#ifdef DELAYERD
					if (statarg->dstate[i] == DELAYER_STATE_PAUSE)
					{
						streaminterfacelivestatistic[i].bitrateout = 0;
					}
#endif
					streaminterfacelivestatistic[i].bufferlevel = bufferlevel;
					streaminterfacelivestatistic[i].udpdrops = statarg->rt_stats->stat_udpdrops;
					streaminterfacelivestatistic[i].rtt = statarg->rt_stats->stat_average_rtt[i];
					streaminterfacelivestatistic[i].acc_rtp_drops = acc_rtp_drops;
					snprintf(streaminterfacelivestatistic[i].state, DB_MAX_STATE, "%s", "sending");
					statarg->rudp_sequence_list->acc_transfered_bytes_rudp_per_path[i] += statarg->rudp_sequence_list->transfered_bytes_rudp_per_path[i];
					statarg->rudp_sequence_list->transfered_bytes_rudp_per_path[i] = 0;
				}
				break;
			case RECORDER:
				for (i = 0; i < statarg->num_paths; i++)
				{
					streaminterfacelivestatistic[i].bitratein = total_inbytes*8 / STAT_AVERAGE_SECONDS;
					streaminterfacelivestatistic[i].bitrateout = 0;
					streaminterfacelivestatistic[i].bufferlevel = bufferlevel;
					streaminterfacelivestatistic[i].udpdrops = statarg->rt_stats->stat_udpdrops;
					streaminterfacelivestatistic[i].rtt = statarg->rt_stats->stat_average_rtt[i];
					streaminterfacelivestatistic[i].acc_rtp_drops = acc_rtp_drops;
					snprintf(streaminterfacelivestatistic[i].state, DB_MAX_STATE, "%s", "recording");
					statarg->rudp_sequence_list->acc_transfered_bytes_rudp_per_path[i] = 0;
					statarg->rudp_sequence_list->transfered_bytes_rudp_per_path[i] = 0;
				}
				break;
			case PLAYER:
				for (i = 0; i < statarg->num_paths; i++)
				{
					streaminterfacelivestatistic[i].bitratein = total_inbytes*8 /  STAT_AVERAGE_SECONDS;
					streaminterfacelivestatistic[i].bitrateout = statarg->rudp_sequence_list->transfered_bytes_rudp_per_path[i]*8; // approx 1sec
#ifdef PLAYERD
					if (statarg->dstate[i] == PLAYER_STATE_PAUSE)
					{
						streaminterfacelivestatistic[i].bitrateout = 0;
					}
#endif
					streaminterfacelivestatistic[i].bufferlevel = bufferlevel;
					streaminterfacelivestatistic[i].udpdrops = statarg->rt_stats->stat_udpdrops;
					streaminterfacelivestatistic[i].rtt = statarg->rt_stats->stat_average_rtt[i];
					streaminterfacelivestatistic[i].acc_rtp_drops = acc_rtp_drops;
					snprintf(streaminterfacelivestatistic[i].state, DB_MAX_STATE, "%s", "sending");
					statarg->rudp_sequence_list->acc_transfered_bytes_rudp_per_path[i] += statarg->rudp_sequence_list->transfered_bytes_rudp_per_path[i];
					statarg->rudp_sequence_list->transfered_bytes_rudp_per_path[i] = 0;
				}
				break;
			default:
				break;
			} // switch (process_mode)

			// For both paths
			streaminterfacelivestatistic[statarg->num_paths].bitratein = total_inbytes*8 /  STAT_AVERAGE_SECONDS;
			streaminterfacelivestatistic[statarg->num_paths].bitrateout = total_outbytes*8 / STAT_AVERAGE_SECONDS;
			streaminterfacelivestatistic[statarg->num_paths].bufferlevel = bufferlevel;
			streaminterfacelivestatistic[statarg->num_paths].udpdrops = statarg->rt_stats->stat_udpdrops;
			streaminterfacelivestatistic[statarg->num_paths].rtt = statarg->rt_stats->stat_average_rtt[0];
			streaminterfacelivestatistic[statarg->num_paths].acc_rtp_drops = acc_rtp_drops;
			if (statarg->process_mode == RECORDER)
			{
				snprintf(streaminterfacelivestatistic[statarg->num_paths].state, DB_MAX_STATE, "%s", "recording");
			}
			else
			{
				snprintf(streaminterfacelivestatistic[statarg->num_paths].state, DB_MAX_STATE, "%s", "sending");
			}
			
			// === UPDATE DB WITH STATS ABOUT THE INTERFACE ===
#ifdef TRYLOCK
			tryCount = 0;
			do {
				pthread_ret = pthread_mutex_trylock(statarg->mutex_dbconn);
				tryCount++;
				if (pthread_ret == EBUSY)
					sched_yield();
			} while (pthread_ret == EBUSY);
			if (tryCount > 5)
			{
				LOGMSG(LOG_WARNING, "thread_statistic: mutex_dbconn #2 busy %d times",
					tryCount);
			}
#else
			pthread_ret = pthread_mutex_lock(statarg->mutex_dbconn);
			if (pthread_ret != 0)
			{
				LOGMSG(LOG_WARNING, "pthread_mutex_lock failed (%d)",
					pthread_ret);
			}
#endif
			for (i = 0; i < statarg->num_paths; i++)
			{
				dbupdatestreaminterfacelivestatistic(statarg->dbconn, &streaminterfacelivestatistic[i]);
#ifdef DELAYERD
				db_insert_delayerstats(statarg->dbconn, &(statarg->d_stats[i]));
#endif
#ifdef RECORDERD
				db_insert_recorderstats(statarg->dbconn, &(statarg->r_stats[i]));
#endif
#ifdef PLAYERD
				db_insert_playerstats(statarg->dbconn, &(statarg->p_stats[i]));
#endif
			}
			// update summary path 100
			dbupdatestreaminterfacelivestatistic(statarg->dbconn, &streaminterfacelivestatistic[statarg->num_paths]);

#ifdef RUDP_FILE
			if (file_item.id != 0)
			{
				db_update_file_transferredbytes(statarg->dbconn, &file_item);
			}
#endif

			pthread_mutex_unlock(statarg->mutex_dbconn);
			statarg->rt_stats->laststattime = tv.tv_sec;
		}
		usleep(100000);
	}

	LOGMSG(LOG_NOTICE, "%s: EXIT thread_statistic",
		statarg->name);

	return NULL;
}
