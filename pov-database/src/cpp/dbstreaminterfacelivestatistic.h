// __DBSTREAMINTERFACELIVESTATISTICCPP_H__
#ifndef __DBSTREAMINTERFACELIVESTATISTICCPP_H__
#define __DBSTREAMINTERFACELIVESTATISTICCPP_H__

#include "dbcommon.h"
#include <string>
#include <vector>

namespace DBStreaminterfaceLivestatistic
{
	typedef struct _StreaminterfaceLivestatisticData
	{
		uint64_t id;
		int streaminterface_id;
		std::string mode;
		std::string state;
		int bitratein;
		int bitrateout;
		int bufferlevel;
		int rtt;
		int timestamp;
		std::string source;
		std::string destination;
		std::string udpdrops;
		int acc_rtp_drops;
		int pathnumber;
		std::string pathname;
		bool multipath;
		bool mploadbalance;
		int ttl;
		int hostclientcertificate_id;
		int talkbackclientcertificate_id;
	
	}Data;

		int db_fetch_by_id(PGconn *, uint64_t, DBStreaminterfaceLivestatistic::Data *);
		int db_fetch_all_by_streaminterface_id(PGconn *, uint64_t, std::vector<DBStreaminterfaceLivestatistic::Data> *);
		int db_fetch_all(PGconn *, std::vector<DBStreaminterfaceLivestatistic::Data> *);
}
#endif
