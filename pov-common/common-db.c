#include "common-db.h"
#include "common.h"

int logtodb_nolock(PGconn *dbconn, uint64_t streaminterfaceid, int loglevel, char *fmt, ...)
{
	s_interfacelog streaminterfacelog;
	memset(&streaminterfacelog, 0, sizeof(s_interfacelog));

	va_list ap;
	va_start(ap, fmt);
	vsnprintf(streaminterfacelog.text, DB_MAX_LOGTEXT, fmt, ap);
	va_end(ap);

	streaminterfacelog.streaminterface_id = streaminterfaceid;
	streaminterfacelog.level = loglevel;
	int retdbadd;
	retdbadd = dbaddstreaminterfacelog(&dbconn, &streaminterfacelog);
	if (retdbadd)
	{
		LOGMSG(LOG_WARNING, "st%"PRIu64": Failed to add to db: %s %d",
			streaminterfaceid, streaminterfacelog.text, retdbadd);
	}
	return retdbadd;
}

int logtodb(pthread_mutex_t *mutex_dbconn, PGconn *dbconn, uint64_t streaminterfaceid, int loglevel, char *fmt, ...)
{
	s_interfacelog streaminterfacelog;
	memset(&streaminterfacelog, 0, sizeof(s_interfacelog));

	va_list ap;
	va_start(ap, fmt);
	vsnprintf(streaminterfacelog.text, DB_MAX_LOGTEXT, fmt, ap);
	va_end(ap);

	streaminterfacelog.streaminterface_id = streaminterfaceid;
	streaminterfacelog.level = loglevel;
	int retdbadd;
	pthread_mutex_lock(mutex_dbconn);
	retdbadd = dbaddstreaminterfacelog(&dbconn, &streaminterfacelog);
	pthread_mutex_unlock(mutex_dbconn);
	if (retdbadd)
	{
		LOGMSG(LOG_WARNING, "st%"PRIu64": Failed to add to db: %s %d",
			streaminterfaceid, streaminterfacelog.text, retdbadd);
	}
	return retdbadd;
}

int logtodbsys(pthread_mutex_t *mutex_dbconn, PGconn *dbconn, uint64_t streaminterfaceid, int loglevel, char *fmt, ...)
{
	s_interfacelog streaminterfacelog;
	memset(&streaminterfacelog, 0, sizeof(s_interfacelog));

	va_list ap;
	va_start(ap, fmt);
	vsnprintf(streaminterfacelog.text, DB_MAX_LOGTEXT, fmt, ap);
	va_end(ap);

	LOGMSG(LOG_WARNING, "%s", streaminterfacelog.text);
	streaminterfacelog.streaminterface_id = streaminterfaceid;
	streaminterfacelog.level = loglevel;
	int retdbadd;
	pthread_mutex_lock(mutex_dbconn);
	retdbadd = dbaddstreaminterfacelog(&dbconn, &streaminterfacelog);
	pthread_mutex_unlock(mutex_dbconn);
	if (retdbadd)
	{
		LOGMSG(LOG_WARNING, "st%"PRIu64": Failed to add to db: %d",
			streaminterfaceid, retdbadd);
	}
	return retdbadd;
}
