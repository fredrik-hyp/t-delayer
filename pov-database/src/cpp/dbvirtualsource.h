// __DBVIRTUALSOURCECPP_H__
#ifndef __DBVIRTUALSOURCECPP_H__
#define __DBVIRTUALSOURCECPP_H__

#include "dbcommon.h"
#include <string>
#include <vector>
namespace DBVirtualSource
{
	typedef struct _VirtualSourceData
	{
		uint64_t id;
		int revision;
		std::string name;
		std::string httpurl;
		std::string url;
		std::string address;
		int port;
		int virtualsourcegroup_id;
	}Data;

	int db_fetch_by_id(PGconn *, uint64_t, DBVirtualSource::Data *);
	int db_fetch_all(PGconn *, std::vector<DBVirtualSource::Data> *);
}

#endif
