// __DBSTREAMSTATCPP_H__
#ifndef __DBSTREAMSTATCPP_H__
#define __DBSTREAMSTATCPP_H__

#include "dbcommon.h"
#include <string>
#include <vector>
namespace DBStreamStat
{
	typedef struct _StreamStat
	{
		uint64_t id;
		int streaminterface_id;
		std::string timestamp;
		std::string streammode;
		std::string source_address;
		int source_port;
		std::string sink_address;
		int sink_port;
		int drops;
		int bitrate;
		int bitrate_in;
		int bitrate_out;
		int resendcounter;
		int rudprtt;
		int pathnumber;
		std::string pathname;
		int linkdrops;
		bool multipath;
		bool mploadbalance;
		
	}Data;

	int db_fetch_by_id(PGconn *, uint64_t, DBStreamStat::Data *);
	int db_fetch_by_streaminterfaceid_between_timestamps(PGconn *, uint64_t, const std::string &, const std::string &, std::vector<DBStreamStat::Data> *);
	int db_fetch_by_streaminterfaceid_between_epoch_timestamps(PGconn *, uint64_t, const std::string &, const std::string &, std::vector<DBStreamStat::Data> *);
	int db_fetch_all(PGconn *, std::vector<DBStreamStat::Data> *);
}

#endif
