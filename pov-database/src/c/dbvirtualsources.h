#ifndef __DBVIRTUALSOURCES_H__
#define __DBVIRTUALSOURCES_H__
#include "dbcommon.h"

typedef struct _dbvirtualourcegroup{
    uint64_t id;
    uint64_t vsourcegroupid;
    char name[1024];
}dbvirtualsourcegroup;

typedef struct _dbvirtualsource{
	uint64_t id;
	uint64_t revision;
	char name[256];
	char httpurl[1024];
	char address[256];
	uint16_t port;
    uint64_t vsourcegroupid;
	}dbvirtualsource;

int dbaddvirtualsourcegroup(PGconn **conn, dbvirtualsourcegroup *vsourcegroup);
int dbfetchvirtualsourcegroupbyid(PGconn **conn, dbvirtualsourcegroup *vsourcegroup, uint64_t id);
int dbdeletevirtualsourcegroup(PGconn **conn, uint64_t id);
int dbfetchallvirtualsourcegroups(PGconn **conn, dbvirtualsourcegroup ***vsourcegroup, int *numfetched, int sortbyid);

int dbaddorupdatevirtualsource(PGconn **conn, dbvirtualsource *vsource);
int dbfetchvirtualsourcebyid(PGconn **conn, dbvirtualsource *vsource, uint64_t id);
int dbfetchvirtualsourcebyname(PGconn **conn, dbvirtualsource *vsource, char *name);
int dbfetchvirtualsourcebyhttpurl(PGconn **conn, dbvirtualsource *vsource, char *httpurl);
int dbdeletevirtualsourcebyid(PGconn **conn, uint64_t id);

// returns number of rows, caller must deallocate memory
int dbfetchallvirtualsources(PGconn **conn, dbvirtualsource ***vsources, int sortbyid, int *numrows);

#endif
