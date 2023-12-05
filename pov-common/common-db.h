/*
 * common settings for transmitter and receiver
 */
#ifndef __COMMON_DB_H__
#define __COMMON_DB_H__

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <stdarg.h>
#include <pthread.h>
#include <ctype.h>

#include "dbcommon.h"
#include "dbstreaminterfaces.h"

// MAX_INTERFACE_PATH = 1 maininterface + up to DB_MAX_SOURCE-1 redundant paths
#define MAX_INTERFACE_PATH			DB_MAX_SOURCE

int logtodb_nolock(PGconn *dbconn, uint64_t streaminterfaceid, int loglevel, char *fmt, ...);
int logtodb(pthread_mutex_t *mutex_dbconn, PGconn *dbconn, uint64_t streaminterfaceid, int loglevel, char *fmt, ...);
int logtodbsys(pthread_mutex_t *mutex_dbconn, PGconn *dbconn, uint64_t streaminterfaceid, int loglevel, char *fmt, ...);

#endif // __COMMON_DB_H__
