#ifndef __DBSYSTEMINFO_H__
#define __DBSYSTEMINFO_H__

#include "dbcommon.h"

#define VIDEO_PATH_LEN		256

//typedef struct _systeminfo{
// id | dbversion | systemname | statdb_remote_url | statdb_sleeptime | statdb_chunksize | labelapipassw
// | video_path
//	uint64_t id;
//	int dbversion;
//	char systemname[512];
//	char statdb_remote_url[512];
//	int statdb_sleeptime;
//	int statdb_chunksize;
//	char labelapipassw[512];
//	char video_path[1024];
//} dbsysteminfo;
//

int dbfetch_video_path(PGconn **conn, char *video_path);
#endif
