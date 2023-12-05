#include "dbplccs.h"

int dbfetchallplccs(PGconn *conn, int no_of_plccs, plcc *plccs, int *numplccs_copied){
	char query[512];
	PGresult *res;
	unsigned int numtuples; 
	int i;
	
	sprintf(query, "SELECT id,connect_server, connect_port, cert_ca, cert_client, cert_client_key FROM plcc_configs");

#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n", query);
#endif

	if (PQstatus(conn) == CONNECTION_BAD)
	{
		return 1;
	}

	res = PQexec(conn,query);
	
	if(PQresultStatus(res) != PGRES_TUPLES_OK){
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr,"Could not fetch plccs\n");
#endif
		PQclear(res);
		return 2;
	}
	numtuples = PQntuples(res);
	
	if (numtuples < 1)
	{
		PQclear(res);
		*numplccs_copied = 0;
		return 0;
	}

	if (numtuples > no_of_plccs)
	{
		syslog(LOG_WARNING, "Found more interface than the system can handle (max: %d, found: %d)", no_of_plccs, numtuples);
		syslog(LOG_WARNING, "Will only copy the first %d interfaces", no_of_plccs);
	}
	
	for (i=0; i < numtuples && i < no_of_plccs; i++)
	{
		plccs[i].id = atoll(PQgetvalue(res,i,0));
		strncpy(plccs[i].connect_server , PQgetvalue(res,i,1), PLCC_CONNECT_SERVER_LENGTH);
		plccs[i].connect_port =  atoi(PQgetvalue(res,i,2));
		strncpy(plccs[i].cert_ca, PQgetvalue(res,i,3), PLCC_CERT_FILENAME_LENGTH-1);		
		strncpy(plccs[i].cert_client, PQgetvalue(res,i,4), PLCC_CERT_FILENAME_LENGTH-1);
		strncpy(plccs[i].cert_client_key, PQgetvalue(res,i,5), PLCC_CERT_FILENAME_LENGTH-1);		
	}
	*numplccs_copied = i;

	PQclear(res);
	return 0;
}

int dbupdateplcc(PGconn *conn, plcc *plcchan){
	char query[1024];
	char *connect_server;
	char *cert_ca;
	char *cert_client;
	char *cert_client_key;
	
	PGresult *res;
	
	connect_server = pqescape(plcchan->connect_server);
	cert_ca = pqescape(plcchan->cert_ca);
	cert_client = pqescape(plcchan->cert_client);
	cert_client_key = pqescape(plcchan->cert_client_key);
	
	sprintf(query, "UPDATE plccs SET timestamp=now(), connect_server='%s', connect_port='%d', cert_ca=%s, cert_client=%s, cert_client_key=%s WHERE id=%" PRIu64 , connect_server, plcchan->connect_port, cert_ca, cert_client, cert_client_key, plcchan->id);

#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n", query);
#endif
	if (PQstatus(conn) == CONNECTION_BAD)
	{
		return 1;
	}

	res = PQexec(conn,query);
	
	if(PQresultStatus(res) != PGRES_COMMAND_OK){
		if(PQresultStatus(res) != PGRES_BAD_RESPONSE || PQresultStatus(res)  != PGRES_FATAL_ERROR){
#ifdef NODAEMON_DEBUG_DB
			fprintf(stderr, "PQresultStatus(res) != PGRES_BAD_RESPONSE %d\n", (PQresultStatus(res) != PGRES_BAD_RESPONSE));
			fprintf(stderr, "PQresultStatus(res) != PGRES_FATAL_ERROR %d\n", (PQresultStatus(res)  != PGRES_FATAL_ERROR));	
#endif
		}
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "Answer not ok\n");
#endif
		free(connect_server);
		free(cert_ca);
		free(cert_client);
		free(cert_client_key);
		PQclear(res);
		return 2;
	}
	
	if(atoi(PQcmdTuples(res)) < 1){
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "Unable to update PLLC with id %"PRIu64"\n", plcchan->id);
#endif
		free(connect_server);
		free(cert_ca);
		free(cert_client);
		free(cert_client_key);
		PQclear(res);
		return 3;
	}
	
	free(connect_server);
	free(cert_ca);
	free(cert_client);
	free(cert_client_key);
	PQclear(res);
	return 0;
}
