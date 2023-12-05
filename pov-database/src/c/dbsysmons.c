#include "dbsysmons.h"

int dbupdatesysmon(PGconn **conn,dbsysmon *sysmon)
{
	if (conn == NULL || sysmon == NULL)
	{
		syslog(LOG_WARNING, "%s argument is NULL", __FUNCTION__);
		return 1;
	}

	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection", __FUNCTION__);
		return 1;
	}

	char query[BIG_QUERY_LEN];
	PGresult *res;

	snprintf(query, BIG_QUERY_LEN,
	"UPDATE sysmons " \
	" SET timestamp=now(), "\
	" nrofcpus=%d,"\
	" cpu1min=%f, "\
	" cpu5min=%f, "\
	" cpu15min=%f, "\
	" sda0realloc=%d, "\
	" sda1realloc=%d, "\
	" sda2realloc=%d, "\
	" sda3realloc=%d, "\
	" sda4realloc=%d, "\
	" sda5realloc=%d, "\
	" sda6realloc=%d, "\
	" sda7realloc=%d, "\
	" sda8realloc=%d, "\
	" sda9realloc=%d ",
	sysmon->nrofcpus, sysmon->cpu1min, sysmon->cpu5min, sysmon->cpu15min, sysmon->reallocsectors[0], sysmon->reallocsectors[1], sysmon->reallocsectors[2], sysmon->reallocsectors[3], sysmon->reallocsectors[4], sysmon->reallocsectors[5], sysmon->reallocsectors[6], sysmon->reallocsectors[7], sysmon->reallocsectors[8], sysmon->reallocsectors[9]);

	res = PQexec(*conn, query);

	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		if (PQresultStatus(res) != PGRES_BAD_RESPONSE ||
			PQresultStatus(res) != PGRES_FATAL_ERROR)
		{
			fprintf(stderr, "PQresultStatus(res) != PGRES_BAD_RESPONSE %d\n",
					(PQresultStatus(res) != PGRES_BAD_RESPONSE));
			fprintf(stderr, "PQresultStatus(res) != PGRES_FATAL_ERROR %d\n",
					(PQresultStatus(res) != PGRES_FATAL_ERROR));	
		}
		fprintf(stderr, "%s: Answer not ok\n", __FUNCTION__);
#endif
		PQclear(res);
		DBClose(*conn);
		// Assume link is down and restablish, fix to better handling
		*conn = DBConnect();
		syslog(LOG_WARNING, "%s: Cant write sysmon update", __FUNCTION__);
		return 2;
	}

	if (atoi(PQcmdTuples(res)) < 1)
	{
		PQclear(res);
		snprintf(query, BIG_QUERY_LEN,
			"INSERT INTO sysmons (nrofcpus, cpu1min, cpu5min, cpu15min) VALUES (%d, %f,%f,%f)",
			sysmon->nrofcpus, sysmon->cpu1min, sysmon->cpu5min, sysmon->cpu15min);
			
#ifdef NODAEMON_DEBUG_DB_QUERY
		fprintf(stderr, "%s\n", query);
#endif
		
		res = PQexec(*conn, query);
		if (PQresultStatus(res) != PGRES_COMMAND_OK)
		{
			PQclear(res);
#ifdef NODAEMON_DEBUG_DB
			fprintf(stderr, "%s: Answer not ok\n", __FUNCTION__);
#endif
			return 3;
		}
	}
	
	PQclear(res);
	return 0;
}
