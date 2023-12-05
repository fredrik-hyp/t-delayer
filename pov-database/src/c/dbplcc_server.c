#include "dbplcc_server.h"

int dbplccfetchallserverconfigs(PGconn *dbconn, int max, serverconfig *serverconfigs, int *read){
	char query[512];
	PGresult *res;
	unsigned int numtuples; 
	int i;
	
	sprintf(query, "SELECT id, listen_address, listen_port, cert_ca, cert_ca_key  FROM plcc_server_configs");

#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n", query);
#endif

	if (PQstatus(dbconn) == CONNECTION_BAD)
	{
		return 1;
	}

	res = PQexec(dbconn,query);
	
	if(PQresultStatus(res) != PGRES_TUPLES_OK){
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr,"Could not fetch server config\n");
#endif
		PQclear(res);
		return 2;
	}
	numtuples = PQntuples(res);
	
	if (numtuples < 1)
	{
		PQclear(res);
		*read = 0;
		return 0;
	}

	if (numtuples > max)
	{
		syslog(LOG_WARNING, "Found more configs than the system can handle (max: %d, found: %d)", max, numtuples);
		syslog(LOG_WARNING, "Will only copy the first %d configs", max);
	}
	
	for (i=0; i < numtuples && i < max; i++)
	{
		serverconfigs[i].id = atoll(PQgetvalue(res,i,0));
		strncpy(serverconfigs[i].listen_address , PQgetvalue(res,i,1), PLCC_LISTEN_ADDRESS_LENGTH);
		serverconfigs[i].listen_port =  atoi(PQgetvalue(res,i,2));
		strncpy(serverconfigs[i].cert_ca, PQgetvalue(res,i,3), PLCC_CERT_FILENAME_LENGTH-1);		
		strncpy(serverconfigs[i].cert_ca_key, PQgetvalue(res,i,4), PLCC_CERT_FILENAME_LENGTH-1);
	}
	*read = i;

	PQclear(res);
	return 0;
}
