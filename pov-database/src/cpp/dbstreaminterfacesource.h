// __DBSTREAMINTERFACESOURCECPP_H__
#ifndef __DBSTREAMINTERFACESOURCECPP_H__
#define __DBSTREAMINTERFACESOURCECPP_H__

#include "dbcommon.h"
#include <string>
#include <vector>
namespace DBStreaminterfaceSource
{
	typedef struct _StreaminterfaceSourceData
	{
		uint64_t id;
		int streaminterfaceconfig_id;
		std::string name;
		int priority;
		std::string sourcetype;
		std::string host;
		int port;
		std::string talkbackhost;
		int talkbackport;
		int virtualsource_id;
		int vsourcerevision;
		int hostclientcertificate_id;
		int talkbackclientcertificate_id;
		int maxretrytime;
		int maxretries;
		int streammissfailovertimeout;
		int recording_id;
		bool addrtp;

	}Data;

	int db_fetch_by_id(PGconn *, uint64_t, DBStreaminterfaceSource::Data *);
	int db_fetch_by_streaminterfaceconfig_id(PGconn *, uint64_t, DBStreaminterfaceSource::Data *);
	int db_fetch_all(PGconn *, std::vector<DBStreaminterfaceSource::Data> *);
	int db_update_by_id(PGconn *, DBStreaminterfaceSource::Data *);
	int db_update_by_streaminterfaceconfig_id(PGconn *, DBStreaminterfaceSource::Data *);

}

#endif
