// __DBSTREAMINTERFACESINKCPP_H__
#ifndef __DBSTREAMINTERFACESINKCPP_H__
#define __DBSTREAMINTERFACESINKCPP_H__

#include "dbcommon.h"
#include <string>
#include <vector>
namespace DBStreaminterfaceSink
{
	typedef struct _StreaminterfaceSinkData
	{
		uint64_t id;
		int streaminterfaceconfig_id;
		std::string name;
		std::string sinktype;
		std::string host;
		int port;
		std::string talkbackhost;
		int talkbackport;
		int ttl;
		int hostclientcertificate_id;
		int talkbackclientcertificate_id;
		int priority;
		int maxretrytime;
		int maxretries;
		int streammissfailovertimeout;
		int maxbps;		
		bool rawudp;
		std::string outnic;
		int recording_id;
	}Data;

	int db_fetch_by_id(PGconn *, uint64_t, DBStreaminterfaceSink::Data *);
	int db_fetch_by_streaminterfaceconfig_id(PGconn *, uint64_t, DBStreaminterfaceSink::Data *);
	int db_fetch_all(PGconn *, std::vector<DBStreaminterfaceSink::Data> *);
	int db_update_by_id(PGconn *, DBStreaminterfaceSink::Data *);
	int db_update_by_streaminterfaceconfig_id(PGconn *, DBStreaminterfaceSink::Data *);
}

#endif
