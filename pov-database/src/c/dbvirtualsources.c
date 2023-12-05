#include "dbvirtualsources.h"

#define SMALL_QUERY_LEN		128
#define MEDIUM_QUERY_LEN	512
#define BIG_QUERY_LEN		1024
#define DOUBLE_BIG_QUERY_LEN	2048

int dbaddvirtualsourcegroup(PGconn **conn, dbvirtualsourcegroup *vsourcegroup)
{
    char query[4096];
    char virtualsourcegroupid[64];
    char *name;
    
    uint64_t vsourcegroupid;
    PGresult *res;
    
	if (conn == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!", __FUNCTION__);
		return 1;
	}
	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection", __FUNCTION__);
		return 1;
	}
    
    name = pqescape(vsourcegroup->name);
    
    if(vsourcegroup->vsourcegroupid==0)
    {
        strcpy(virtualsourcegroupid, "NULL");
    }
    else
    {
        sprintf(virtualsourcegroupid, "%" PRIu64, vsourcegroup->vsourcegroupid);
    }

    sprintf(query, "INSERT INTO virtualsourcegroups (virtualsourcegroup_id, name) VALUES (%s, '%s')", virtualsourcegroupid, name);

#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n",query);
#endif
        
    res = PQexec(*conn,query);

	if(PQresultStatus(res) != PGRES_COMMAND_OK)
    {
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "Answer not ok %d\n", __LINE__);
#endif
		free(name);
		PQclear(res);
		return 1;
	}
    else
    {
    	free(name);
    	PQclear(res);    

        sprintf(query, "select currval('virtualsourcegroups_id_seq')");
        res = PQexec(*conn, query);
        if(PQresultStatus(res) != PGRES_TUPLES_OK)
        {
            syslog(LOG_WARNING, "Could not fetch currval for svirtualsourcegroups_id_seq. (%s)", PQerrorMessage(*conn) );
            PQclear(res);
            return 3;
        }
        
        vsourcegroup->id = atoll(PQgetvalue(res,0,0));
    	PQclear(res);
    }
    
    return 0;
}

int dbfetchvirtualsourcegroupbyid(PGconn **conn, dbvirtualsourcegroup *vsourcegroup, uint64_t id)
{
	char query[1024];
	PGresult *res;
	
	
	sprintf(query, "SELECT id, virtualsourcegroup_id, name FROM virtualsourcegroups WHERE id=%" PRIu64, id);
#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n",query);
#endif

	res = PQexec(*conn, query);
	if(PQresultStatus(res) != PGRES_TUPLES_OK){
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "Answer not ok %d\n", __LINE__);
#endif
		PQclear(res);
		return 1;
	}
	
	if(PQntuples(res) < 1){
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "no data %d\n",PQntuples(res));
#endif
		PQclear(res);
	
		return 2;
	}
	
	
	vsourcegroup->id = atoll(PQgetvalue(res,0,0));
    if(PQgetisnull(res, 0, 1)){
        vsourcegroup->vsourcegroupid = 0;
    }
    else{
        vsourcegroup->vsourcegroupid = atoll(PQgetvalue(res,0,1));
        
    }
	strncpy(vsourcegroup->name, PQgetvalue(res,0,2), 255);
	PQclear(res);
#ifdef NODAEMON_DEBUG_DB
	fprintf(stderr, "found source\n");
#endif
	return 0;    
}

int dbdeletevirtualsourcegroup(PGconn **conn, uint64_t id)
{
	PGresult *res;
	char query[1024];
	
	if (conn == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!", __FUNCTION__);
		return 1;
	}
	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection", __FUNCTION__);
		return 1;
	}
    
	sprintf(query, "DELETE FROM virtualsourcegroups WHERE id=%" PRIu64, id);
	res = PQexec(*conn, query);
//	if(PQresultStatus(res) != PGRES_TUPLES_OK){
   	if(atoi(PQcmdTuples(res)) != 1)
    {
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "Answer not ok %d\n", __LINE__);
#endif
		PQclear(res);
		return -1;
	}	
	
    PQclear(res);
	return 0;   
}

