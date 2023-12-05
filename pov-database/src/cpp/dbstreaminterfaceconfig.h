// __DBSTREAMINTERFACECONFIGCPP_H__
#ifndef __DBSTREAMINTERFACECONFIGCPP_H__
#define __DBSTREAMINTERFACECONFIGCPP_H__

#include "dbcommon.h"
#include <string>
#include <vector>
namespace DBStreaminterfaceConfig
{
	typedef struct _StreaminterfaceData
	{
		uint64_t id;
		int streaminterface_id;
		std::string typeofconfig;
		bool autostart;
		int msbuffer;
		std::string resendmode;
		std::string mode;
		bool nosessionid;
		int reordersize;
		bool backuptoprimary;
		bool multipath;
		bool mploadbalance;
		bool mcinusecheck;
		bool actasserver;
		/*
		uint64_t id = 0;
		int streaminterface_id = 0;
		std::string typeofconfig = "";
		bool autostart = false;
		int msbuffer = 0;
		std::string resendmode = "";
		std::string mode = "";
		bool nosessionid = false;
		int reordersize = 32;
		bool backuptoprimary = false;;
		bool multipath = false;
		bool mploadbalance = false;
		bool mcinusecheck = false;
		bool actasserver = false;
		*/
	}Data;

	int db_fetch_by_id(PGconn *, uint64_t, DBStreaminterfaceConfig::Data *);
	int db_fetch_by_streaminterface_id(PGconn *, uint64_t, DBStreaminterfaceConfig::Data *);
	int db_fetch_all(PGconn *, std::vector<DBStreaminterfaceConfig::Data> *);
	int db_update_by_id(PGconn *, DBStreaminterfaceConfig::Data *);
	int db_update_by_streaminterface_id(PGconn *, DBStreaminterfaceConfig::Data *);
}

#endif
