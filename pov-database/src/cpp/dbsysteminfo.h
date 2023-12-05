// __DBSYSTEMINFOCPP_H__
#ifndef __DBSYSTEMINFOCPP_H__
#define __DBSYSTEMINFOCPP_H__

#include "dbcommon.h"
#include <string>
namespace DBSystemInfo
{
	typedef struct _SystemData
	{
		uint64_t id;
		int dbversion;
		std::string systemname;
		std::string statdb_remote_url;
		int statdb_sleeptime;
		int statdb_chunksize;
		std::string labelapipassw;
	}Data;

	int db_fetch(PGconn *, DBSystemInfo::Data *);
	int db_fetch_password(PGconn *, std::string &);
}

#endif