int dbfetchallvirtualsourcegroups(PGconn **conn, dbvirtualsourcegroup ***vsourcegroups, int *numfetched, int sortbyid)
{
    char query[512];
    PGresult *res;
    int i, n;
    
    *numfetched = 0;
	if (conn == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!", __FUNCTION__);
		return 1;
	}
	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection", __FUNCTION__);
		return 1;
	}
    
    if(sortbyid)
    {
        sprintf(query, "SELECT id, virtualsourcegroup_id, name FROM virtualsourcegroups ORDER BY id");        
    }
    else
    {
        sprintf(query, "SELECT id, virtualsourcegroup_id, name port FROM virtualsourcegroups");        
    }
    
    dbvirtualsourcegroup *dbvsourcegroup;
    
	res = PQexec(*conn, query);
	if(PQresultStatus(res) != PGRES_TUPLES_OK)
    {
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "Answer not ok %d\n", __LINE__);
#endif
		PQclear(res);
		return -1;
	}

    n = PQntuples(res);
	
	if(n < 1)
    {
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "no data %d\n",n);
#endif
		PQclear(res);
	
		return 0;
	}
	
    *vsourcegroups = calloc(n, sizeof(dbvirtualsourcegroup));
	
    for(i=0;i<n;i++)
    {
        dbvsourcegroup = calloc(1, sizeof(dbvirtualsourcegroup));
        dbvsourcegroup->id = atoll(PQgetvalue(res,i,0));
        dbvsourcegroup->vsourcegroupid = atoll(PQgetvalue(res,i,1));	
	    strncpy(dbvsourcegroup->name, PQgetvalue(res,i,2), 255);
        (*vsourcegroups)[i] = dbvsourcegroup;
    }
    *numfetched = n;
	PQclear(res);

#ifdef NODAEMON_DEBUG_DB
	fprintf(stderr, "found sources\n");
#endif
	return 0;    
}

// Will add or update a virtualsoruce
int dbaddorupdatevirtualsource(PGconn **conn, dbvirtualsource *vsource)
{
	char query[4096];
	char *name;
	char *httpurl;
	char *address;
	PGresult *res;
	
	if (conn == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!", __FUNCTION__);
		return 1;
	}
	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection", __FUNCTION__);
		return 1;
	}
    
	name = pqescape(vsource->name);
	httpurl = pqescape(vsource->httpurl);
	address = pqescape(vsource->address);
    if(vsource->vsourcegroupid != 0)
    {
	    sprintf(query, "UPDATE virtualsources SET revision=%"PRIu64", name='%s', httpurl='%s', address='%s', port=%d, virtualsourcegroup_id=%"PRIu64" WHERE id=%" PRIu64,
		     vsource->revision, name, httpurl, address, vsource->port, vsource->vsourcegroupid, vsource->id);

    }
    else
    {
	    sprintf(query, "UPDATE virtualsources SET revision=%"PRIu64", name='%s', httpurl='%s', address='%s', port=%d, virtualsourcegroup_id=null WHERE id=%" PRIu64,
		     vsource->revision, name, httpurl, address, vsource->port, vsource->id);        
    }
#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n",query);
#endif
	res = PQexec(*conn, query);
	
	if(PQresultStatus(res) != PGRES_COMMAND_OK){
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "Answer not ok %d\n", __LINE__);
#endif
		free(name);
		free(httpurl);
		free(address);
		PQclear(res);
		return 1;
	}
	
	if(atoi(PQcmdTuples(res)) < 1){
        PQclear(res);
        if(vsource->vsourcegroupid != 0)
        {        
		    sprintf(query, "INSERT INTO virtualsources (revision, name, httpurl, address, port, virtualsourcegroup_id)  VALUES (%"PRIu64", '%s', '%s','%s', %d, %"PRIu64")", vsource->revision, name, httpurl, address, vsource->port, vsource->vsourcegroupid);
        }
        else
        {
		    sprintf(query, "INSERT INTO virtualsources (revision, name, httpurl, address, port)  VALUES (%"PRIu64", '%s', '%s','%s', %d)", vsource->revision, name, httpurl, address, vsource->port);            
        }
#ifdef NODAEMON_DEBUG_DB_QUERY
		fprintf(stderr, "%s\n",query);
#endif
		res = PQexec(*conn,query);
		if(PQresultStatus(res) != PGRES_COMMAND_OK){
			free(name);
			free(httpurl);
			free(address);
#ifdef NODAEMON_DEBUG_DB
			fprintf(stderr, "Answer not ok %d\n",__LINE__);
#endif
			PQclear(res);
			return 2;
		}
	}
	free(name);
	free(httpurl);
	free(address);
	PQclear(res);
	return 0;
}


