#include "dbcertificates.h"

#define SMALL_QUERY_LEN		128
#define MEDIUM_QUERY_LEN	512
#define BIG_QUERY_LEN		1024
#define DOUBLE_BIG_QUERY_LEN	2048

int dbfetchallrootcertificates(PGconn **conn, dbrootcertificate ***rootcertificates, int *numcertificates_copied)
{
	char query[SMALL_QUERY_LEN];
	PGresult *res;
    dbrootcertificate *rootcertificate=NULL;
	unsigned int numtuples; 
	int i;

	if (conn == NULL || numcertificates_copied == NULL)
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
		"SELECT id, name FROM rootcertificates ORDER BY id");

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
		fprintf(stderr, "%s: Could not fetch rootcertificates\n", __FUNCTION__);
#endif
		PQclear(res);
		return 2;
	}
	numtuples = PQntuples(res);
	
	if (numtuples < 1)
	{
		PQclear(res);
		*numcertificates_copied = 0;
		return 0;
	}


    *rootcertificates = calloc(numtuples, sizeof(dbrootcertificate));
	
    for(i=0;i<numtuples;i++){
        rootcertificate = calloc(1, sizeof(dbrootcertificate));
        rootcertificate->id = atoll(PQgetvalue(res,i,0));
	    strncpy(rootcertificate->name, PQgetvalue(res,i,1), 1023);
        (*rootcertificates)[i] = rootcertificate;
    }	
	*numcertificates_copied = i;

	PQclear(res);
	return 0;
}

int dbfetchrootcertificatebyorganisationid(PGconn **conn, uint64_t organisationid, dbrootcertificate *rootcertificate)
{
	char query[SMALL_QUERY_LEN];
	PGresult *res;
	unsigned int numtuples; 
	int i;

	if (conn == NULL || organisationid == 0)
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
		"SELECT id, name FROM rootcertificates WHERE organisation_id=%" PRIu64, organisationid);

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
		fprintf(stderr, "%s: Could not fetch rootcertificate.\n", __FUNCTION__);
#endif
		PQclear(res);
		return 2;
	}
	numtuples = PQntuples(res);
	
	if (numtuples < 1)
	{
		PQclear(res);
		return 0;
	}


	
    rootcertificate->id = atoll(PQgetvalue(res,i,0));
	strncpy(rootcertificate->name, PQgetvalue(res,i,1), 1023);

	PQclear(res);
	return 0;
}

int dbfetchclientcertificatesbyrootcertificateid(PGconn **conn, uint64_t rootcertificateid, dbclientcertificate ***clientcertificates, int *numcertificates_copied)
{
	char query[SMALL_QUERY_LEN];
	PGresult *res;
    dbclientcertificate *clientcertificate=NULL;
	unsigned int numtuples; 
	int i;

	if (conn == NULL || numcertificates_copied == NULL || rootcertificateid == 0)
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
		"SELECT id, name FROM clientcertificates WHERE rootcertificate_id=%" PRIu64 " ORDER BY id", rootcertificateid);

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
		fprintf(stderr, "%s: Could not fetch rootcertificates\n", __FUNCTION__);
#endif
		PQclear(res);
		return 2;
	}
	numtuples = PQntuples(res);
	
	if (numtuples < 1)
	{
		PQclear(res);
		*numcertificates_copied = 0;
		return 0;
	}


    *clientcertificates = calloc(numtuples, sizeof(dbclientcertificate));
	
    for(i=0;i<numtuples;i++){
        clientcertificate = calloc(1, sizeof(dbclientcertificate));
        clientcertificate->id = atoll(PQgetvalue(res,i,0));
	    strncpy(clientcertificate->name, PQgetvalue(res,i,1), 1023);
        (*clientcertificates)[i] = clientcertificate;
    }	
	*numcertificates_copied = i;

	PQclear(res);
	return 0;
}
