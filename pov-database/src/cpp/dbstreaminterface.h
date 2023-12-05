// __DBSTREAMINTERFACECPP_H__
#ifndef __DBSTREAMINTERFACECPP_H__
#define __DBSTREAMINTERFACECPP_H__

#include "dbcommon.h"
#include <string>
#include <vector>
namespace DBStreaminterface
{
	typedef struct _StreaminterfaceData
	{
		uint64_t id;
		std::string interfacetype;
		std::string name;
		std::string usertext;
	}Data;

	int db_fetch_by_id(PGconn *, uint64_t, DBStreaminterface::Data *);
	int db_fetch_all(PGconn *, std::vector<DBStreaminterface::Data> *);
}

#endif
