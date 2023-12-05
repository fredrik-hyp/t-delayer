#ifndef __DBSYSMONS_H__
#define __DBSYSMONS_H__

#include "dbcommon.h"
#include "common.h"

typedef struct _dbsysmon
{
	uint64_t id;
	uint64_t timestamp;
	int nrofcpus;
	float cpu1min;
	float cpu5min;
	float cpu15min;
	int reallocsectors[NO_OF_SDA_PARTITIONS];
}dbsysmon;
	
int dbupdatesysmon(PGconn **conn, dbsysmon *sysmon);	

#endif