int dbfetchvirtualsourcebyid(PGconn **conn, dbvirtualsource *vsource,  uint64_t id){
	char query[MEDIUM_QUERY_LEN];
	PGresult *res;
	
	if (conn == NULL || vsource == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!", __FUNCTION__);
		return 1;
	}
	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection", __FUNCTION__);
		return 1;
	}
	
	int len = snprintf(query, MEDIUM_QUERY_LEN, "SELECT id, revision, name, httpurl, address, port FROM virtualsources WHERE id=%" PRIu64, id);

	if (len == MEDIUM_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is to small(%d/%d)!",
			__FUNCTION__, len, MEDIUM_QUERY_LEN);
	}    

#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n",query);
#endif

	res = PQexec(*conn, query);
	if(PQresultStatus(res) != PGRES_TUPLES_OK){
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "Answer not ok %d\n", __LINE__);
#endif
		PQclear(res);
		return 1;
	}
	
	if(PQntuples(res) < 1){
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "no data %d\n",PQntuples(res));
#endif
		PQclear(res);
	
		return 2;
	}
	
	
	vsource->id = atoll(PQgetvalue(res,0,0));
	vsource->revision = atoll(PQgetvalue(res,0,1));	
	strncpy(vsource->name, PQgetvalue(res,0,2), 255);
	strncpy(vsource->httpurl, PQgetvalue(res,0,3), 1023);
	strncpy(vsource->address, PQgetvalue(res,0,4), 255);
	vsource->port = atoi(PQgetvalue(res,0,5));
	PQclear(res);
#ifdef NODAEMON_DEBUG_DB
	fprintf(stderr, "found source\n");
#endif
	return 0;
}
// Fetch a virtualsource by name

int dbfetchvirtualsourcebyname(PGconn **conn, dbvirtualsource *vsource, char *name){
	char *__name;
	char query[1024];
	PGresult *res;
	
    
	if (conn == NULL || name == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!", __FUNCTION__);
		return 1;
	}
	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection", __FUNCTION__);
		return 1;
	}
    
	__name = pqescape(name);
	
	sprintf(query, "SELECT id, revision, name, httpurl, address, port FROM virtualsources WHERE name='%s'",__name);
#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n",query);
#endif

	res = PQexec(*conn, query);
	if(PQresultStatus(res) != PGRES_TUPLES_OK){
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "Answer not ok %d\n", __LINE__);
#endif
		free(__name);
		PQclear(res);
		return -1;
	}
	
	if(PQntuples(res) < 1){
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "no data %d\n",PQntuples(res));
#endif
		PQclear(res);
	
		free(__name);
		return 0;
	}
	
	free(__name);
	
	vsource->id = atoll(PQgetvalue(res,0,0));
	vsource->revision = atoll(PQgetvalue(res,0,1));	
	strncpy(vsource->name, PQgetvalue(res,0,2), 255);
	strncpy(vsource->httpurl, PQgetvalue(res,0,3), 1023);
	strncpy(vsource->address, PQgetvalue(res,0,4), 255);
	vsource->port = atoi(PQgetvalue(res,0,5));
	PQclear(res);

#ifdef NODAEMON_DEBUG_DB
	fprintf(stderr, "found source\n");
#endif
	return 1;
}

int dbdeletevirtualsourcebyid(PGconn **conn, uint64_t id){
	PGresult *res;
	char query[1024];
	
	if (conn == NULL || id==0)
	{
		syslog(LOG_CRIT, "%s argument is NULL!", __FUNCTION__);
		return 1;
	}
	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection", __FUNCTION__);
		return 1;
	}
    
	sprintf(query, "DELETE FROM virtualsources WHERE id=%" PRIu64, id);
	res = PQexec(*conn, query);
