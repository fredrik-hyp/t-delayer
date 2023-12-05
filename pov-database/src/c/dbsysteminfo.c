#include "dbsysteminfo.h"

#define SMALL_QUERY_LEN		128

int dbfetch_video_path(PGconn **conn, char *video_path)
{
	char query[SMALL_QUERY_LEN];
	PGresult *res;
	unsigned int numtuples; 

	if (video_path == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!", __FUNCTION__);
		return 1;
	}
	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection", __FUNCTION__);
		return 1;
	}
	
	int len = snprintf(query, SMALL_QUERY_LEN,
		"SELECT video_path FROM systeminfo");

	if (len == SMALL_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is to small (%d/%d)!", __FUNCTION__, len, SMALL_QUERY_LEN);
	}
#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n", query);
#endif

	res = PQexec(*conn, query);
	
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "%s: Could not fetch video path\n", __FUNCTION__);
#endif
		PQclear(res);
		return 2;
	}
	numtuples = PQntuples(res);
	
	if (numtuples < 1)
	{
		PQclear(res);
		return 3;
	}

	strncpy(video_path, PQgetvalue(res,0,0), VIDEO_PATH_LEN-1);
	PQclear(res);
	return 0;
}
