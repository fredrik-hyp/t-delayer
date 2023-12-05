#include "dborganisations.h"

#define SMALL_QUERY_LEN		128
#define MEDIUM_QUERY_LEN	512
#define BIG_QUERY_LEN		1024
#define DOUBLE_BIG_QUERY_LEN	2048

int dbfetchallorganisations(PGconn **conn, dborganisation ***organisations, int *numorgs_copied)
{
	char query[SMALL_QUERY_LEN];
	PGresult *res;
    dborganisation *organisation=NULL;
	unsigned int numtuples; 
	int i;

	if (conn == NULL || numorgs_copied == NULL)
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
		"SELECT id, name FROM organisations ORDER BY id");

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
		fprintf(stderr, "%s: Could not fetch organisations\n", __FUNCTION__);
#endif
		PQclear(res);
		return 2;
	}
	numtuples = PQntuples(res);
	
	if (numtuples < 1)
	{
		PQclear(res);
		*numorgs_copied = 0;
		return 0;
	}


    *organisations = calloc(numtuples, sizeof(dborganisation));
	
    for(i=0;i<numtuples;i++){
        organisation = calloc(1, sizeof(dborganisation));
        organisation->id = atoll(PQgetvalue(res,i,0));
	    strncpy(organisation->name, PQgetvalue(res,i,1), 1023);
        (*organisations)[i] = organisation;
    }	
	*numorgs_copied = i;

	PQclear(res);
	return 0;
}
