#ifndef __DBCOMMON_H__
#define __DBCOMMON_H__

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <inttypes.h>
#include <libpq-fe.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>

#define DB_MAX_MODE_LENGTH 64
#define DB_MAX_NAME_LENGTH 256
// actually 253 including dots according to wikipedia
#define DB_MAX_HOSTNAME_LENGTH 256
#define DB_MAX_HOSTNAME_PORT_LENGTH 1024
#define DB_MAX_CERTIFICATE_LEN 2048

#define POVLOGLEVEL_ERR 1
#define POVLOGLEVEL_WARN 2
#define POVLOGLEVEL_INFO 3

#define DB_MAX_STARTSTOPINFO		256
#define DB_MAX_TIMESTAMP		256
#define DB_MAX_TYPEOFCONFIG		16
#define DB_MAX_STREAMINTERFACETYPE	24
#define DB_MAX_INTERFACETYPE		10
#define DB_MAX_RESENDMODE		16
#define DB_MAX_STATE			32
#define DB_MAX_LOGTEXT			1024
#define DB_MAX_SOURCE			8
#define DB_MAX_SINK			8
#define DB_MAX_NR_OF_STREAM_PATHS 9

#define SMALL_QUERY_LEN		128
#define MEDIUM_QUERY_LEN	512
#define BIG_QUERY_LEN		1024
#define DOUBLE_BIG_QUERY_LEN	2048

#define MAX_STREAMINTERFACES 256

PGconn *DBConnect();
void DBClose(PGconn *dbconn);
int check_DB_connection_and_reconnect(PGconn **dbconn);

char *pqescape (char *input);
int DBCount(PGconn **dbconn, int *result, char *table);

int DBIsAvailable();

#endif
