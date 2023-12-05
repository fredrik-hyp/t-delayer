#include "dbcommon.h"

PGconn *DBConnect()
{
	if (DBIsAvailable() == 0)
	{
		syslog(LOG_NOTICE, "DBConnect: DB is not available - doesnt answer on PQping");
		return NULL;
	}

	PGconn *dbconn;

#ifndef DATABASENAME
	dbconn = PQconnectdb("dbname=delayerdb host=localhost user=postgres");
#else
	char connstring[2000];
	sprintf(connstring, "dbname=%s host=localhost user=postgres", DATABASENAME);
	dbconn = PQconnectdb(connstring);
#endif
	if (PQstatus(dbconn) == CONNECTION_BAD)
	{
		PQfinish(dbconn);
		return NULL;
	}
	return dbconn;
}

void DBClose(PGconn *dbconn)
{
	PQfinish(dbconn);
}

int check_DB_connection_and_reconnect(PGconn **dbconn)
{
	if (dbconn == NULL)
	{
		syslog(LOG_WARNING, "check_DB_connection_and_reconnect: argument is NULL");
		return 1;
	}
	if (*dbconn == NULL)
	{
		syslog(LOG_NOTICE, "check_DB_connection_and_reconnect: connection is null - do a connect");
		*dbconn = DBConnect();
		if (*dbconn == NULL)
		{
			return 1;
		}
	}
	if (PQstatus(*dbconn) == CONNECTION_BAD)
	{
		syslog(LOG_NOTICE, "check_DB_connection_and_reconnect: connection is bad - do a reconnect");
		DBClose(*dbconn);
		*dbconn = DBConnect();
		if (*dbconn == NULL)
		{
			return 1;
		}
	}
	return 0;
}

int DBIsAvailable()
{
	PGPing pingresult;
	pingresult = PQping("dbname=delayerdb host=localhost user=postgres");
	
	switch(pingresult)
	{
		case PQPING_OK:
			return 1;
			break;
		case PQPING_REJECT:
			return 0;
			break;
		case PQPING_NO_RESPONSE:
			return 0;
			break;
		case PQPING_NO_ATTEMPT:
			return 0;
			break;
		default:
			return 0;
	}
	
	return 0;
}

char *pqescape (char *input)
{
	int len;
	char *output;

	if (input == NULL)
	{
		return(NULL);
	}

	len = strlen(input);
	if (len == 1)
	{
		return(NULL);
	}

	output = malloc((len * 2) + 1);
	if (!output)
	{
		return(NULL);
	}

	PQescapeString(output, input, len);
	return(output);
}

int DBCount(PGconn **dbconn, int *result, char *table)
{
	char query[MEDIUM_QUERY_LEN];
	PGresult *res;
	int numtuples, len;
	
	if (dbconn == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!", __FUNCTION__);
		return 1;
	}

	if (check_DB_connection_and_reconnect(dbconn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection.",
			__FUNCTION__);
		return 2;
	}
	
	if(strlen(table) < 1)
	{
		return 3;
	}

	len = snprintf(query, MEDIUM_QUERY_LEN, "SELECT COUNT(*) FROM %s", table);
	
	if(len == MEDIUM_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is too small(%d/%d)!",
			 __FUNCTION__, len, MEDIUM_QUERY_LEN);		
	}
	
#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n", query);
#endif

	res = PQexec(*dbconn, query);

	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "%s: Answer not ok\n", __FUNCTION__);
#endif
		PQclear(res);
		return 4;
	}
	
	numtuples = PQntuples(res);

	// Check number of rows returned 
	if(numtuples == 0)
	{
		PQclear(res);
		return 5;
	}
	else if (numtuples != 1)
	{
		PQclear(res);
		return 6;
	}
	
	*result = atoll(PQgetvalue(res, 0, 0));
	
	return 0;
}