//	if(PQresultStatus(res) != PGRES_TUPLES_OK){
   	if(atoi(PQcmdTuples(res)) != 1){
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "Answer not ok %d\n", __LINE__);
#endif
		PQclear(res);
		return -1;
	}	
	
    PQclear(res);
	return 0;
}

int dbfetchvirtualsourcebyhttpurl(PGconn **conn, dbvirtualsource *vsource, char *httpurl){
	char *__httpurl;
	char query[1024];
	PGresult *res;
	
	if (conn == NULL || vsource == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!", __FUNCTION__);
		return 1;
	}
	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection", __FUNCTION__);
		return 1;
	}
    
	__httpurl = pqescape(httpurl);

	int len = snprintf(query, MEDIUM_QUERY_LEN,
			"SELECT id, revision, name, httpurl, address, port FROM virtualsources WHERE httpurl='%s'",__httpurl);

	if (len == MEDIUM_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is to small(%d/%d)!",
			__FUNCTION__, len, MEDIUM_QUERY_LEN);
	}
    	
#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n",query);
#endif
	free(__httpurl);
    __httpurl = NULL;
    
	res = PQexec(*conn, query);
	if(PQresultStatus(res) != PGRES_TUPLES_OK){
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "Answer not ok %d\n", __LINE__);
#endif
		PQclear(res);
		return -1;
	}
	
	if(PQntuples(res) < 1){
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "no data %d\n",PQntuples(res));
#endif
		PQclear(res);
	
		return -2;
	}
	
	
	vsource->id = atoll(PQgetvalue(res,0,0));
	vsource->revision = atoll(PQgetvalue(res,0,1));	
	strncpy(vsource->name, PQgetvalue(res,0,2), 255);
	strncpy(vsource->httpurl, PQgetvalue(res,0,3), 1023);
	strncpy(vsource->address, PQgetvalue(res,0,4), 255);
	vsource->port = atoi(PQgetvalue(res,0,5));
	PQclear(res);

#ifdef NODAEMON_DEBUG_DB
	fprintf(stderr, "found source\n");
#endif
	return 0;
}

// returns number of rows, caller must deallocate memory
int dbfetchallvirtualsources(PGconn **conn, dbvirtualsource ***vsources, int sortbyid, int *numrows){
    char query[512];
    PGresult *res;
    int i, n;

	if (conn == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!", __FUNCTION__);
		return 1;
	}
	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection", __FUNCTION__);
		return 1;
	}
        
    if(sortbyid){
        sprintf(query, "SELECT id, revision, name, httpurl, address, port FROM virtualsources ORDER BY id");        
    }
    else{
        sprintf(query, "SELECT id, revision, name, httpurl, address, port FROM virtualsources");        
    }
    
    dbvirtualsource *vsource;
    
	res = PQexec(*conn, query);
	if(PQresultStatus(res) != PGRES_TUPLES_OK){
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "Answer not ok %d\n", __LINE__);
#endif
		PQclear(res);
		return 1;
	}

    n = PQntuples(res);
	 *numrows = n;
	if(n < 1){
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "no data %d\n",n);
#endif
		PQclear(res);
	
		return 0;
	}
	
    *vsources = calloc(n, sizeof(vsource));
	
    for(i=0;i<n;i++){
        vsource = calloc(1, sizeof(dbvirtualsource));
        vsource->id = atoll(PQgetvalue(res,i,0));
        vsource->revision = atoi(PQgetvalue(res,i,1));	
	    strncpy(vsource->name, PQgetvalue(res,i,2), 255);
        strncpy(vsource->httpurl, PQgetvalue(res,i,3), 1023);
        strncpy(vsource->address, PQgetvalue(res,i,4), 255);
	    vsource->port = atoi(PQgetvalue(res,i,5));
        (*vsources)[i] = vsource;
    }
    
	PQclear(res);

#ifdef NODAEMON_DEBUG_DB
	fprintf(stderr, "found sources\n");
#endif

	return 0;
    
}
