#include "dbstreaminterfaces.h"
extern PGconn *dbconn;

int copytostreaminterfaceconfig(PGresult *res, s_interfaceconfig *streaminterfaceconfig, uint16_t maxconfigs)
{
	int i, nrofrows;
	
	nrofrows = PQntuples(res);
	if(nrofrows < 1)
	{
		return 1;
	}

	for(i=0; i < nrofrows && i < maxconfigs; i++)
	{
		// ====== ID ======
		streaminterfaceconfig[i].id = atoll(PQgetvalue(res,i,0));

		// ====== streaminterface ID ======
		streaminterfaceconfig[i].streaminterface_id = atoll(PQgetvalue(res,i,1));

		// ====== typeofconfig ======
		strncpy(streaminterfaceconfig[i].typeofconfig, PQgetvalue(res,i,2), DB_MAX_TYPEOFCONFIG);
		streaminterfaceconfig[i].typeofconfig[DB_MAX_TYPEOFCONFIG-1] = '\0';
	
		// ====== autostart ======
		if (!strcmp(PQgetvalue(res,i,3),"t"))
		{
			streaminterfaceconfig[i].autostart = 1;
		}
		else
		{
			streaminterfaceconfig[i].autostart = 0;
		}

		// ====== msbuffer ======
		streaminterfaceconfig[i].msbuffer = atoi(PQgetvalue(res, i, 4));

		// ====== resendmode ======
		memset(streaminterfaceconfig[i].resendmode, 0, DB_MAX_RESENDMODE);
		strncpy(streaminterfaceconfig[i].resendmode, PQgetvalue(res, i, 5), DB_MAX_RESENDMODE);
		streaminterfaceconfig[i].resendmode[DB_MAX_RESENDMODE-1] = '\0';

		// ====== mode ======
		memset(streaminterfaceconfig[i].mode, 0, DB_MAX_MODE_LENGTH);
		strncpy(streaminterfaceconfig[i].mode, PQgetvalue(res, i, 6), DB_MAX_MODE_LENGTH);
		streaminterfaceconfig[i].mode[DB_MAX_MODE_LENGTH-1] = '\0';

		// ====== nosessionid ======
		if (strcmp(PQgetvalue(res, i, 7), "t") == 0)
		{
			streaminterfaceconfig[i].nosessionid = 1;
		}
		else
		{
			streaminterfaceconfig[i].nosessionid = 0;
		}

		// ====== backuptoprimary ======
		if (strcmp(PQgetvalue(res, i, 8), "t") == 0)
		{
			streaminterfaceconfig[i].backuptoprimary = 1;
		}
		else
		{
			streaminterfaceconfig[i].backuptoprimary = 0;
		}

		// ====== multipath ======
		if (strcmp(PQgetvalue(res, i, 9), "t") == 0)
		{
			streaminterfaceconfig[i].multipath = 1;
		}
		else
		{
			streaminterfaceconfig[i].multipath = 0;
		}

		// ====== mploadbalance ======
		if (strcmp(PQgetvalue(res, i, 10), "t") == 0)
		{
			streaminterfaceconfig[i].mploadbalance = 1;
		}
		else
		{
			streaminterfaceconfig[i].mploadbalance = 0;
		}
		
		// ====== act as server ======
		if (strcmp(PQgetvalue(res, i, 11), "t") == 0)
		{
			streaminterfaceconfig[i].actasserver = 1;
		}
		else
		{
			streaminterfaceconfig[i].actasserver = 0;
		}
	}

	return 0;
}

int copytostreaminterface(PGresult *res, s_interface *streaminterface)
{
	streaminterface->id = atoll(PQgetvalue(res,0,0));
	strncpy(streaminterface->interfacetype, PQgetvalue(res,0,1), DB_MAX_STREAMINTERFACETYPE);
	streaminterface->interfacetype[DB_MAX_STREAMINTERFACETYPE-1] = '\0';
	
	strncpy(streaminterface->name, PQgetvalue(res,0,2), DB_MAX_NAME_LENGTH);
	streaminterface->name[DB_MAX_NAME_LENGTH-1] = '\0';

	strncpy(streaminterface->usertext, PQgetvalue(res,0,3), DB_MAX_NAME_LENGTH);
	streaminterface->usertext[DB_MAX_NAME_LENGTH-1] = '\0';		
	
	return 0;
}

int 
	dbfetchallstreaminterfaces(PGconn **conn, int no_of_sinterface, s_interface *streaminterfaces, int *numinterfaces_copied)
{
	char query[SMALL_QUERY_LEN];
	PGresult *res;
	unsigned int numtuples; 
	int i;

	if (conn == NULL || streaminterfaces == NULL || numinterfaces_copied == NULL)
	{
		syslog(LOG_CRIT, "dbfetchallstreaminterfaces argument is NULL!");
		return 1;
	}
	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "dbfetchallstreaminterfaces cant reconnect DB connection");
		return 1;
	}
	
	int len = snprintf(query, SMALL_QUERY_LEN,
		"SELECT id, interfacetype, name, usertext " \
		" FROM streaminterfaces ORDER BY id");

	if (len == SMALL_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "dbfetchallstreaminterfaces snprintf buf is to small (%d/%d)!", len, SMALL_QUERY_LEN);
	}
#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n", query);
#endif

	res = PQexec(*conn, query);
	
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "dbfetchallstreaminterfaces: Could not fetch streaminterfaces\n");
#endif
		PQclear(res);
		return 2;
	}
	numtuples = PQntuples(res);
	
	if (numtuples < 1)
	{
		PQclear(res);
		*numinterfaces_copied = 0;
		return 0;
	}

	if (numtuples > no_of_sinterface)
	{
		syslog(LOG_WARNING, "Found more interface than the system can handle (max: %d, found: %d)",
			no_of_sinterface, numtuples);
		syslog(LOG_WARNING, "Will only copy the first %d interfaces",
			no_of_sinterface);
	}
	
	for (i = 0; i < numtuples && i < no_of_sinterface; i++)
	{
		streaminterfaces[i].id = atoll(PQgetvalue(res,i,0));
		strncpy(streaminterfaces[i].interfacetype, PQgetvalue(res,i,1), DB_MAX_STREAMINTERFACETYPE);
		streaminterfaces[i].interfacetype[DB_MAX_STREAMINTERFACETYPE-1] = '\0';

		strncpy(streaminterfaces[i].name, PQgetvalue(res,i,2), DB_MAX_NAME_LENGTH);
		streaminterfaces[i].name[DB_MAX_NAME_LENGTH-1] = '\0';

		strncpy(streaminterfaces[i].usertext, PQgetvalue(res,i,3), DB_MAX_NAME_LENGTH);	
		streaminterfaces[i].usertext[DB_MAX_NAME_LENGTH-1] = '\0';
	}
	*numinterfaces_copied = i;

	PQclear(res);
	return 0;
}

int dbfetchstreaminterfacebyid(PGconn **conn, s_interface *streaminterface, uint64_t id)
{
	char query[SMALL_QUERY_LEN];
	PGresult *res;

	if (conn == NULL || streaminterface == NULL)
	{
		syslog(LOG_CRIT, "dbfetchstreaminterfacebyname argument is NULL!");
		return 1;
	}
	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "dbfetchstreaminterfacebyname cant reconnect DB connection");
		return 1;
	}


	int len = snprintf(query, SMALL_QUERY_LEN,
		"SELECT id, interfacetype, name, usertext " \
		" FROM streaminterfaces " \
		" WHERE id=%" PRIu64 , id);
	if (len == SMALL_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "dbfetchstreaminterfacebyname snprintf buf is to small(%d/%d)!", len, SMALL_QUERY_LEN);
	}
	

#ifdef NODAEMON_DEBUG
	fprintf(stderr, "%s\n", query);
#endif

	res = PQexec(*conn, query);

	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
#ifdef NODAEMON_DEBUG
		fprintf(stderr, "dbfetchstreaminterfacebyname: Answer not ok %d\n", PQresultStatus(res));
#endif
		PQclear(res);
		return 3;
	}
	
	if (PQntuples(res) != 1)
	{
		PQclear(res);
		return 4;
	}
	
	copytostreaminterface(res, streaminterface);
	PQclear(res);
	
	return 0;
}

int dbfetchstreaminterfacebyname(PGconn **conn, s_interface *streaminterface, char *name)
{
	char query[SMALL_QUERY_LEN];
	char *sname;
	PGresult *res;

	if (conn == NULL || streaminterface == NULL || name == NULL)
	{
		syslog(LOG_CRIT, "dbfetchstreaminterfacebyname argument is NULL!");
		return 1;
	}
	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "dbfetchstreaminterfacebyname cant reconnect DB connection");
		return 1;
	}

	if (strlen(name) >= DB_MAX_NAME_LENGTH)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "dbfetchstreaminterfacebyname: name is too large\n");
#endif
		return 2;
	}

	sname = pqescape(name);
	if (sname == NULL)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "dbfetchstreaminterfacebyname: Sanitation failed!\n");
#endif
		return 2;
	}

	int len = snprintf(query, SMALL_QUERY_LEN,
		"SELECT id, interfacetype, name, usertext " \
		" FROM streaminterfaces " \
		" WHERE name='%s'", sname);
	if (len == SMALL_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "dbfetchstreaminterfacebyname snprintf buf is to small(%d/%d)!", len, SMALL_QUERY_LEN);
	}
	
	free(sname);
	sname = NULL;

#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n", query);
#endif

	res = PQexec(*conn, query);

	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "dbfetchstreaminterfacebyname: Answer not ok %d\n", PQresultStatus(res));
#endif
		PQclear(res);
		return 3;
	}
	
	if (PQntuples(res) != 1)
	{
		PQclear(res);
		return 4;
	}
	
	copytostreaminterface(res, streaminterface);
	PQclear(res);
	
	return 0;
}

int dbfetchstreaminterfaceruninfobystreaminterfaceid(PGconn **conn, s_interfaceruninfo *streaminterfaceruninfo,
			uint64_t streaminterfaceid)
{
	char query[MEDIUM_QUERY_LEN];
	PGresult *res;

	if (conn == NULL || streaminterfaceruninfo == NULL)
	{
		syslog(LOG_CRIT, "dbfetchstreaminterfaceruninfobystreaminterfaceid argument is NULL!");
		return 1;
	}
	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "dbfetchstreaminterfaceruninfobystreaminterfaceid cant reconnect DB connection");
		return 1;
	}
	
	int len = snprintf(query, MEDIUM_QUERY_LEN,
		"SELECT id, streaminterface_id, " \
		" extract(epoch from lastchanged)::int as lastchanged, " \
		" pid, startstopinfo " \
		" FROM streaminterfaceruninfos " \
		" WHERE streaminterface_id=%" PRIu64 " ",
		streaminterfaceid);
	if (len == MEDIUM_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "dbfetchstreaminterfaceruninfobystreaminterfaceid snprintf buf is to small(%d/%d)!", len, MEDIUM_QUERY_LEN);	
	}
#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n", query);
#endif

	res = PQexec(*conn, query);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
//		fprintf(stderr, "dbfetchstreaminterfaceruninfobystreaminterfaceid: Answer not ok (%d)\n", __LINE__);
		PQclear(res);
		return 2;
	}

	if (PQntuples(res) != 1)
	{
		if (PQntuples(res) > 1)
		{
			syslog(LOG_WARNING, "dbfetchstreaminterfaceruninfobystreaminterfaceid returned more tuples (%d) then expected %" PRIu64,
				PQntuples(res), streaminterfaceid);
		}
		PQclear(res);
		return 3;
	}
	
	streaminterfaceruninfo->id = atoll(PQgetvalue(res,0,0));
	streaminterfaceruninfo->streaminterface_id = atoll(PQgetvalue(res,0,1));
	streaminterfaceruninfo->lastchanged = atoll(PQgetvalue(res,0,2));
	streaminterfaceruninfo->pid = atoll(PQgetvalue(res,0,3));
	strncpy(streaminterfaceruninfo->startstopinfo, PQgetvalue(res,0,4), DB_MAX_STARTSTOPINFO);
	streaminterfaceruninfo->startstopinfo[DB_MAX_STARTSTOPINFO-1] = '\0';
	
	PQclear(res);
	return 0;
}

int dbfetchstreaminterfaceconfigbyid(PGconn **conn, s_interfaceconfig *streaminterfaceconfig, uint64_t  id)
{
	char query[BIG_QUERY_LEN];
	PGresult *res;

	if (conn == NULL || streaminterfaceconfig == NULL)
	{
		syslog(LOG_CRIT, "dbfetchstreaminterfaceconfigbystreaminterfaceid argument is NULL!");
		return 1;
	}
	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "dbfetchstreaminterfaceconfigbystreaminterfaceid cant reconnect DB connection");
		return 1;
	}
	
	int len = snprintf(query, BIG_QUERY_LEN,
		"SELECT id, streaminterface_id, typeofconfig, autostart, " \
		" msbuffer, resendmode, mode, "\
		" nosessionid, backuptoprimary, multipath, mploadbalance, " \
		" actasserver "\
		" FROM streaminterfaceconfigs " \
		" WHERE streaminterface_id=%" PRIu64 , id);

	if (len == BIG_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "dbfetchstreaminterfaceconfigbystreaminterfaceid snprintf buf is to small(%d/%d)!", len, BIG_QUERY_LEN);
	}
#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n", query);
#endif

	res = PQexec(*conn, query);
	
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "dbfetchstreaminterfaceconfigbystreaminterfaceid: Answer not ok %d %d\n",
				PQresultStatus(res), PGRES_NONFATAL_ERROR);
#endif
		PQclear(res);
		return 2;
	}
	
	if (PQntuples(res) != 1)
	{
		PQclear(res);
		return 4;
	}

	copytostreaminterfaceconfig(res, streaminterfaceconfig,1);

	PQclear(res);
	return 0;									
}

int dbfetchstreaminterfaceconfigbystreaminterfaceid(PGconn **conn,
		s_interfaceconfig *streaminterfaceconfig,
		uint64_t streaminterfaceid)
{
	char query[BIG_QUERY_LEN];
	PGresult *res;

	if (conn == NULL || streaminterfaceconfig == NULL)
	{
		syslog(LOG_CRIT, "dbfetchstreaminterfaceconfigbystreaminterfaceid argument is NULL!");
		return 1;
	}
	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "dbfetchstreaminterfaceconfigbystreaminterfaceid cant reconnect DB connection");
		return 1;
	}
	
	int len = snprintf(query, BIG_QUERY_LEN,
		"SELECT id, streaminterface_id, typeofconfig, autostart, " \
		" msbuffer, resendmode, mode, "\
		" nosessionid, backuptoprimary, multipath, mploadbalance, " \
		" actasserver "\
		" FROM streaminterfaceconfigs " \
		" WHERE streaminterface_id=%" PRIu64 , streaminterfaceid);

	if (len == BIG_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "dbfetchstreaminterfaceconfigbystreaminterfaceid snprintf buf is to small(%d/%d)!", len, BIG_QUERY_LEN);
	}
#ifdef NODAEMON_DEBUG
	fprintf(stderr, "%s\n", query);
#endif

	res = PQexec(*conn, query);
	
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
#ifdef NODAEMON_DEBUG
		fprintf(stderr, "dbfetchstreaminterfaceconfigbystreaminterfaceid: Answer not ok %d %d\n",
				PQresultStatus(res), PGRES_NONFATAL_ERROR);
#endif
		PQclear(res);
		return 2;
	}
	
	if (PQntuples(res) != 1)
	{
		PQclear(res);
		return 4;
	}

	copytostreaminterfaceconfig(res, streaminterfaceconfig,1);
	PQclear(res);
	return 0;
}

int dbfetchstreaminterfaceconfigsforrecorderandplayer(PGconn **conn, s_interfaceconfig *streaminterfaceconfig, uint16_t maxconfigs)
{
	char query[BIG_QUERY_LEN];
	PGresult *res;

	if (conn == NULL || streaminterfaceconfig == NULL)
	{
		syslog(LOG_CRIT, "dbfetchstreaminterfaceconfigbystreaminterfaceid argument is NULL!");
		return 1;
	}
	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "dbfetchstreaminterfaceconfigbystreaminterfaceid cant reconnect DB connection");
		return 1;
	}
	PQstatus(*conn);
	
	int len = snprintf(query, BIG_QUERY_LEN,
		"SELECT id, streaminterface_id, typeofconfig, autostart, " \
		" msbuffer, resendmode, mode, "\
		" nosessionid, backuptoprimary, multipath, mploadbalance, " \
		" actasserver, "\
		" FROM streaminterfaceconfigs " \
		" WHERE mode='player' OR mode='recorder' LIMIT %d", maxconfigs);

	PQstatus(*conn);
	if (len == BIG_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is to small(%d/%d)!", __FUNCTION__, len, BIG_QUERY_LEN);
	}
#ifdef NODAEMON_DEBUG
	fprintf(stderr, "%s\n", query);
#endif

	res = PQexec(*conn, query);
	
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
#ifdef NODAEMON_DEBUG
		fprintf(stderr, "%s: Answer not ok %d %d\n",
				__FUNCTION__, PQresultStatus(res), PGRES_NONFATAL_ERROR);
#endif
		PQclear(res);
		return 2;
	}
	PQstatus(*conn);
	
	if (PQntuples(res) < 1)
	{
		PQclear(res);
		return 4;
	}
	PQstatus(*conn);

	copytostreaminterfaceconfig(res, streaminterfaceconfig, maxconfigs);
	PQstatus(*conn);

	PQclear(res);
	PQstatus(*conn);
	return 0;													
}
												
int dbfetchstreaminterfacesourcebystreaminterfaceconfigid(PGconn **conn, s_interfacesource *streaminterfacesource,
				uint64_t streaminterfaceconfigid)
{
	char query[MEDIUM_QUERY_LEN];
	PGresult *res;

	if (conn == NULL || streaminterfacesource == NULL)
	{
		syslog(LOG_CRIT, "dbfetchstreaminterfacesourcebystreaminterfaceconfigid argument is NULL!");
		return 1;
	}
	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "dbfetchstreaminterfacesourcebystreaminterfaceconfigid cant reconnect DB connection");
		return 1;
	}
	
	int len = snprintf(query, MEDIUM_QUERY_LEN,
		"SELECT id, streaminterfaceconfig_id, name, priority, " \
		" sourcetype, host, port, talkbackhost, talkbackport, "\
		" hostclientcertificate_id, talkbackclientcertificate_id, "\
		" maxretrytime, streammissfailovertimeout, virtualsource_id, "\
		" recording_id, addrtp "\
		" FROM streaminterfacesources "\
		" WHERE streaminterfaceconfig_id=%" PRIu64,
		streaminterfaceconfigid);
	if (len == MEDIUM_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "dbfetchstreaminterfacesourcebystreaminterfaceconfigid snprintf buf is to small(%d/%d)!",
			len, MEDIUM_QUERY_LEN);
	}

#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n", query);
#endif
	
	res = PQexec(*conn, query);
	
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "dbfetchstreaminterfacesourcebystreaminterfaceconfigid Answer not ok\n");
#endif
		PQclear(res);
		return 2;
	}
	
	if (PQntuples(res) != 1)
	{
		PQclear(res);
		return 3;
	}
	
	// ====== ID ======
	if (PQgetisnull(res, 0, 0))
	{
		streaminterfacesource->id = 0;
	}
	else
	{
		streaminterfacesource->id = atoll(PQgetvalue(res, 0, 0));
	}

	// ====== streaminterfaceconfig_id ======
	if (PQgetisnull(res, 0, 1))
	{
		streaminterfacesource->streaminterfaceconfig_id = 0;
	}
	else
	{
		streaminterfacesource->streaminterfaceconfig_id = atoll(PQgetvalue(res, 0, 1));
	}

	// ====== name ======
	strncpy(streaminterfacesource->name, PQgetvalue(res, 0, 2), DB_MAX_NAME_LENGTH);
	streaminterfacesource->name[DB_MAX_NAME_LENGTH-1] = '\0';

	// ====== priority ======
	if (PQgetisnull(res, 0, 3))
	{
		streaminterfacesource->priority = 0;
	}
	else
	{
		streaminterfacesource->priority = atoi(PQgetvalue(res, 0, 3));
	}

	// ====== sourcetype ======
	strncpy(streaminterfacesource->sourcetype, PQgetvalue(res, 0, 4), DB_MAX_INTERFACETYPE);
	streaminterfacesource->sourcetype[DB_MAX_INTERFACETYPE-1] = '\0';

	// ======= host ======
	strncpy(streaminterfacesource->host, PQgetvalue(res, 0, 5), DB_MAX_HOSTNAME_LENGTH);
	streaminterfacesource->host[DB_MAX_HOSTNAME_LENGTH-1] = '\0';

	// ====== port ======
	if (PQgetisnull(res, 0, 6))
	{
		streaminterfacesource->port = 0;
	}
	else
	{
		streaminterfacesource->port = atoi(PQgetvalue(res, 0, 6));
	}

	// ====== talkbackhost ======
	strncpy(streaminterfacesource->talkbackhost, PQgetvalue(res,0,7), DB_MAX_HOSTNAME_LENGTH);
	streaminterfacesource->talkbackhost[DB_MAX_HOSTNAME_LENGTH-1] = '\0';

	// ====== talkbackport ======
	if (PQgetisnull(res,0,8))
	{
		streaminterfacesource->talkbackport = 0;
	}
	else
	{
		streaminterfacesource->talkbackport = atoi(PQgetvalue(res, 0, 8));
	}

	// ====== hostclientcertificate_id ======
	if (PQgetisnull(res, 0, 9))
	{
		streaminterfacesource->hostclientcertificate_id = 0;
	}
	else
	{
		streaminterfacesource->hostclientcertificate_id = atoll(PQgetvalue(res, 0, 9));
	}

	// ====== talkbackclientcertificate_id ======
	if (PQgetisnull(res, 0, 10))
	{
		streaminterfacesource->talkbackclientcertificate_id = 0;
	}
	else
	{
		streaminterfacesource->talkbackclientcertificate_id = atoll(PQgetvalue(res, 0, 10));
	}

	// ====== maxretrytime ======
	if (PQgetisnull(res, 0, 11))
	{
		streaminterfacesource->maxretrytime = 0;
	}
	else
	{
		streaminterfacesource->maxretrytime = atoi(PQgetvalue(res, 0, 11));
	}

	// ====== streammissfailovertimeout ======
	if (PQgetisnull(res, 0, 12))
	{
		streaminterfacesource->streammissfailovertimeout = 0;
	}
	else
	{
		streaminterfacesource->streammissfailovertimeout = atoi(PQgetvalue(res, 0, 12));
	}

	// ====== virtualsource_id =======
	if (PQgetisnull(res, 0, 13))
	{
		streaminterfacesource->virtualsource_id = 0;
	}
	else
	{
		streaminterfacesource->virtualsource_id = atol(PQgetvalue(res,0,13));
	}

	// ====== recording_id =======
	if (PQgetisnull(res, 0, 14))
	{
		streaminterfacesource->recording_id = 0;
	}
	else
	{
		streaminterfacesource->recording_id = atol(PQgetvalue(res,0,14));
	}

	// add rtp header if raw udp input
	if (!strcmp(PQgetvalue(res,0,15),"t"))
	{
		streaminterfacesource->addrtp = 1;
	}
	else
	{
		streaminterfacesource->addrtp = 0;
	}

	PQclear(res);
	return 0;
}

int dbfetchstreaminterfacesource(PGconn **conn, s_interfacesource *streaminterfacesource, s_interfaceruninfo *streaminterfaceruninfo,int *nRows)
{
	char query[DOUBLE_BIG_QUERY_LEN];
	PGresult *res;
	int len;

	if (conn == NULL)
	{
		syslog(LOG_CRIT, "%s: argument is NULL!",
			__FUNCTION__);
		return 1;
	}
	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s: cant reconnect DB connection",
			__FUNCTION__ );
		return 1;
	}

	if (streaminterfaceruninfo)
	{
		len = snprintf(query, DOUBLE_BIG_QUERY_LEN,
			"SELECT streaminterfacesources.id, " \
			" streaminterfaceconfig_id, " \
			" name, priority, sourcetype, host, port, " \
			" talkbackhost, talkbackport, " \
			" virtualsource_id, vsourcerevision,  recording_id, " \
			" streaminterfaceruninfos.id, streaminterface_id, " \
			" extract(epoch from lastchanged)::int as lastchanged, " \
			" pid, startstopinfo " \
			" FROM streaminterfacesources, streaminterfaceruninfos " \
			" WHERE streaminterfacesources.streaminterfaceconfig_id=streaminterfaceruninfos.streaminterface_id " \
			" ORDER BY streaminterfaceconfig_id" );
	}
	else
	{
		len = snprintf(query, DOUBLE_BIG_QUERY_LEN,
			"SELECT id, streaminterfaceconfig_id, " \
			" name, priority, sourcetype, host, port, " \
			" talkbackhost, talkbackport, " \
			" virtualsource_id, vsourcerevision, recording_id " \
			" FROM streaminterfacesources " \
			" ORDER BY streaminterfaceconfig_id");
	}

	if (len == DOUBLE_BIG_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "dbfetchstreaminterfacesourcebystreaminterfaceconfigid snprintf buf is to small(%d/%d)!",
			len, DOUBLE_BIG_QUERY_LEN);
	}

#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n", query);
#endif
	
	res = PQexec(*conn, query);
	
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "Answer not ok\n");
#endif
		PQclear(res);
		return 1;
	}
	
	*nRows = PQntuples(res);
	if (*nRows < 1)
	{
		PQclear(res);
		return 2;
	}

	int i;
	for( i = 0; i < *nRows && i < MAX_STREAMINTERFACES; i++)
	{
		// ====== id =======
		if (PQgetisnull(res,i,0))
		{
			streaminterfacesource[i].id = 0;
		}
		else
		{
			streaminterfacesource[i].id = atoll(PQgetvalue(res,i,0));
		}

		// ====== streaminterfaceconfig_id =======
		if (PQgetisnull(res,i,1))
		{
			streaminterfacesource[i].streaminterfaceconfig_id = 0;
		}
		else
		{
			streaminterfacesource[i].streaminterfaceconfig_id = atoll(PQgetvalue(res,i,1));
		}

		// ====== name =======
		strncpy(streaminterfacesource[i].name,PQgetvalue(res,i,2), DB_MAX_NAME_LENGTH-1);

		// ====== priority =======
		if (PQgetisnull(res,i,3))
		{
			streaminterfacesource[i].priority = 0;
		}
		else
		{
			streaminterfacesource[i].priority = atoi(PQgetvalue(res,i,3));
		}

		// ====== sourcetype =======
		strncpy(streaminterfacesource[i].sourcetype, PQgetvalue(res,i,4),10);

		// ====== host =======
		strncpy(streaminterfacesource[i].host, PQgetvalue(res,i,5), DB_MAX_HOSTNAME_LENGTH-1);

		// ====== port =======
		if (PQgetisnull(res, i, 6))
		{
			streaminterfacesource[i].port = 0;
		}
		else
		{
			streaminterfacesource[i].port = atoi(PQgetvalue(res,i,6));
		}

		// ====== talkbackhost =======
		strncpy(streaminterfacesource[i].talkbackhost, PQgetvalue(res,i,7), DB_MAX_HOSTNAME_LENGTH-1);

		// ====== talkbackport =======
		if (PQgetisnull(res,i,8))
		{
			streaminterfacesource[i].talkbackport = 0;
		}
		else
		{
			streaminterfacesource[i].talkbackport = atoi(PQgetvalue(res,i,8));
		}

		// ====== virtualsource_id =======
		if (PQgetisnull(res,i,9))
		{
			streaminterfacesource[i].virtualsource_id = 0;
		}
		else
		{
			streaminterfacesource[i].virtualsource_id = atoll(PQgetvalue(res,i,9));
		}

		// ====== vsourcerevision =======
		if (PQgetisnull(res,i,10))
		{
			streaminterfacesource[i].vsourcerevision = 0;
		}
		else
		{
			streaminterfacesource[i].vsourcerevision = atoll(PQgetvalue(res,i,10));
		}

		// ====== recording_id =======
		if (PQgetisnull(res,i,11))
		{
			streaminterfacesource[i].recording_id = 0;
		}
		else
		{
			streaminterfacesource[i].recording_id = atoll(PQgetvalue(res,i,11));
		}

		if (streaminterfaceruninfo)
		{

			// ====== id =======
			if (PQgetisnull(res,i,12))
			{
				streaminterfaceruninfo[i].id = 0;
			}
			else
			{
				streaminterfaceruninfo[i].id = atoll(PQgetvalue(res,i,12));
			}

			// ====== streaminterface_id =======
			if (PQgetisnull(res,i,13))
			{
				streaminterfaceruninfo[i].streaminterface_id = 0;
			}
			else
			{
				streaminterfaceruninfo[i].streaminterface_id = atoll(PQgetvalue(res,i,13));
			}

			// ====== lastchanged =======
			if (PQgetisnull(res,i,14))
			{
				streaminterfaceruninfo[i].lastchanged = 0;
			}
			else
			{
				streaminterfaceruninfo[i].lastchanged = atoll(PQgetvalue(res,i,14));
			}

			// ====== pid =======
			if (PQgetisnull(res,i,15))
			{
				streaminterfaceruninfo[i].pid = 0;
			}
			else
			{
				streaminterfaceruninfo[i].pid = atoll(PQgetvalue(res,i,15));
			}

			// ====== startstopinfo =======
			strncpy(streaminterfaceruninfo[i].startstopinfo, PQgetvalue(res,i,16), 256);
		}
#if 0
		syslog(LOG_NOTICE, "%" PRIu64 " %" PRIu64 " %s %d %s %s %d %s %d %" PRIu64 " %d %"PRIu64,
			streaminterfacesource[i].id,
			streaminterfacesource[i].streaminterfaceconfig_id,
			streaminterfacesource[i].name, 
			streaminterfacesource[i].priority, 
			streaminterfacesource[i].sourcetype,
			streaminterfacesource[i].host,
			streaminterfacesource[i].port,
			streaminterfacesource[i].talkbackhost,
			streaminterfacesource[i].talkbackport,
			streaminterfacesource[i].virtualsource_id,
			streaminterfacesource[i].vsourcerevision,
			streaminterfacesource[i].recording_id);
#endif

	}

	PQclear(res);
	return 0;
	}

int dbfetch_streaminterfacebackupsource_by_streaminterfacesourceid(PGconn **conn,
				s_interfacesource *streaminterfacesource,
				unsigned short *num_backup_source,
				uint64_t streaminterfacesource_id)
{
	char query[MEDIUM_QUERY_LEN];
	PGresult *res;

	if (conn == NULL || streaminterfacesource == NULL)
	{
		syslog(LOG_CRIT, "dbfetch_streaminterfacebackupsource_by_streaminterfacesourceid argument is NULL!");
		return 1;
	}
	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "dbfetch_streaminterfacebackupsource_by_streaminterfacesourceid cant reconnect DB connection");
		return 1;
	}
	
	int len = snprintf(query, MEDIUM_QUERY_LEN,
		"SELECT id, name, priority, sourcetype, host, port, "\
		" talkbackhost, talkbackport, " \
		" hostclientcertificate_id, talkbackclientcertificate_id, " \
		" maxretrytime, streammissfailovertimeout "\
		" FROM streaminterfacebackupsources "\
		" WHERE streaminterfacesource_id=%" PRIu64
		" ORDER BY priority",
		streaminterfacesource_id);
	if (len == MEDIUM_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "dbfetch_streaminterfacebackupsource_by_streaminterfacesourceid snprintf buf is to small(%d/%d)!",
			len, MEDIUM_QUERY_LEN);
	}

#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n", query);
#endif
	
	res = PQexec(*conn, query);
	
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "dbfetch_streaminterfacebackupsource_by_streaminterfacesourceid Answer not ok\n");
#endif
		PQclear(res);
		return 2;
	}
	
	int numrows = PQntuples(res);
	if (numrows <= 0)
	{
		PQclear(res);
		return 0;
	}

	if (numrows < DB_MAX_SOURCE-1)
	{
		*num_backup_source = numrows;
	}
	else
	{
		*num_backup_source = DB_MAX_SOURCE-1;
		syslog(LOG_WARNING, "dbfetch_streaminterfacebackupsource_by_streaminterfacesourceid fetched more backup sources than the system permits");
	}
	
	int i;
	for (i = 0; i < *num_backup_source; i++)
	{

		// ====== streaminterfacesource id ======
		// Reuse the same structure in the backup scenarios
		streaminterfacesource[i].streaminterfaceconfig_id = streaminterfacesource_id;

		// ====== ID ======
		if (PQgetisnull(res, i, 0))
		{
			streaminterfacesource[i].id = 0;
		}
		else
		{
			streaminterfacesource[i].id = atoll(PQgetvalue(res, i, 0));
		}
		// ====== name ======
		strncpy(streaminterfacesource[i].name, PQgetvalue(res, i, 1), DB_MAX_NAME_LENGTH);
		streaminterfacesource[i].name[DB_MAX_NAME_LENGTH-1] = '\0';

		// ====== priority ======
		if (PQgetisnull(res,0,2))
		{
			streaminterfacesource[i].priority = 0;
		}
		else
		{
			streaminterfacesource[i].priority = atoi(PQgetvalue(res, i, 2));
		}

		// ====== sourcetype ======
		strncpy(streaminterfacesource[i].sourcetype, PQgetvalue(res, i, 3), DB_MAX_INTERFACETYPE);
		streaminterfacesource[i].sourcetype[DB_MAX_INTERFACETYPE-1] = '\0';

		// ======= host ======
		strncpy(streaminterfacesource[i].host, PQgetvalue(res, i, 4), DB_MAX_HOSTNAME_LENGTH);
		streaminterfacesource[i].host[DB_MAX_HOSTNAME_LENGTH-1] = '\0';

		// ====== port ======
		if (PQgetisnull(res, i, 5))
		{
			streaminterfacesource[i].port = 0;
		}
		else
		{
			streaminterfacesource[i].port = atoi(PQgetvalue(res, i, 5));
		}

		// ====== talkbackhost ======
		strncpy(streaminterfacesource[i].talkbackhost, PQgetvalue(res, i, 6), DB_MAX_HOSTNAME_LENGTH);
		streaminterfacesource[i].talkbackhost[DB_MAX_HOSTNAME_LENGTH-1] = '\0';

		// ====== talkbackport ======
		if (PQgetisnull(res, i, 7))
		{
			streaminterfacesource[i].talkbackport = 0;
		}
		else
		{
			streaminterfacesource[i].talkbackport = atoi(PQgetvalue(res, i, 7));
		}

		// ====== hostclientcertificate_id ======
		if (PQgetisnull(res, i, 8))
		{
			streaminterfacesource[i].hostclientcertificate_id = 0;
		}
		else
		{
			streaminterfacesource[i].hostclientcertificate_id = atoll(PQgetvalue(res, i, 8));
		}

		// ====== talkbackclientcertificate_id ======
		if (PQgetisnull(res, i, 9))
		{
			streaminterfacesource[i].talkbackclientcertificate_id = 0;
		}
		else
		{
			streaminterfacesource[i].talkbackclientcertificate_id = atoll(PQgetvalue(res, i, 9));
		}

		// ====== maxretrytime ======
		if (PQgetisnull(res, i, 10))
		{
			streaminterfacesource[i].maxretrytime = 0;
		}
		else
		{
			streaminterfacesource[i].maxretrytime = atoi(PQgetvalue(res, i, 10));
		}

		// ====== streammissfailovertimeout ======
		if (PQgetisnull(res, i, 11))
		{
			streaminterfacesource[i].streammissfailovertimeout = 0;
		}
		else
		{
			streaminterfacesource[i].streammissfailovertimeout = atoi(PQgetvalue(res, i, 11));
		}
	}

	PQclear(res);
	return 0;
}

int dbfetchstreaminterfacesinkbystreaminterfaceconfigid(PGconn **conn, s_interfacesink *streaminterfacesink,
		uint64_t streaminterfaceconfigid)
{
	char query[MEDIUM_QUERY_LEN];
	PGresult *res;

	if (conn == NULL || streaminterfacesink == NULL)
	{
		syslog(LOG_CRIT, "dbfetchstreaminterfacesinkbystreaminterfaceconfigid argument is NULL!");
		return 1;
	}
	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "dbfetchstreaminterfacesinkbystreaminterfaceconfigid cant reconnect DB connection");
		return 1;
	}

	int len = snprintf(query, MEDIUM_QUERY_LEN,
		"SELECT id, streaminterfaceconfig_id, name, sinktype, " \
		" host, port, talkbackhost, talkbackport, " \
		" ttl, hostclientcertificate_id, talkbackclientcertificate_id,"\
		" maxretrytime, streammissfailovertimeout, priority, maxbps, rawudp, recording_id "\
		" FROM "\
		" streaminterfacesinks " \
		" WHERE streaminterfaceconfig_id=%" PRIu64,
		streaminterfaceconfigid);

	if (len == MEDIUM_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "dbfetchstreaminterfacesinkbystreaminterfaceconfigid snprintf buf is to small(%d/%d)!",
			len, MEDIUM_QUERY_LEN);
	}

#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr,"%s\n", query);
#endif

	res = PQexec(*conn, query);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "dbfetchstreaminterfacesinkbystreaminterfaceconfigid: Answer not ok\n");
#endif
		PQclear(res);
		return 2;
	}
	
	if (PQntuples(res) != 1)
	{
		PQclear(res);
		return 3;
	}

	// ====== ID ======
	if (PQgetisnull(res, 0, 0))
	{
		streaminterfacesink->id = 0;
	}
	else
	{
		streaminterfacesink->id = atoll(PQgetvalue(res, 0, 0));
	}

	// ====== streaminterfaceconfig_id ======
	if (PQgetisnull(res, 0, 1))
	{
		streaminterfacesink->streaminterfaceconfig_id = 0;
	}
	else
	{
		streaminterfacesink->streaminterfaceconfig_id = atoll(PQgetvalue(res, 0, 1));
	}

	// ====== name ======
	strncpy(streaminterfacesink->name, PQgetvalue(res, 0, 2), DB_MAX_NAME_LENGTH);
	streaminterfacesink->name[DB_MAX_NAME_LENGTH-1] = '\0';

	// ====== sinktype ======
	strncpy(streaminterfacesink->sinktype, PQgetvalue(res, 0, 3), DB_MAX_INTERFACETYPE);
	streaminterfacesink->sinktype[DB_MAX_INTERFACETYPE-1] = '\0';

	// ====== host ======
	strncpy(streaminterfacesink->host, PQgetvalue(res, 0, 4), DB_MAX_HOSTNAME_LENGTH);
	streaminterfacesink->host[DB_MAX_HOSTNAME_LENGTH-1] = '\0';

	// ====== port ======
	if (PQgetisnull(res, 0, 5))
	{
		streaminterfacesink->port = 0;
	}
	else
	{
		streaminterfacesink->port = atoi(PQgetvalue(res, 0, 5));
	}

	// ====== talkbackhost ======
	strncpy(streaminterfacesink->talkbackhost, PQgetvalue(res, 0, 6), DB_MAX_HOSTNAME_LENGTH);
	streaminterfacesink->talkbackhost[DB_MAX_HOSTNAME_LENGTH-1] = '\0';

	// ====== talkbackport ======
	if (PQgetisnull(res, 0, 7))
	{
		streaminterfacesink->talkbackport = 0;
	}
	else
	{
		streaminterfacesink->talkbackport = atoi(PQgetvalue(res, 0, 7));
	}

	// ====== ttl ======
	if (PQgetisnull(res, 0, 8))
	{
		streaminterfacesink->ttl = 1;
	}
	else
	{
		streaminterfacesink->ttl = atoi(PQgetvalue(res, 0, 8));
	}

	// ====== hostclientcertificate_id ======
	if (PQgetisnull(res, 0, 9))
	{
		streaminterfacesink->hostclientcertificate_id = 0;
	}
	else
	{
		streaminterfacesink->hostclientcertificate_id = atoll(PQgetvalue(res, 0, 9));
	}

	// ====== talkbackclientcertificate_id ======
	if (PQgetisnull(res, 0, 10))
	{
		streaminterfacesink->talkbackclientcertificate_id = 0;
	}
	else
	{
		streaminterfacesink->talkbackclientcertificate_id = atoll(PQgetvalue(res, 0, 10));
	}
	
	// ====== maxretrytime ======
	if (PQgetisnull(res, 0, 11))
	{
		streaminterfacesink->maxretrytime = 0;
	}
	else
	{
		streaminterfacesink->maxretrytime = atoi(PQgetvalue(res, 0, 11));
	}

	// ====== streammissfailovertimeout ======
	if (PQgetisnull(res, 0, 12))
	{
		streaminterfacesink->streammissfailovertimeout = 0;
	}
	else
	{
		streaminterfacesink->streammissfailovertimeout = atoi(PQgetvalue(res, 0, 12));
	}

	// ====== priority ======
	if (PQgetisnull(res, 0, 13))
	{
		streaminterfacesink->priority = 0;
	}
	else
	{
		streaminterfacesink->priority = atoi(PQgetvalue(res, 0, 13));
	}

	// ====== maxbps ======
	if (PQgetisnull(res, 0, 14))
	{
		streaminterfacesink->maxbps = 0;
	}
	else
	{
		streaminterfacesink->maxbps = atoll(PQgetvalue(res, 0, 14));
	}

	// ===== rawudp =====
	if (!strcmp(PQgetvalue(res,0,15),"t"))
	{
		streaminterfacesink->rawudp = 1;
	}
	else
	{
		streaminterfacesink->rawudp = 0;
	}

	// ===== recording_id =====
	if (PQgetisnull(res, 0, 16))
	{
		streaminterfacesink->recording_id = 0;
	}
	else
	{
		streaminterfacesink->recording_id = atoll(PQgetvalue(res, 0, 16));
	}
	
	PQclear(res);
	return 0;
}

int dbfetch_streaminterfacebackupsink_by_streaminterfacesinkid(PGconn **conn,
		s_interfacesink *streaminterfacesink,
		unsigned short *num_backup_sink,
		uint64_t streaminterfacesink_id)
{
	char query[MEDIUM_QUERY_LEN];
	PGresult *res;

	if (conn == NULL || streaminterfacesink == NULL)
	{
		syslog(LOG_CRIT, "dbfetch_streaminterfacebackupsink_by_streaminterfacesinkid argument is NULL!");
		return 1;
	}

	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "dbfetch_streaminterfacebackupsink_by_streaminterfacesinkid cant reconnect DB connection");
		return 1;
	}

	int len = snprintf(query, MEDIUM_QUERY_LEN,
		"SELECT id, name, sinktype, host, port, " \
		" talkbackhost, talkbackport, " \
		" hostclientcertificate_id, talkbackclientcertificate_id, "\
		" maxretrytime, streammissfailovertimeout, priority, maxbps, rawudp, ttl"\
		" FROM streaminterfacebackupsinks "\
		" WHERE streaminterfacesink_id=%" PRIu64		
		" ORDER BY priority",
		streaminterfacesink_id);

	if (len == MEDIUM_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "dbfetch_streaminterfacebackupsink_by_streaminterfacesinkid snprintf buf is to small(%d/%d)!",
			len, MEDIUM_QUERY_LEN);
	}

#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr,"%s\n", query);
#endif

	res = PQexec(*conn, query);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "dbfetch_streaminterfacebackupsink_by_streaminterfacesinkid: Answer not ok\n");
#endif
		PQclear(res);
		return 2;
	}
	
	int numrows = PQntuples(res);
	if (numrows <= 0)
	{
		PQclear(res);
		return 0;
	}

	if (numrows < DB_MAX_SINK-1)
	{
		*num_backup_sink = numrows;
	}
	else
	{
		*num_backup_sink = DB_MAX_SINK-1;
		syslog(LOG_WARNING, "dbfetch_streaminterfacebackupsink_by_streaminterfacesinkid: fetched more backup sources than the system permits");
	}
	int i;
	for (i = 0; i < *num_backup_sink; i++)
	{
		// ====== streaminterfacesource id ======
		// Reuse the same structure in the backup scenarios
		streaminterfacesink[i].streaminterfaceconfig_id = streaminterfacesink_id;

		// ====== ID ======
		if (PQgetisnull(res, i, 0))
		{
			streaminterfacesink[i].id = 0;
		}
		else
		{
			streaminterfacesink[i].id = atoll(PQgetvalue(res, i, 0));
		}

		// ====== name ======
		strncpy(streaminterfacesink[i].name, PQgetvalue(res, i, 1), DB_MAX_NAME_LENGTH);
		streaminterfacesink[i].name[DB_MAX_NAME_LENGTH-1] = '\0';

		// ====== sinktype ======
		strncpy(streaminterfacesink[i].sinktype, PQgetvalue(res, i, 2), DB_MAX_INTERFACETYPE);
		streaminterfacesink[i].sinktype[DB_MAX_INTERFACETYPE-1] = '\0';

		// ====== host ======
		strncpy(streaminterfacesink[i].host, PQgetvalue(res, i, 3), DB_MAX_HOSTNAME_LENGTH);
		streaminterfacesink[i].host[DB_MAX_HOSTNAME_LENGTH-1] = '\0';

		// ====== port ======
		if (PQgetisnull(res, i, 4))
		{
			streaminterfacesink[i].port = 0;
		}
		else
		{
			streaminterfacesink[i].port = atoi(PQgetvalue(res, i, 4));
		}

		// ====== talkbackhost ======
		strncpy(streaminterfacesink[i].talkbackhost, PQgetvalue(res, i, 5), DB_MAX_HOSTNAME_LENGTH);
		streaminterfacesink[i].talkbackhost[DB_MAX_HOSTNAME_LENGTH-1] = '\0';

		// ====== talkbackport ======
		if (PQgetisnull(res, i, 6))
		{
			streaminterfacesink[i].talkbackport = 0;
		}
		else
		{
			streaminterfacesink[i].talkbackport = atoi(PQgetvalue(res, i, 6));
		}

		// ====== hostclientcertificate_id ======
		if (PQgetisnull(res, i, 7))
		{
			streaminterfacesink[i].hostclientcertificate_id = 0;
		}
		else
		{
			streaminterfacesink[i].hostclientcertificate_id = atoll(PQgetvalue(res, i, 7));
		}

		// ====== talkbackclientcertificate_id ======
		if (PQgetisnull(res, i, 8))
		{
			streaminterfacesink[i].talkbackclientcertificate_id = 0;
		}
		else
		{
			streaminterfacesink[i].talkbackclientcertificate_id = atoll(PQgetvalue(res, i, 8));
		}
		
		// ====== maxretrytime ======
		if (PQgetisnull(res, i, 9))
		{
			streaminterfacesink[i].maxretrytime = 0;
		}
		else
		{
			streaminterfacesink[i].maxretrytime = atoi(PQgetvalue(res, i, 9));
		}

		// ====== streammissfailovertimeout ======
		if (PQgetisnull(res, i, 10))
		{
			streaminterfacesink[i].streammissfailovertimeout = 0;
		}
		else
		{
			streaminterfacesink[i].streammissfailovertimeout = atoi(PQgetvalue(res, i, 10));
		}

		// ====== priority ======
		if (PQgetisnull(res, i, 11))
		{
			streaminterfacesink[i].priority = 0;
		}
		else
		{
			streaminterfacesink[i].priority = atoi(PQgetvalue(res, i, 11));
		}

		// ====== maxbps ======
		if (PQgetisnull(res, i, 12))
		{
			streaminterfacesink[i].maxbps = 0;
		}
		else
		{
			streaminterfacesink[i].maxbps = atoll(PQgetvalue(res, i, 12));
		}

	// ===== rawudp =====
		if (!strcmp(PQgetvalue(res,i,13),"t"))
		{
			streaminterfacesink[i].rawudp = 1;
		}
		else
		{
			streaminterfacesink[i].rawudp = 0;
		}

		// ====== ttl ======
		if (PQgetisnull(res, i, 14))
		{
			streaminterfacesink[i].ttl = 0;
		}
		else
		{
			streaminterfacesink[i].ttl = atoll(PQgetvalue(res, i, 14));
		}
	
	}

	PQclear(res);
	return 0;
}

int dbupdatestreaminterfacesourcefromvirtualsource(PGconn **conn, dbvirtualsource *vsource, uint64_t streaminterfaceconfigid)
{
	char query[MEDIUM_QUERY_LEN];
	PGresult *res;
	
	if (conn == NULL || vsource == NULL)
	{
		syslog(LOG_CRIT, "%s: argument is NULL!",
			__FUNCTION__);
		return 1;
	}
	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s: cant reconnect DB connection",
			__FUNCTION__);
		return 1;
	}

	snprintf(query, MEDIUM_QUERY_LEN,
		"UPDATE streaminterfacesources " \
		" SET host='%s', port='%d', " \
		" vsourcerevision=%"PRIu64" " \
		" WHERE streaminterfaceconfig_id=%" PRIu64,
		vsource->address, vsource->port,
		vsource->revision, streaminterfaceconfigid);

#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n", query);
#endif
	
	res = PQexec(*conn, query);

	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "Answer not ok\n");
#endif
		PQclear(res);
		return 1;
	}

	PQclear(res);
	return 0;
}

// just update the usertext
int dbupdatestreaminterfaceusertext(PGconn **conn, uint64_t streaminterfaceid, char *_usertext)
{
	char query[BIG_QUERY_LEN];
	char *usertext;
	PGresult *res;

	if (conn == NULL || _usertext == NULL)
	{
		syslog(LOG_CRIT, "dbupdatestreaminterfaceusertext argument is NULL!");
		return 1;
	}
	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "dbupdatestreaminterfaceusertext cant reconnect DB connection");
		return 1;
	}

	if (strlen(_usertext) >= DB_MAX_NAME_LENGTH)
	{
		_usertext[DB_MAX_NAME_LENGTH-1] = '\0';
	}

	usertext = pqescape(_usertext);
	if (usertext == NULL)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "Sanitation failed!\n");
#endif
		return 2;
	}
	int len = snprintf(query, BIG_QUERY_LEN,
		"UPDATE streaminterfaces " \
		" SET usertext='%s' " \
		" WHERE id=%" PRIu64,
		usertext, streaminterfaceid);
	if (len == BIG_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "dbupdatestreaminterfaceusertext snprintf buf is to small(%d/%d)!", len, BIG_QUERY_LEN);
	}


	res = PQexec(*conn, query);

	free(usertext);
	usertext = NULL;

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
		fprintf(stderr, "dbupdatestreaminterfaceusertext: Answer not ok\n");
#endif
		PQclear(res);
		DBClose(*conn);
		// Assume link is down and restablish, fix to better handling
		*conn = DBConnect();
		syslog(LOG_WARNING, "dbupdatestreaminterfaceusertext: Cant write usertext update");
		return 3;
	}
	else if (atoi(PQcmdTuples(res)) < 1)
	{
		syslog(LOG_WARNING, "Unable to update usertext in streaminterfaces.");
		PQclear(res);
		return 4;

	}
	PQclear(res);

	return 0;
}

int dbupdatestreaminterfaceconfigautostart(PGconn **conn, uint64_t streaminterfaceconfigid, int autostart)
{
	char query[BIG_QUERY_LEN];
	char strautostart[10];
	PGresult *res;

	if (conn == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!",
			__FUNCTION__);
		return 1;
	}
	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection",
			__FUNCTION__);
		return 1;
	}

	if (autostart)
	{
		sprintf(strautostart, "t");
	}
	else
	{
		sprintf(strautostart, "f");
	}

	int len = snprintf(query, BIG_QUERY_LEN,
		"UPDATE streaminterfaceconfigs " \
		" SET autostart='%s' " \
		" WHERE id=%" PRIu64,
		strautostart, streaminterfaceconfigid);
	if (len == BIG_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is to small(%d/%d)!",
			__FUNCTION__, len, BIG_QUERY_LEN);
	}

	res = PQexec(*conn, query);

	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		if (PQresultStatus(res) != PGRES_BAD_RESPONSE ||
			PQresultStatus(res) != PGRES_FATAL_ERROR)
		{
			fprintf(stderr,
				"PQresultStatus(res) != PGRES_BAD_RESPONSE %d\n",
				(PQresultStatus(res) != PGRES_BAD_RESPONSE));
			fprintf(stderr,
				"PQresultStatus(res) != PGRES_FATAL_ERROR %d\n",
				(PQresultStatus(res) != PGRES_FATAL_ERROR));
		}
		fprintf(stderr, "%s: Answer not ok\n", __FUNCTION__);
#endif
		PQclear(res);
		DBClose(*conn);
		// Assume link is down and restablish, fix to better handling
		*conn = DBConnect();
		syslog(LOG_WARNING, "%s: Cant write autostart update",
			__FUNCTION__);
		return 3;
	}
	else if (atoi(PQcmdTuples(res)) < 1)
	{
		syslog(LOG_WARNING,
			"Unable to update autostart in streaminterfaceconfigs.");
		PQclear(res);
		return 4;

	}
	PQclear(res);

	return 0;
}

int dbupdatestreaminterfaceconfigmode(PGconn **conn, uint64_t streaminterfaceconfigid, char *_mode)
{
	char query[BIG_QUERY_LEN];
	char *mode;
	PGresult *res;

	if (conn == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!", __FUNCTION__);
		return 1;
	}
	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection",
			__FUNCTION__);
		return 1;
	}

	if (strlen(_mode) >= DB_MAX_NAME_LENGTH)
	{
		_mode[DB_MAX_NAME_LENGTH-1] = '\0';
	}

	if (strncmp(_mode, "transmitter", 11) &&
		strncmp(_mode, "receiver", 8))
	{
		return 2;
	}

	mode = pqescape(_mode);

	int len = snprintf(query, BIG_QUERY_LEN,
		"UPDATE streaminterfaceconfigs " \
		" SET mode='%s' " \
		" WHERE id=%" PRIu64,
		mode, streaminterfaceconfigid);
	if (len == BIG_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is to small(%d/%d)!",
			__FUNCTION__, len, BIG_QUERY_LEN);
	}


	res = PQexec(*conn, query);
	free(mode);
	mode = NULL;

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
		syslog(LOG_WARNING, "%s: Cant write mode update", __FUNCTION__);
		return 3;
	}
	else if (atoi(PQcmdTuples(res)) < 1)
	{
		syslog(LOG_WARNING, "Unable to update mode in streaminterfaceconfigs.");
		PQclear(res);
		return 4;

	}
	PQclear(res);

	return 0;
}

int dbupdatestreaminterfaceconfigmsbuffer(PGconn **conn, uint64_t streaminterfaceconfigid, int _msbuffer)
{
	char query[BIG_QUERY_LEN];
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

	int len = snprintf(query, BIG_QUERY_LEN,
		"UPDATE streaminterfaceconfigs " \
		" SET msbuffer='%d' " \
		" WHERE id=%" PRIu64,
		_msbuffer, streaminterfaceconfigid);
	if (len == BIG_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is to small(%d/%d)!",
			__FUNCTION__, len, BIG_QUERY_LEN);
	}

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
		syslog(LOG_WARNING, "%s: Cant write msbuffer update",
			__FUNCTION__);
		return 3;
	}
	else if (atoi(PQcmdTuples(res)) < 1)
	{
		syslog(LOG_WARNING, "Unable to update msbuffer in streaminterfaceconfigs.");
		PQclear(res);
		return 4;

	}
	PQclear(res);

	return 0;
}

int dbupdatestreaminterfaceconfigmultipath(PGconn **conn, uint64_t streaminterfaceconfigid, int multipath)
{
	char query[BIG_QUERY_LEN];
	char strmultipath[10];
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

	if (multipath)
	{
		sprintf(strmultipath, "t");
	}
	else
	{
		sprintf(strmultipath, "f");
	}

	int len = snprintf(query, BIG_QUERY_LEN,
		"UPDATE streaminterfaceconfigs " \
		" SET multipath='%s' " \
		" WHERE id=%" PRIu64,
		strmultipath, streaminterfaceconfigid);
	if (len == BIG_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is to small(%d/%d)!",
			__FUNCTION__, len, BIG_QUERY_LEN);
	}


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
		syslog(LOG_WARNING, "%s: Cant write multipath update", __FUNCTION__);
		return 3;
	}
	else if (atoi(PQcmdTuples(res)) < 1)
	{
		syslog(LOG_WARNING, "Unable to update multipath in streaminterfaceconfigs.");
		PQclear(res);
		return 4;

	}
	PQclear(res);

	return 0;
}

int dbupdatestreaminterfaceconfigmploadbalance(PGconn **conn, uint64_t streaminterfaceconfigid, int mploadbalance)
{
	char query[BIG_QUERY_LEN];
	char strmploadbalance[10];
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

	if (mploadbalance)
	{
		sprintf(strmploadbalance, "t");
	}
	else
	{
		sprintf(strmploadbalance, "f");
	}

	int len = snprintf(query, BIG_QUERY_LEN,
		"UPDATE streaminterfaceconfigs " \
		" SET mploadbalance='%s' " \
		" WHERE id=%" PRIu64,
		strmploadbalance, streaminterfaceconfigid);
	if (len == BIG_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is to small(%d/%d)!", __FUNCTION__, len, BIG_QUERY_LEN);
	}


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
		syslog(LOG_WARNING, "%s: Cant write mploadbalance update", __FUNCTION__);
		return 3;
	}
	else if (atoi(PQcmdTuples(res)) < 1)
	{
		syslog(LOG_WARNING, "Unable to update mploadbalance in streaminterfaceconfigs.");
		PQclear(res);
		return 4;

	}
	PQclear(res);

	return 0;
}

int dbupdatestreaminterfacesourcetype(PGconn **conn, uint64_t streaminterfacesourceid, char *_sourcetype)
{
	if (conn == NULL || _sourcetype == NULL)
	{
		syslog(LOG_WARNING, "dbupdatestreaminterfacesourcetype argument is NULL");
		return 1;
	}

	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "dbupdatestreaminterfacesourcetype cant reconnect DB connection");
		return 1;
	}

	char query[BIG_QUERY_LEN];
	char *sourcetype;
	PGresult *res;

	if (strlen(_sourcetype) >= DB_MAX_INTERFACETYPE)
	{
		_sourcetype[DB_MAX_INTERFACETYPE-1] = 0;
	}

	sourcetype = pqescape(_sourcetype);
	snprintf(query, BIG_QUERY_LEN,
		"UPDATE streaminterfacesources " \
		" SET sourcetype='%s' " \
		" WHERE id=%" PRIu64,
		sourcetype, streaminterfacesourceid);
	free(sourcetype);

	sourcetype=NULL;
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
		fprintf(stderr, "dbupdatestreaminterfacesourcetype: Answer not ok\n");
#endif
		PQclear(res);
		DBClose(*conn);
		// Assume link is down and restablish, fix to better handling
		*conn = DBConnect();
		syslog(LOG_WARNING, "%s: Cant write sourcetype update",__FUNCTION__ );
		return 2;
	}
	else if (atoi(PQcmdTuples(res)) < 1)
	{
		syslog(LOG_WARNING, "%s: Unable to update sourcetype in streaminterfacesources.", __FUNCTION__);
		PQclear(res);
		return 3;
	}

	PQclear(res);

	return 0;
}

int dbupdatestreaminterfacesourcehost(PGconn **conn, uint64_t streaminterfacesourceid, char *_sourcehost)
{
	if (conn == NULL || _sourcehost == NULL)
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
	char *sourcehost;
	PGresult *res;

	if (strlen(_sourcehost) >= DB_MAX_HOSTNAME_LENGTH)
	{
		_sourcehost[DB_MAX_HOSTNAME_LENGTH-1] = 0;
	}

	sourcehost = pqescape(_sourcehost);
	snprintf(query, BIG_QUERY_LEN,
		"UPDATE streaminterfacesources " \
		" SET host='%s' " \
		" WHERE id=%" PRIu64,
		sourcehost, streaminterfacesourceid);
	free(sourcehost);

	sourcehost=NULL;
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
		syslog(LOG_WARNING, "%s: Cant write host update", __FUNCTION__);
		return 2;
	}
	else if (atoi(PQcmdTuples(res)) < 1)
	{
		syslog(LOG_WARNING, "%s: Unable to update host in streaminterfacesources.", __FUNCTION__);
		PQclear(res);
		return 3;
	}

	PQclear(res);

	return 0;
}

int dbupdatestreaminterfacesourceport(PGconn **conn, uint64_t streaminterfacesourceid, uint16_t port)
{
	if (conn == NULL || port == 0)
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
		"UPDATE streaminterfacesources " \
		" SET port='%d' " \
		" WHERE id=%" PRIu64,
		port, streaminterfacesourceid);

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
		syslog(LOG_WARNING, "%s: Cant write port update", __FUNCTION__);
		return 2;
	}
	else if (atoi(PQcmdTuples(res)) < 1)
	{
		syslog(LOG_WARNING, "%s: Unable to update port in streaminterfacesources.", __FUNCTION__);
		PQclear(res);
		return 3;
	}

	PQclear(res);

	return 0;
}
int dbupdatestreaminterfacesourcerecordingid(PGconn **conn, uint64_t streaminterfacesourceid, uint64_t recordingid)
{
	if (conn == NULL || recordingid == 0)
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
		"UPDATE streaminterfacesources " \
		" SET recording_id=%"PRIu64 "" \
		" WHERE id=%" PRIu64,
		recordingid, streaminterfacesourceid);

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
		syslog(LOG_WARNING, "%s: Cant write recordingid update", __FUNCTION__);
		return 2;
	}
	else if (atoi(PQcmdTuples(res)) < 1)
	{
		syslog(LOG_WARNING, "%s: Unable to update recordingid in streaminterfacesources.", __FUNCTION__);
		PQclear(res);
		return 3;
	}

	PQclear(res);

	return 0;
}

int dbupdatestreaminterfacesourcetalkbackhost(PGconn **conn, uint64_t streaminterfacesourceid, char *_talkbackhost)
{
	if (conn == NULL || _talkbackhost == NULL)
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
	char *talkbackhost;
	PGresult *res;

	if (strlen(_talkbackhost) >= DB_MAX_HOSTNAME_LENGTH)
	{
		_talkbackhost[DB_MAX_HOSTNAME_LENGTH-1] = 0;
	}

	talkbackhost = pqescape(_talkbackhost);
	snprintf(query, BIG_QUERY_LEN,
		"UPDATE streaminterfacesources " \
		" SET talkbackhost='%s' " \
		" WHERE id=%" PRIu64,
		talkbackhost, streaminterfacesourceid);
	free(talkbackhost);

	talkbackhost=NULL;
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
		syslog(LOG_WARNING, "%s: Cant write talkbackhost update", __FUNCTION__);
		return 2;
	}
	else if (atoi(PQcmdTuples(res)) < 1)
	{
		syslog(LOG_WARNING, "%s: Unable to update talkbackhost in streaminterfacesources.", __FUNCTION__);
		PQclear(res);
		return 3;
	}

	PQclear(res);

	return 0;
}

int dbupdatestreaminterfacesourcetalkbackport(PGconn **conn, uint64_t streaminterfacesourceid, uint16_t talkbackport)
{
	if (conn == NULL || talkbackport == 0)
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
		"UPDATE streaminterfacesources " \
		" SET talkbackport='%d' " \
		" WHERE id=%" PRIu64,
		talkbackport, streaminterfacesourceid);

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
		syslog(LOG_WARNING, "%s: Cant write talkbackport update", __FUNCTION__);
		return 2;
	}
	else if (atoi(PQcmdTuples(res)) < 1)
	{
		syslog(LOG_WARNING, "%s: Unable to update talkbackport in streaminterfacesources.", __FUNCTION__);
		PQclear(res);
		return 3;
	}

	PQclear(res);

	return 0;
}

int dbupdatestreaminterfacesourcevirtualsourceid(PGconn **conn, uint64_t streaminterfacesourceid, uint64_t virtualsourceid)
{
	if (conn == NULL)
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
  
	if (virtualsourceid != 0)
	{
		snprintf(query, BIG_QUERY_LEN,
			"UPDATE streaminterfacesources " \
			" SET virtualsource_id='%" PRIu64 "' " \
			" WHERE id=%" PRIu64,
			virtualsourceid, streaminterfacesourceid);
	}
	else 
	{
		snprintf(query, BIG_QUERY_LEN,
			"UPDATE streaminterfacesources SET " \
			" virtualsource_id=null " \
			" WHERE id=%" PRIu64,
			streaminterfacesourceid);
	}
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
		syslog(LOG_WARNING, "%s: Cant write virtualsource_id update", __FUNCTION__);
		return 2;
	}
	else if (atoi(PQcmdTuples(res)) < 1)
	{
		syslog(LOG_WARNING, "%s: Unable to update virtualsourceid in streaminterfacesources.", __FUNCTION__);
		PQclear(res);
		return 3;
	}

	PQclear(res);

	return 0;
}

int dbupdatestreaminterfacesourcehostclientcertificateid(PGconn **conn, uint64_t streaminterfacesourceid, uint64_t hostclientcertificateid)
{
	if (conn == NULL || hostclientcertificateid == 0)
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
		"UPDATE streaminterfacesources " \
		" SET hostclientcertificate_id='%" PRIu64 "' " \
		" WHERE id=%" PRIu64,
		hostclientcertificateid, streaminterfacesourceid);

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
		syslog(LOG_WARNING, "%s: Cant write hostclientcertificate_id update", __FUNCTION__);
		return 2;
	}
	else if (atoi(PQcmdTuples(res)) < 1)
	{
		syslog(LOG_WARNING, "%s: Unable to update hostclientcertificate_id in streaminterfacesources.", __FUNCTION__);
		PQclear(res);
		return 3;
	}

	PQclear(res);

	return 0;
}

int dbupdatestreaminterfacesourcetalkbackclientcertificateid(PGconn **conn, uint64_t streaminterfacesourceid, uint64_t talkbackclientcertificateid)
{
	if (conn == NULL || talkbackclientcertificateid == 0)
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
		"UPDATE streaminterfacesources " \
		" SET talkbackclientcertificate_id='%" PRIu64 "' " \
		" WHERE id=%" PRIu64,
		talkbackclientcertificateid, streaminterfacesourceid);

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
		syslog(LOG_WARNING, "%s: Cant write talkbackclientcertificate_id update", __FUNCTION__);
		return 2;
	}
	else if (atoi(PQcmdTuples(res)) < 1)
	{
		syslog(LOG_WARNING, "%s: Unable to update talkbackclientcertificate_id in streaminterfacesources.", __FUNCTION__);
		PQclear(res);
		return 3;
	}

	PQclear(res);

	return 0;
}

int dbupdatestreaminterfacesinktype(PGconn **conn, uint64_t streaminterfacesinkid, char *_sinktype)
{
	char query[BIG_QUERY_LEN];
	char *sinktype;
	PGresult *res;

	if (conn == NULL || _sinktype == NULL)
	{
		syslog(LOG_WARNING, "dbupdatestreaminterfacesinktype argument is NULL");
		return 1;
	}	
	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "dbupdatestreaminterfacesinktype cant reconnect DB connection");
		return 1;
	}

	if (strlen(_sinktype) >= DB_MAX_INTERFACETYPE)
	{
		_sinktype[DB_MAX_INTERFACETYPE-1] = 0;
	}

	sinktype = pqescape(_sinktype);
	snprintf(query, BIG_QUERY_LEN,
		"UPDATE streaminterfacesinks " \
		" SET sinkype='%s' " \
		" WHERE id=%" PRIu64,
		sinktype, streaminterfacesinkid);
	free(sinktype);
	sinktype=NULL;

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
		fprintf(stderr, "dbupdatestreaminterfacesinktype: Answer not ok\n");
#endif
		PQclear(res);
		DBClose(*conn);
		// Assume link is down and restablish, fix to better handling
		*conn = DBConnect();
		syslog(LOG_WARNING, "dbupdatestreaminterfacesinktype: Cant write usertext update");
		return 2;
	}
	else if (atoi(PQcmdTuples(res)) < 1)
	{
		syslog(LOG_WARNING, "dbupdatestreaminterfacesinktype: Unable to update usertext in streaminterfaces.");
		PQclear(res);
		return 3;
	}

	PQclear(res);

	return 0;
}

int dbupdatestreaminterfacesinkttl(PGconn **conn, uint64_t streaminterfacesinkid, uint16_t ttl)
{
	if (conn == NULL || ttl == 0)
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
		"UPDATE streaminterfacesinks " \
		" SET ttl='%d' " \
		" WHERE id=%" PRIu64,
		ttl, streaminterfacesinkid);

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
		syslog(LOG_WARNING, "%s: Cant write ttl update", __FUNCTION__);
		return 2;
	}
	else if (atoi(PQcmdTuples(res)) < 1)
	{
		syslog(LOG_WARNING, "%s: Unable to update ttl in streaminterfacesinks.", __FUNCTION__);
		PQclear(res);
		return 3;
	}

	PQclear(res);

	return 0;
}


int dbupdatestreaminterfacesinkhost(PGconn **conn, uint64_t streaminterfacesinkid, char *_sinkhost)
{
	if (conn == NULL || _sinkhost == NULL)
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
	char *sinkhost;
	PGresult *res;

	if (strlen(_sinkhost) >= DB_MAX_HOSTNAME_LENGTH)
	{
		_sinkhost[DB_MAX_HOSTNAME_LENGTH-1] = 0;
	}

	sinkhost = pqescape(_sinkhost);
	snprintf(query, BIG_QUERY_LEN,
		"UPDATE streaminterfacesinks " \
		" SET host='%s' " \
		" WHERE id=%" PRIu64,
		sinkhost, streaminterfacesinkid);
	free(sinkhost);

	sinkhost=NULL;
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
		syslog(LOG_WARNING, "%s: Cant write host update", __FUNCTION__);
		return 2;
	}
	else if (atoi(PQcmdTuples(res)) < 1)
	{
		syslog(LOG_WARNING, "%s: Unable to update host in streaminterfacesinks.", __FUNCTION__);
		PQclear(res);
		return 3;
	}

	PQclear(res);

	return 0;
}

int dbupdatestreaminterfacesinkport(PGconn **conn, uint64_t streaminterfacesinkid, uint16_t port)
{
	if (conn == NULL || port == 0)
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
		"UPDATE streaminterfacesinks " \
		" SET port='%d' " \
		" WHERE id=%" PRIu64,
		port, streaminterfacesinkid);

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
		syslog(LOG_WARNING, "%s: Cant write port update", __FUNCTION__);
		return 2;
	}
	else if (atoi(PQcmdTuples(res)) < 1)
	{
		syslog(LOG_WARNING, "%s: Unable to update port in streaminterfacesinks.", __FUNCTION__);
		PQclear(res);
		return 3;
	}

	PQclear(res);

	return 0;
}


int dbupdatestreaminterfacesinktalkbackhost(PGconn **conn, uint64_t streaminterfacesinkid, char *_talkbackhost)
{
	if (conn == NULL || _talkbackhost == NULL)
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
	char *talkbackhost;
	PGresult *res;

	if (strlen(_talkbackhost) >= DB_MAX_HOSTNAME_LENGTH)
	{
		_talkbackhost[DB_MAX_HOSTNAME_LENGTH-1] = 0;
	}

	talkbackhost = pqescape(_talkbackhost);
	snprintf(query, BIG_QUERY_LEN,
		"UPDATE streaminterfacesinks " \
		" SET talkbackhost='%s' " \
		" WHERE id=%" PRIu64,
		talkbackhost, streaminterfacesinkid);
	free(talkbackhost);

	talkbackhost=NULL;
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
		syslog(LOG_WARNING, "%s: Cant write talkbackhost update", __FUNCTION__);
		return 2;
	}
	else if (atoi(PQcmdTuples(res)) < 1)
	{
		syslog(LOG_WARNING, "%s: Unable to update talkbackhost in streaminterfacesinks.", __FUNCTION__);
		PQclear(res);
		return 3;
	}

	PQclear(res);

	return 0;
}

int dbupdatestreaminterfacesinktalkbackport(PGconn **conn, uint64_t streaminterfacesinkid, uint16_t talkbackport)
{
	if (conn == NULL || talkbackport == 0)
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
		"UPDATE streaminterfacesinks " \
		" SET talkbackport='%d' " \
		" WHERE id=%" PRIu64,
		talkbackport, streaminterfacesinkid);

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
		syslog(LOG_WARNING, "%s: Cant write talkbackport update", __FUNCTION__);
		return 2;
	}
	else if (atoi(PQcmdTuples(res)) < 1)
	{
		syslog(LOG_WARNING, "%s: Unable to update talkbackport in streaminterfacesinks.", __FUNCTION__);
		PQclear(res);
		return 3;
	}

	PQclear(res);

	return 0;
}

int dbupdatestreaminterfacesinkhostclientcertificateid(PGconn **conn, uint64_t streaminterfacesourceid, uint64_t hostclientcertificateid)
{
	if (conn == NULL || hostclientcertificateid == 0)
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
		"UPDATE streaminterfacesinks " \
		" SET hostclientcertificate_id='%" PRIu64 "' " \
		" WHERE id=%" PRIu64,
		hostclientcertificateid, streaminterfacesourceid);

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
		syslog(LOG_WARNING, "%s: Cant write hostclientcertificate_id update", __FUNCTION__);
		return 2;
	}
	else if (atoi(PQcmdTuples(res)) < 1)
	{
		syslog(LOG_WARNING, "%s: Unable to update hostclientcertificate_id in streaminterfacesources.", __FUNCTION__);
		PQclear(res);
		return 3;
	}

	PQclear(res);

	return 0;
}

int dbupdatestreaminterfacesinktalkbackclientcertificateid(PGconn **conn, uint64_t streaminterfacesourceid, uint64_t talkbackclientcertificateid)
{
	if (conn == NULL || talkbackclientcertificateid == 0)
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
		"UPDATE streaminterfacesinks " \
		" SET talkbackclientcertificate_id='%" PRIu64 "' " \
		" WHERE id=%" PRIu64,
		talkbackclientcertificateid, streaminterfacesourceid);

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
		syslog(LOG_WARNING, "%s: Cant write talkbackclientcertificate_id update", __FUNCTION__);
		return 2;
	}
	else if (atoi(PQcmdTuples(res)) < 1)
	{
		syslog(LOG_WARNING, "%s: Unable to update talkbackclientcertificate_id in streaminterfacesources.", __FUNCTION__);
		PQclear(res);
		return 3;
	}

	PQclear(res);

	return 0;
}

int dbupdatestreaminterfacebackupsourcename(PGconn **conn, uint64_t streaminterfacebackupsourceid, char *_name)
{
	char query[BIG_QUERY_LEN];
	char *name;
	PGresult *res;

	if (conn == NULL || _name == NULL)
	{
		syslog(LOG_WARNING, "%s argument is NULL", __FUNCTION__);
		return 1;
	}	
	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection", __FUNCTION__);
		return 1;
	}

	if (strlen(_name) >= DB_MAX_HOSTNAME_LENGTH)
	{
		_name[DB_MAX_HOSTNAME_LENGTH-1] = 0;
	}

	name = pqescape(_name);
	snprintf(query, BIG_QUERY_LEN,
		"UPDATE streaminterfacebackupsources " \
		" SET name='%s' " \
		" WHERE id=%" PRIu64,
		name, streaminterfacebackupsourceid);
	free(name);
	name=NULL;

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
		syslog(LOG_WARNING, "%s: Cant write usertext update", __FUNCTION__);
		return 2;
	}
	else if (atoi(PQcmdTuples(res)) < 1)
	{
		syslog(LOG_WARNING, "%s: Unable to update usertext in streaminterfaces.", __FUNCTION__);
		PQclear(res);
		return 3;
	}

	PQclear(res);

	return 0;
}

int dbupdatestreaminterfacebackupsourcemaxretrytime(PGconn **conn, uint64_t streaminterfacebackupsourceid, uint16_t maxretrytime)
{
	if (conn == NULL || maxretrytime < 0)
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
		"UPDATE streaminterfacebackupsources " \
		" SET port='%d' " \
		" WHERE id=%" PRIu64,
		maxretrytime, streaminterfacebackupsourceid);

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
		syslog(LOG_WARNING, "%s: Cant write port update", __FUNCTION__);
		return 2;
	}
	else if (atoi(PQcmdTuples(res)) < 1)
	{
		syslog(LOG_WARNING, "%s: Unable to update port in streaminterfacesinks.", __FUNCTION__);
		PQclear(res);
		return 3;
	}

	PQclear(res);

	return 0;
}

int dbupdatestreaminterfacebackupsourcestreammissfailovertimeout(PGconn **conn, uint64_t streaminterfacebackupsourceid, uint16_t streammisstimeout)
{
	if (conn == NULL || streammisstimeout < 0)
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
		"UPDATE streaminterfacebackupsources " \
		" SET streammissfailovertimeout='%d' " \
		" WHERE id=%" PRIu64,
		streammisstimeout, streaminterfacebackupsourceid);

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
		syslog(LOG_WARNING, "%s: Cant write port update", __FUNCTION__);
		return 2;
	}
	else if (atoi(PQcmdTuples(res)) < 1)
	{
		syslog(LOG_WARNING, "%s: Unable to update port in streaminterfacesinks.", __FUNCTION__);
		PQclear(res);
		return 3;
	}

	PQclear(res);

	return 0;
}

int dbupdatestreaminterfacebackupsourcetype(PGconn **conn, uint64_t streaminterfacebackupsourceid, char *_sourcetype)
{
	if (conn == NULL || _sourcetype == NULL)
	{
		syslog(LOG_WARNING, "dbupdatestreaminterfacesourcetype argument is NULL");
		return 1;
	}

	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "dbupdatestreaminterfacesourcetype cant reconnect DB connection");
		return 1;
	}

	char query[BIG_QUERY_LEN];
	char *sourcetype;
	PGresult *res;

	if (strlen(_sourcetype) >= DB_MAX_INTERFACETYPE)
	{
		_sourcetype[DB_MAX_INTERFACETYPE-1] = 0;
	}

	sourcetype = pqescape(_sourcetype);
	snprintf(query, BIG_QUERY_LEN,
		"UPDATE streaminterfacebackupsources " \
		" SET sourcetype='%s' " \
		" WHERE id=%" PRIu64,
		sourcetype, streaminterfacebackupsourceid);
	free(sourcetype);

	sourcetype=NULL;
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
		fprintf(stderr, "dbupdatestreaminterfacesourcetype: Answer not ok\n");
#endif
		PQclear(res);
		DBClose(*conn);
		// Assume link is down and restablish, fix to better handling
		*conn = DBConnect();
		syslog(LOG_WARNING, "%s: Cant write sourcetype update",__FUNCTION__ );
		return 2;
	}
	else if (atoi(PQcmdTuples(res)) < 1)
	{
		syslog(LOG_WARNING, "%s: Unable to update sourcetype in streaminterfacesources.", __FUNCTION__);
		PQclear(res);
		return 3;
	}

	PQclear(res);

	return 0;
}

int dbupdatestreaminterfacebackupsourcehost(PGconn **conn, uint64_t streaminterfacebackupsourceid, char *_host)
{
	char query[BIG_QUERY_LEN];
	char *host;
	PGresult *res;

	if (conn == NULL || _host == NULL)
	{
		syslog(LOG_WARNING, "%s argument is NULL", __FUNCTION__);
		return 1;
	}	
	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection", __FUNCTION__);
		return 1;
	}

	if (strlen(_host) >= DB_MAX_HOSTNAME_LENGTH)
	{
		_host[DB_MAX_HOSTNAME_LENGTH-1] = 0;
	}

	host = pqescape(_host);
	snprintf(query, BIG_QUERY_LEN,
		"UPDATE streaminterfacebackupsources " \
		" SET host='%s' " \
		" WHERE id=%" PRIu64,
		host, streaminterfacebackupsourceid);
	free(host);
	host=NULL;

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
		syslog(LOG_WARNING, "%s: Cant write usertext update", __FUNCTION__);
		return 2;
	}
	else if (atoi(PQcmdTuples(res)) < 1)
	{
		syslog(LOG_WARNING, "%s: Unable to update usertext in streaminterfaces.", __FUNCTION__);
		PQclear(res);
		return 3;
	}

	PQclear(res);

	return 0;
}

int dbupdatestreaminterfacebackupsourceport(PGconn **conn, uint64_t streaminterfacebackupsourceid, uint16_t port)
{
	if (conn == NULL || port == 0)
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
		"UPDATE streaminterfacebackupsources " \
		" SET port='%d' " \
		" WHERE id=%" PRIu64,
		port, streaminterfacebackupsourceid);

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
		syslog(LOG_WARNING, "%s: Cant write port update", __FUNCTION__);
		return 2;
	}
	else if (atoi(PQcmdTuples(res)) < 1)
	{
		syslog(LOG_WARNING, "%s: Unable to update port in streaminterfacesinks.", __FUNCTION__);
		PQclear(res);
		return 3;
	}

	PQclear(res);

	return 0;
}

int dbupdatestreaminterfacebackupsourcetalkbackhost(PGconn **conn, uint64_t streaminterfacebackupsourceid, char *_talkbackhost)
{
	char query[BIG_QUERY_LEN];
	char *talkbackhost;
	PGresult *res;

	if (conn == NULL || _talkbackhost == NULL)
	{
		syslog(LOG_WARNING, "%s argument is NULL", __FUNCTION__);
		return 1;
	}	
	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection", __FUNCTION__);
		return 1;
	}

	if (strlen(_talkbackhost) >= DB_MAX_HOSTNAME_LENGTH)
	{
		_talkbackhost[DB_MAX_HOSTNAME_LENGTH-1] = 0;
	}

	talkbackhost = pqescape(_talkbackhost);
	snprintf(query, BIG_QUERY_LEN,
		"UPDATE streaminterfacebackupsources " \
		" SET talkbackhost='%s' " \
		" WHERE id=%" PRIu64,
		talkbackhost, streaminterfacebackupsourceid);
	free(talkbackhost);
	talkbackhost=NULL;

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
		syslog(LOG_WARNING, "%s: Cant write usertext update", __FUNCTION__);
		return 2;
	}
	else if (atoi(PQcmdTuples(res)) < 1)
	{
		syslog(LOG_WARNING, "%s: Unable to update usertext in streaminterfaces.", __FUNCTION__);
		PQclear(res);
		return 3;
	}

	PQclear(res);

	return 0;
}

int dbupdatestreaminterfacebackupsourcetalkbackport(PGconn **conn, uint64_t streaminterfacebackupsourceid, uint16_t talkbackport)
{
	if (conn == NULL || talkbackport == 0)
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
		"UPDATE streaminterfacebackupsources " \
		" SET talkbackport='%d' " \
		" WHERE id=%" PRIu64,
		talkbackport, streaminterfacebackupsourceid);

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
		syslog(LOG_WARNING, "%s: Cant write port update", __FUNCTION__);
		return 2;
	}
	else if (atoi(PQcmdTuples(res)) < 1)
	{
		syslog(LOG_WARNING, "%s: Unable to update port in streaminterfacesinks.", __FUNCTION__);
		PQclear(res);
		return 3;
	}

	PQclear(res);

	return 0;
}

int dbupdatestreaminterfacebackupsourcehostclientcertificateid(PGconn **conn, uint64_t streaminterfacesourceid, uint64_t hostclientcertificateid)
{
	if (conn == NULL || hostclientcertificateid == 0)
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
		"UPDATE streaminterfacbackupesources " \
		" SET hostclientcertificate_id='%" PRIu64 "' " \
		" WHERE id=%" PRIu64,
		hostclientcertificateid, streaminterfacesourceid);

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
		syslog(LOG_WARNING, "%s: Cant write hostclientcertificate_id update", __FUNCTION__);
		return 2;
	}
	else if (atoi(PQcmdTuples(res)) < 1)
	{
		syslog(LOG_WARNING, "%s: Unable to update hostclientcertificate_id in streaminterfacebackupsources.", __FUNCTION__);
		PQclear(res);
		return 3;
	}

	PQclear(res);

	return 0;
}

int dbupdatestreaminterfacebackupsourcetalkbackclientcertificateid(PGconn **conn, uint64_t streaminterfacesourceid, uint64_t talkbackclientcertificateid)
{
	if (conn == NULL || talkbackclientcertificateid == 0)
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
		"UPDATE streaminterfacebackupsources " \
		" SET talkbackclientcertificate_id='%" PRIu64 "' " \
		" WHERE id=%" PRIu64,
		talkbackclientcertificateid, streaminterfacesourceid);

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
		syslog(LOG_WARNING, "%s: Cant write talkbackclientcertificate_id update", __FUNCTION__);
		return 2;
	}
	else if (atoi(PQcmdTuples(res)) < 1)
	{
		syslog(LOG_WARNING, "%s: Unable to update talkbackclientcertificate_id in streaminterfacebackupsources.", __FUNCTION__);
		PQclear(res);
		return 3;
	}

	PQclear(res);

	return 0;
}


int dbupdatestreaminterfacebackupsinkname(PGconn **conn, uint64_t streaminterfacebackupsinkid, char *_name)
{
	char query[BIG_QUERY_LEN];
	char *name;
	PGresult *res;

	if (conn == NULL || _name == NULL)
	{
		syslog(LOG_WARNING, "%s argument is NULL", __FUNCTION__);
		return 1;
	}	
	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection", __FUNCTION__);
		return 1;
	}

	if (strlen(_name) >= DB_MAX_HOSTNAME_LENGTH)
	{
		_name[DB_MAX_HOSTNAME_LENGTH-1] = 0;
	}

	name = pqescape(_name);
	snprintf(query, BIG_QUERY_LEN,
		"UPDATE streaminterfacebackupsinks " \
		" SET name='%s' " \
		" WHERE id=%" PRIu64,
		name, streaminterfacebackupsinkid);
	free(name);
	name=NULL;

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
		syslog(LOG_WARNING, "%s: Cant write usertext update", __FUNCTION__);
		return 2;
	}
	else if (atoi(PQcmdTuples(res)) < 1)
	{
		syslog(LOG_WARNING, "%s: Unable to update usertext in streaminterfaces.", __FUNCTION__);
		PQclear(res);
		return 3;
	}

	PQclear(res);

	return 0;
}

int dbupdatestreaminterfacebackupsinkmaxretrytime(PGconn **conn, uint64_t streaminterfacebackupsinkid, uint16_t maxretrytime)
{
	if (conn == NULL || maxretrytime < 0)
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
		"UPDATE streaminterfacebackupsinks " \
		" SET port='%d' " \
		" WHERE id=%" PRIu64,
		maxretrytime, streaminterfacebackupsinkid);

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
		syslog(LOG_WARNING, "%s: Cant write port update", __FUNCTION__);
		return 2;
	}
	else if (atoi(PQcmdTuples(res)) < 1)
	{
		syslog(LOG_WARNING, "%s: Unable to update port in streaminterfacesinks.", __FUNCTION__);
		PQclear(res);
		return 3;
	}

	PQclear(res);

	return 0;
}

int dbupdatestreaminterfacebackupsinktype(PGconn **conn, uint64_t streaminterfacebackupsinkid, char *_sinktype)
{
	char query[BIG_QUERY_LEN];
	char *sinktype;
	PGresult *res;

	if (conn == NULL || _sinktype == NULL)
	{
		syslog(LOG_WARNING, "%s argument is NULL", __FUNCTION__);
		return 1;
	}	
	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection", __FUNCTION__);
		return 1;
	}

	if (strlen(_sinktype) >= DB_MAX_INTERFACETYPE)
	{
		_sinktype[DB_MAX_INTERFACETYPE-1] = 0;
	}

	sinktype = pqescape(_sinktype);
	snprintf(query, BIG_QUERY_LEN,
		"UPDATE streaminterfacebackupsinks " \
		" SET sinkype='%s' " \
		" WHERE id=%" PRIu64,
		sinktype, streaminterfacebackupsinkid);
	free(sinktype);
	sinktype=NULL;

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
		fprintf(stderr, "dbupdatestreaminterfacesinktype: Answer not ok\n");
#endif
		PQclear(res);
		DBClose(*conn);
		// Assume link is down and restablish, fix to better handling
		*conn = DBConnect();
		syslog(LOG_WARNING, "%s: Cant write usertext update", __FUNCTION__);
		return 2;
	}
	else if (atoi(PQcmdTuples(res)) < 1)
	{
		syslog(LOG_WARNING, "%s: Unable to update usertext in streaminterfaces.", __FUNCTION__);
		PQclear(res);
		return 3;
	}

	PQclear(res);

	return 0;
}

int dbupdatestreaminterfacebackupsinkhost(PGconn **conn, uint64_t streaminterfacebackupsinkid, char *_host)
{
	char query[BIG_QUERY_LEN];
	char *host;
	PGresult *res;

	if (conn == NULL || _host == NULL)
	{
		syslog(LOG_WARNING, "%s argument is NULL", __FUNCTION__);
		return 1;
	}	
	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection", __FUNCTION__);
		return 1;
	}

	if (strlen(_host) >= DB_MAX_HOSTNAME_LENGTH)
	{
		_host[DB_MAX_HOSTNAME_LENGTH-1] = 0;
	}

	host = pqescape(_host);
	snprintf(query, BIG_QUERY_LEN,
		"UPDATE streaminterfacebackupsinks " \
		" SET host='%s' " \
		" WHERE id=%" PRIu64,
		host, streaminterfacebackupsinkid);
	free(host);
	host=NULL;

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
		syslog(LOG_WARNING, "%s: Cant write usertext update", __FUNCTION__);
		return 2;
	}
	else if (atoi(PQcmdTuples(res)) < 1)
	{
		syslog(LOG_WARNING, "%s: Unable to update usertext in streaminterfaces.", __FUNCTION__);
		PQclear(res);
		return 3;
	}

	PQclear(res);

	return 0;
}

int dbupdatestreaminterfacebackupsinkport(PGconn **conn, uint64_t streaminterfacebackupsinkid, uint16_t port)
{
	if (conn == NULL || port == 0)
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
		"UPDATE streaminterfacebackupsinks " \
		" SET port='%d' " \
		" WHERE id=%" PRIu64,
		port, streaminterfacebackupsinkid);

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
		syslog(LOG_WARNING, "%s: Cant write port update", __FUNCTION__);
		return 2;
	}
	else if (atoi(PQcmdTuples(res)) < 1)
	{
		syslog(LOG_WARNING, "%s: Unable to update port in streaminterfacesinks.", __FUNCTION__);
		PQclear(res);
		return 3;
	}

	PQclear(res);

	return 0;
}

int dbupdatestreaminterfacebackupsinktalkbackhost(PGconn **conn, uint64_t streaminterfacebackupsinkid, char *_talkbackhost)
{
	char query[BIG_QUERY_LEN];
	char *talkbackhost;
	PGresult *res;

	if (conn == NULL || _talkbackhost == NULL)
	{
		syslog(LOG_WARNING, "%s argument is NULL", __FUNCTION__);
		return 1;
	}	
	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection", __FUNCTION__);
		return 1;
	}

	if (strlen(_talkbackhost) >= DB_MAX_HOSTNAME_LENGTH)
	{
		_talkbackhost[DB_MAX_HOSTNAME_LENGTH-1] = 0;
	}

	talkbackhost = pqescape(_talkbackhost);
	snprintf(query, BIG_QUERY_LEN,
		"UPDATE streaminterfacebackupsinks " \
		" SET talkbackhost='%s' " \
		" WHERE id=%" PRIu64,
		talkbackhost, streaminterfacebackupsinkid);
	free(talkbackhost);
	talkbackhost=NULL;

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
		syslog(LOG_WARNING, "%s: Cant write usertext update", __FUNCTION__);
		return 2;
	}
	else if (atoi(PQcmdTuples(res)) < 1)
	{
		syslog(LOG_WARNING, "%s: Unable to update usertext in streaminterfaces.", __FUNCTION__);
		PQclear(res);
		return 3;
	}

	PQclear(res);

	return 0;
}

int dbupdatestreaminterfacebackupsinktalkbackport(PGconn **conn, uint64_t streaminterfacebackupsinkid, uint16_t talkbackport)
{
	if (conn == NULL || talkbackport == 0)
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
		"UPDATE streaminterfacebackupsinks " \
		" SET talkbackport='%d' " \
		" WHERE id=%" PRIu64,
		talkbackport, streaminterfacebackupsinkid);

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
		syslog(LOG_WARNING, "%s: Cant write port update", __FUNCTION__);
		return 2;
	}
	else if (atoi(PQcmdTuples(res)) < 1)
	{
		syslog(LOG_WARNING, "%s: Unable to update port in streaminterfacesinks.", __FUNCTION__);
		PQclear(res);
		return 3;
	}

	PQclear(res);

	return 0;
}

int dbupdatestreaminterfacebackupsinkhostclientcertificateid(PGconn **conn, uint64_t streaminterfacesinkid, uint64_t hostclientcertificateid)
{
	if (conn == NULL || hostclientcertificateid == 0)
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
		"UPDATE streaminterfacbackupsinks " \
		" SET hostclientcertificate_id='%" PRIu64 "' " \
		" WHERE id=%" PRIu64,
		hostclientcertificateid, streaminterfacesinkid);

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
		syslog(LOG_WARNING, "%s: Cant write hostclientcertificate_id update", __FUNCTION__);
		return 2;
	}
	else if (atoi(PQcmdTuples(res)) < 1)
	{
		syslog(LOG_WARNING, "%s: Unable to update hostclientcertificate_id in streaminterfacebackupsinks.", __FUNCTION__);
		PQclear(res);
		return 3;
	}

	PQclear(res);

	return 0;
}

int dbupdatestreaminterfacebackupsinktalkbackclientcertificateid(PGconn **conn, uint64_t streaminterfacesinkid, uint64_t talkbackclientcertificateid)
{
	if (conn == NULL || talkbackclientcertificateid == 0)
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
		"UPDATE streaminterfacebackupsinks " \
		" SET talkbackclientcertificate_id='%" PRIu64 "' " \
		" WHERE id=%" PRIu64,
		talkbackclientcertificateid, streaminterfacesinkid);

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
		syslog(LOG_WARNING, "%s: Cant write talkbackclientcertificate_id update", __FUNCTION__);
		return 2;
	}
	else if (atoi(PQcmdTuples(res)) < 1)
	{
		syslog(LOG_WARNING, "%s: Unable to update talkbackclientcertificate_id in streaminterfacebackupsinks.", __FUNCTION__);
		PQclear(res);
		return 3;
	}

	PQclear(res);

	return 0;
}


int dbaddstreaminterfacebackupsink(PGconn **conn, s_interfacebackupsink *backupsink)
{
	char *name, *sinktype, *host, *talkbackhost;
	char hostcertid[16], tbhostcertid[16]; 
	char query[DOUBLE_BIG_QUERY_LEN+1];
	PGresult *res;

	memset(hostcertid, 0, sizeof(char) * 16);
	memset(tbhostcertid, 0, sizeof(char) * 16);

	if (conn == NULL)
	{
		syslog(LOG_WARNING, "%s argument is NULL", __FUNCTION__);
		return 1;
	}

	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection", __FUNCTION__);
		return 1;
	}

	name = pqescape(backupsink->name);
	if (name == NULL)
	{
		return 2;
	}

	sinktype = pqescape(backupsink->sinktype);
	if (sinktype == NULL)
	{
		return 2;
	}

	host = pqescape(backupsink->host);
	if (host == NULL)
	{
		return 2;
	}

	talkbackhost = pqescape(backupsink->talkbackhost);
	if (talkbackhost == NULL)
	{
		return 2;
	}

	if (backupsink->hostclientcertificate_id == 0)
	{
		snprintf(hostcertid,15, "null");
	}
	else
	{
		snprintf(hostcertid, 15, "%" PRIu64, backupsink->hostclientcertificate_id);
	}

	if (backupsink->talkbackclientcertificate_id == 0)
	{
		snprintf(tbhostcertid, 15, "null");
	}
	else
	{
		snprintf(tbhostcertid,15, "%" PRIu64, backupsink->talkbackclientcertificate_id);
	}

	int len = snprintf(query, DOUBLE_BIG_QUERY_LEN,
		"INSERT INTO streaminterfacebackupsinks "\
		" (streaminterfacesink_id, name, priority, maxretrytime, " \
		" streammissfailovertimeout, sinktype, host, port, " \
		" talkbackhost, talkbackport, " \
		" hostclientcertificate_id, talkbackclientcertificate_id ) "\
		" VALUES "\
		" (%" PRIu64",%s,%d, %d, %d, '%s', '%s', %d, '%s', %d, %s, %s)",
		backupsink->streaminterfacesink_id, backupsink->name,
		backupsink->priority,
		backupsink->maxretrytime, backupsink->streammissfailovertimeout,
		backupsink->sinktype, backupsink->host, backupsink->port,
		backupsink->talkbackhost, backupsink->talkbackport,
		hostcertid, tbhostcertid
		);

	if (len == DOUBLE_BIG_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "%s: snprintf buf is to small(%d/%d)!",
			__FUNCTION__, len, DOUBLE_BIG_QUERY_LEN);
	}

	free(name);
	name = NULL;

	free(sinktype);
	sinktype = NULL;

	free(host);
	host = NULL;

	free(talkbackhost);
	talkbackhost = NULL;

#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n", query);
#endif
	res = PQexec(*conn, query);

	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "%s: Answer not ok: %s\n", __FUNCTION__, PQerrorMessage(*conn));
#endif
		PQclear(res);
		return 2;
	}
	
	PQclear(res);
	return 0;
}

// inserts id in structure
int dbaddstreaminterfacebackupsource(PGconn **conn, s_interfacebackupsource *backupsource)
{
	char *name, *sourcetype, *host, *talkbackhost;
	char hostcertid[16], tbhostcertid[16]; 
	char query[DOUBLE_BIG_QUERY_LEN+1];
	PGresult *res;

	memset(hostcertid, 0, sizeof(char) * 16);
	memset(tbhostcertid, 0, sizeof(char) * 16);

	if (conn == NULL)
	{
		syslog(LOG_WARNING, "%s argument is NULL",
			__FUNCTION__);
		return 1;
	}

	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection",
			__FUNCTION__);
		return 1;
	}

	name = pqescape(backupsource->name);
	if (name == NULL)
	{
		return 2;
	}

	sourcetype = pqescape(backupsource->sourcetype);
	if (sourcetype == NULL)
	{
		return 2;
	}

	host = pqescape(backupsource->host);
	if (host == NULL)
	{
		return 2;
	}

	talkbackhost = pqescape(backupsource->talkbackhost);
	if (talkbackhost == NULL)
	{
		return 2;
	}

	if (backupsource->hostclientcertificate_id == 0)
	{
		snprintf(hostcertid, 15, "null");
	}
	else
	{
		snprintf(hostcertid, 15, "%" PRIu64, backupsource->hostclientcertificate_id);
	}

	if (backupsource->talkbackclientcertificate_id == 0)
	{
		snprintf(tbhostcertid, 15, "null");
	}
	else
	{
		snprintf(tbhostcertid,15, "%" PRIu64, backupsource->talkbackclientcertificate_id);
	}

	int len = snprintf(query, DOUBLE_BIG_QUERY_LEN,
		"INSERT INTO streaminterfacebackupsources "\
		" (streaminterfacesource_id, name, priority, maxretrytime, " \
		" streammissfailovertimeout, sourcetype, host, port, " \
		" talkbackhost, talkbackport, " \
		" hostclientcertificate_id, talkbackclientcertificate_id ) "\
		" VALUES "\
		" (%" PRIu64",'%s',%d,%d,%d, '%s', '%s', %d, '%s', %d, %s, %s)",
		backupsource->streaminterfacesource_id, backupsource->name,
		backupsource->priority, backupsource->maxretrytime,
		backupsource->streammissfailovertimeout, backupsource->sourcetype,
		backupsource->host, backupsource->port,
		backupsource->talkbackhost, backupsource->talkbackport,
		hostcertid, tbhostcertid
		);

if (len == DOUBLE_BIG_QUERY_LEN || len < 0)
{
	syslog(LOG_WARNING, "%s: snprintf buf is to small(%d/%d)!",
		__FUNCTION__, len, DOUBLE_BIG_QUERY_LEN);
}

free(name);
	name = NULL;

	free(sourcetype);
	sourcetype = NULL;

	free(host);
	host = NULL;

	free(talkbackhost);
	talkbackhost = NULL;

#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n", query);
#endif
	res = PQexec(*conn, query);

	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "%s: Answer not ok: %s\n",
			__FUNCTION__, PQerrorMessage(*conn));
#endif
		PQclear(res);
		return 2;
	}
	
	PQclear(res);

	snprintf(query, DOUBLE_BIG_QUERY_LEN,
		"select currval('streaminterfacebackupsources_id_seq')");
#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "query: %s\n", query);
#endif
	res = PQexec(*conn, query);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		syslog(LOG_WARNING,
			"Could not fetch currval for streaminterfacebackupsources_id_seq. %d/%d (%s)",
			PQresultStatus(res), PGRES_TUPLES_OK, PQerrorMessage(*conn));
		PQclear(res);
		return 3;
	}
	PQclear(res);

	backupsource->id = atoll(PQgetvalue(res,0,0));
	return 0;
}

int dbdeletestreaminterfacebackupsourcebyid(PGconn **conn, uint64_t id)
{
	PGresult *res;
	char query[DOUBLE_BIG_QUERY_LEN+1];

	if (conn == NULL || id == 0)
	{
		syslog(LOG_WARNING, "%s argument is NULL",
			__FUNCTION__);
		return 1;
	}

	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection",
			__FUNCTION__);
		return 1;
	}

	int len = snprintf(query, DOUBLE_BIG_QUERY_LEN,
			"DELETE FROM streaminterfacebackupsources " \
			" WHERE id=%"PRIu64,
			id);

	if (len == DOUBLE_BIG_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "%s: snprintf buf is to small(%d/%d)!",
			__FUNCTION__, len, DOUBLE_BIG_QUERY_LEN);
	}

	res = PQexec(*conn, query);

	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "%s: Answer not ok: %s\n",
			__FUNCTION__, PQerrorMessage(*conn));
#endif
		syslog(LOG_WARNING, "Could not delete from streaminterfacebackupsources!");
		PQclear(res);
		return 2;
	}
	
	PQclear(res);
	return 0;
}

int dbdeletestreaminterfacebackupsinkbyid(PGconn **conn, uint64_t id)
{
	return 0;
}

int dbfetchstreaminterfacelivestatisticbystreaminterfaceid(PGconn **conn,
		s_interfacelivestatistic *streaminterfacelivestatistic,
		uint64_t streaminterfaceid)
{
	char query[MEDIUM_QUERY_LEN];
	PGresult *res;

	if (conn == NULL || streaminterfacelivestatistic == NULL)
	{
		syslog(LOG_CRIT, "dbfetchstreaminterfacelivestatisticsbystreaminterfaceid argument is NULL!");
		return 1;
	}
	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "dbfetchstreaminterfacelivestatisticbystreaminterfaceid cant reconnect DB connection");
		return 1;
	}

	int len = snprintf(query, MEDIUM_QUERY_LEN,
		"SELECT id, streaminterface_id, state, mode, " \
		" bitratein, bitrateout, source, destination, "\
		" timestamp, extract(epoch from timestamp)"\
		" FROM "\
		" streaminterfacelivestatistics " \
		" WHERE streaminterface_id=%" PRIu64,
		streaminterfaceid);

	if (len == MEDIUM_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "dbfetchstreaminterfacelivestatisticsbystreaminterfaceid snprintf buf is to small(%d/%d)!",
			len, MEDIUM_QUERY_LEN);
	}

#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr,"%s\n", query);
#endif

	res = PQexec(*conn, query);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "dbfetchstreaminterfacelivestatisticbystreaminterfaceid: Answer not ok\n");
#endif
		PQclear(res);
		return 2;
	}
	
	if (PQntuples(res) < 1)
	{
		PQclear(res);
		return 3;
	}

	// ====== id ======
	streaminterfacelivestatistic->id = atoll(PQgetvalue(res,0,0));

	// ====== streaminterface id ======
	if (PQgetisnull(res,0,1))
	{
		streaminterfacelivestatistic->streaminterface_id = 0;
	}
	else
	{
		streaminterfacelivestatistic->streaminterface_id = atoll(PQgetvalue(res,0,1));
	}

	// ====== state ======
	strncpy(streaminterfacelivestatistic->state, PQgetvalue(res,0,2), DB_MAX_STATE);
	streaminterfacelivestatistic->state[DB_MAX_STATE-1] = '\0';

	// ====== mode ======
	strncpy(streaminterfacelivestatistic->mode, PQgetvalue(res,0,3), DB_MAX_MODE_LENGTH);
	streaminterfacelivestatistic->mode[DB_MAX_MODE_LENGTH-1] = '\0';

	// ====== bitratein ======
	if (PQgetisnull(res,0,4))
	{
		streaminterfacelivestatistic->bitratein = 0;
	}
	else
	{
		streaminterfacelivestatistic->bitratein = (uint64_t)(strtold(PQgetvalue(res,0,4), NULL));
	}

	// ====== bitrateout ======
	if (PQgetisnull(res,0,5))
	{
		streaminterfacelivestatistic->bitrateout = 0;
	}
	else
	{
		streaminterfacelivestatistic->bitrateout = (uint64_t)(strtold(PQgetvalue(res,0,5), NULL));
	}

	// ====== source ======
	strncpy(streaminterfacelivestatistic->source, PQgetvalue(res,0,6), DB_MAX_HOSTNAME_LENGTH);
	streaminterfacelivestatistic->source[DB_MAX_HOSTNAME_LENGTH-1] = '\0';

	// ====== destination ======
	strncpy(streaminterfacelivestatistic->destination, PQgetvalue(res,0,7), DB_MAX_HOSTNAME_LENGTH);
	streaminterfacelivestatistic->destination[DB_MAX_HOSTNAME_LENGTH-1] = '\0';
	
	// ====== timestamp ======
	streaminterfacelivestatistic->timestamp = (uint64_t)(strtold(PQgetvalue(res, 0, 9), NULL));

	PQclear(res);
	return 0;
}

// fetch statitics with multipath support
int dbfetchstreaminterfacelivestatisticmpbystreaminterfaceid(PGconn **conn,
		s_interfacelivestatistic *streaminterfacelivestatistics,
		uint64_t streaminterfaceid,
		uint16_t *fetchedrows)
{
	char query[BIG_QUERY_LEN];
	PGresult *res;
	int resultrows;
	int residx, i;
	uint64_t pathnumber;

	*fetchedrows = 0;
	if (conn == NULL || streaminterfacelivestatistics == NULL)
	{
		syslog(LOG_CRIT, "dbfetchstreaminterfacelivestatisticsbystreaminterfaceid argument is NULL!");
		return 1;
	}
	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "dbfetchstreaminterfacelivestatisticbystreaminterfaceid cant reconnect DB connection");
		return 1;
	}
	
	int len = snprintf(query, MEDIUM_QUERY_LEN,
		"SELECT id, streaminterface_id, state, mode, " \
		" bitratein, bitrateout, source, destination, " \
		" acc_rtp_drops, pathnumber, pathname, "\
	 	" multipath, mploadbalance, hostclientcertificate_id,"\
		" talkbackclientcertificate_id "\
		" FROM "\
		" streaminterfacelivestatistics " \
		" WHERE streaminterface_id=%" PRIu64,
		streaminterfaceid);

	if (len == MEDIUM_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "dbfetchstreaminterfacelivestatisticsbystreaminterfaceid snprintf buf is to small(%d/%d)!",
			len, MEDIUM_QUERY_LEN);
	}

#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr,"%s\n", query);
#endif

	res = PQexec(*conn, query);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "dbfetchstreaminterfacelivestatisticbystreaminterfaceid: Answer not ok\n");
#endif
		PQclear(res);
		return 2;
	}
	
	resultrows = PQntuples(res);
	if (resultrows == 0 || resultrows > DB_MAX_NR_OF_STREAM_PATHS+1)
	{
		PQclear(res);
		return 3;
	}
	
	
	// walk through the rows
	for (i = 0; i < resultrows; i++)
	{
				
		// ====== residx and pathnumber ======
		if (PQgetisnull(res,i,9))
		{
			residx = 0;
			streaminterfacelivestatistics[0].pathnumber = 100;
		}
		else
		{
			pathnumber = atoll(PQgetvalue(res,i,9));
			switch (pathnumber)
			{
				case 100: residx = 0; break;
				case 0: residx = 1; break;
				case 1: residx = 2; break;
				case 2: residx = 3; break;
				case 3: residx = 4; break;
				case 4: residx = 5; break;
				case 5: residx = 6; break;
				case 6: residx = 7; break;
				case 7: residx = 8; break;
				case 8: residx = 9; break;
				case 9: residx = 10; break;
				default: residx = 0;
			}
			streaminterfacelivestatistics[residx].pathnumber = pathnumber;
		}

		// ====== id ======
		streaminterfacelivestatistics[residx].id = atoll(PQgetvalue(res,i,0));

		// ====== streaminterface_id ======
		if (PQgetisnull(res,i,1))
		{
			streaminterfacelivestatistics[residx].streaminterface_id = 0;
		}
		else
		{
			streaminterfacelivestatistics[residx].streaminterface_id = atoll(PQgetvalue(res,i,1));
		}

		// ====== state ======
		strncpy(streaminterfacelivestatistics[residx].state, PQgetvalue(res,i,2), DB_MAX_STATE);
		streaminterfacelivestatistics[residx].state[DB_MAX_STATE-1] = '\0';

		// ====== mode ======
		strncpy(streaminterfacelivestatistics[residx].mode, PQgetvalue(res,i,3), DB_MAX_MODE_LENGTH);
		streaminterfacelivestatistics[residx].mode[DB_MAX_MODE_LENGTH-1] = '\0';

		// ====== bitratein ======
		if (PQgetisnull(res,i,4))
		{
			streaminterfacelivestatistics[residx].bitratein = 0;
		}
		else
		{
			streaminterfacelivestatistics[residx].bitratein = (uint64_t)(strtold(PQgetvalue(res,i,4), NULL));
		}

		// ====== bitrateout ======
		if (PQgetisnull(res,i,5))
		{
			streaminterfacelivestatistics[residx].bitrateout = 0;
		}
		else
		{
			streaminterfacelivestatistics[residx].bitrateout = (uint64_t)(strtold(PQgetvalue(res,i,5), NULL));
		}

		// ====== source ======
		strncpy(streaminterfacelivestatistics[residx].source, PQgetvalue(res,i,6), DB_MAX_HOSTNAME_LENGTH);
		streaminterfacelivestatistics[residx].source[DB_MAX_HOSTNAME_LENGTH-1] = '\0';

		// ====== destination ======
		strncpy(streaminterfacelivestatistics[residx].destination, PQgetvalue(res,i,7), DB_MAX_HOSTNAME_LENGTH);
		streaminterfacelivestatistics[residx].destination[DB_MAX_HOSTNAME_LENGTH-1] = '\0';

		// ====== acc_rtp_drops ======
		if (PQgetisnull(res,i,8))
		{
			streaminterfacelivestatistics[residx].acc_rtp_drops = 0;
		}
		else
		{
			streaminterfacelivestatistics[residx].acc_rtp_drops = (uint64_t)(strtold(PQgetvalue(res,i,8), NULL));			
		}

		// ====== pathname ======
		strncpy(streaminterfacelivestatistics[residx].pathname, PQgetvalue(res,i,10), DB_MAX_NAME_LENGTH);
		streaminterfacelivestatistics[residx].pathname[DB_MAX_NAME_LENGTH-1] = '\0';

		// ====== multipath ======
		if (strcmp(PQgetvalue(res, i, 11), "t") == 0)
		{
			streaminterfacelivestatistics[residx].multipath = 1;
		}
		else
		{
			streaminterfacelivestatistics[residx].multipath = 0;
		}

		// ====== mploadbalance ======
		if (strcmp(PQgetvalue(res, i, 12), "t") == 0)
		{
			streaminterfacelivestatistics[residx].mploadbalance = 1;
		}
		else
		{
			streaminterfacelivestatistics[residx].mploadbalance = 0;
		}
		
		// ====== hostclientcertificate_id ======
		if (PQgetisnull(res,i,13))
		{
			streaminterfacelivestatistics[residx].hostclientcertificate_id = 0;			
		}
		else
		{
			streaminterfacelivestatistics[residx].hostclientcertificate_id = atoll(PQgetvalue(res,i,13));			
		}

		// ====== talkbackclientcertificate_id ======
		if (PQgetisnull(res,i,14))
		{
			streaminterfacelivestatistics[residx].talkbackclientcertificate_id = 0;			
		}
		else
		{
			streaminterfacelivestatistics[residx].talkbackclientcertificate_id = atoll(PQgetvalue(res,i,14));			
		}
	}

	PQclear(res);
	*fetchedrows = resultrows;
	return 0;
}


// will create row if it doesn't exist
int dbupdatestreaminterfacelivestatistic(PGconn **conn, s_interfacelivestatistic *streaminterfacelivestatistic)
{
	char update_query[BIG_QUERY_LEN];
	char insert_query[BIG_QUERY_LEN];
	int len;
	char *mode;
	char *dbstate;
	char *source;
	char *destination;
	char *pathname;
	char multipath[2];
	char mploadbalance[2];
	char hostclientcertificateid[128];
	char talkbackclientcertificateid[128];
	PGresult *res;

	if (conn == NULL || streaminterfacelivestatistic == NULL)
	{
		syslog(LOG_CRIT, "dbupdatestreaminterfacelivestatistic argument is NULL!");
		return 1;
	}
	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "dbupdatestreaminterfacelivestatistic cant reconnect DB connection");
		return 1;
	}
	
	mode = pqescape(streaminterfacelivestatistic->mode);
	if (mode == NULL)
	{
		return 1;
	}
	
	dbstate = pqescape(streaminterfacelivestatistic->state);
	if (dbstate == NULL)
	{
		free(mode);
		return 1;
	}
	source = pqescape(streaminterfacelivestatistic->source);
	if (source == NULL)
	{
		free(mode);
		free(dbstate);
		return 1;
	}
	destination = pqescape(streaminterfacelivestatistic->destination);
	if (destination == NULL)
	{
		free(mode);
		free(dbstate);
		free(source);
		return 1;
	}
	pathname = pqescape(streaminterfacelivestatistic->pathname);
	if (pathname == NULL)
	{
		free(mode);
		free(dbstate);
		free(source);
		free(destination);
		return 1;
	}
	
	if (streaminterfacelivestatistic->multipath)
	{
		sprintf(multipath, "t");
	}
	else
	{
		sprintf(multipath, "f");
	}
	
	if (streaminterfacelivestatistic->mploadbalance)
	{
		sprintf(mploadbalance, "t");
	}
	else
	{
		sprintf(mploadbalance, "f");
	}
	
	if (streaminterfacelivestatistic->hostclientcertificate_id)
	{
		len = snprintf(hostclientcertificateid, 128,
			"%" PRIu64, streaminterfacelivestatistic->hostclientcertificate_id);
		if (len == 128 || len < 0)
		{
			syslog(LOG_WARNING, "dbupdatestreaminterfacelivestatistic #2 snprintf buf is to small(%d/%d)!",
				len, 128);
		}
	}
	else
	{
		sprintf(hostclientcertificateid, "NULL");
	}

	if (streaminterfacelivestatistic->talkbackclientcertificate_id)
	{
		len = snprintf(talkbackclientcertificateid, 128,
			"%" PRIu64, streaminterfacelivestatistic->talkbackclientcertificate_id);
		if (len == 128 || len < 0)
		{
			syslog(LOG_WARNING, "dbupdatestreaminterfacelivestatistic #2 snprintf buf is to small(%d/%d)!",
				len, 128);
		}
	}
	else
	{
		sprintf(talkbackclientcertificateid, "NULL");
	}

	int len_update = snprintf(update_query, BIG_QUERY_LEN,
		"UPDATE streaminterfacelivestatistics SET " \
		" timestamp=now(), mode='%s', state='%s', bitratein=%" PRIu64 ", "\
		" bitrateout=%" PRIu64 ", bufferlevel=%" PRIu64 ", "\
		" rtt=%" PRIu16 ", source='%s', destination='%s', "\
		" udpdrops=%" PRIu64 ", acc_rtp_drops=%" PRIu64 ", "\
		" pathname='%s', "\
		" multipath='%s', "\
		" mploadbalance='%s', "\
		" ttl=%d, "\
		" hostclientcertificate_id=%s, "\
		" talkbackclientcertificate_id=%s"\
		" WHERE streaminterface_id=%" PRIu64 " AND pathnumber=%" PRIu64,
		mode, dbstate, streaminterfacelivestatistic->bitratein,
		streaminterfacelivestatistic->bitrateout, streaminterfacelivestatistic->bufferlevel,
		streaminterfacelivestatistic->rtt, source, destination,
		streaminterfacelivestatistic->udpdrops, streaminterfacelivestatistic->acc_rtp_drops,
		streaminterfacelivestatistic->pathname,
		multipath,
		mploadbalance,
		streaminterfacelivestatistic->ttl,
		hostclientcertificateid,
		talkbackclientcertificateid,
		streaminterfacelivestatistic->streaminterface_id,
		streaminterfacelivestatistic->pathnumber);

	if (len_update == BIG_QUERY_LEN || len_update < 0)
	{
		syslog(LOG_WARNING, "dbupdatestreaminterfacelivestatistic #1 snprintf buf is to small(%d/%d)!",
			len_update, BIG_QUERY_LEN);
	}


	int len_insert = snprintf(insert_query, BIG_QUERY_LEN,
		"INSERT INTO streaminterfacelivestatistics "\
		" (streaminterface_id, mode, state, "\
		" bitratein, bitrateout, "\
		" bufferlevel, rtt, "\
		" source, destination, udpdrops, "\
		" acc_rtp_drops, pathnumber, "\
		" pathname) "\
		" VALUES "\
		" (%" PRIu64 ", '%s', '%s', "\
		" %" PRIu64 ", %" PRIu64 ", "\
		" %" PRIu64 ", %" PRIu16 ", "\
		" '%s', '%s', %" PRIu64 ", "\
		" %" PRIu64 ", %" PRIu64 ", "\
		" '%s')",
		streaminterfacelivestatistic->streaminterface_id, mode, dbstate,
		streaminterfacelivestatistic->bitratein, streaminterfacelivestatistic->bitrateout,
		streaminterfacelivestatistic->bufferlevel, streaminterfacelivestatistic->rtt,
		source, destination, streaminterfacelivestatistic->udpdrops,
		streaminterfacelivestatistic->acc_rtp_drops, streaminterfacelivestatistic->pathnumber,
		streaminterfacelivestatistic->pathname);
	if (len_insert == BIG_QUERY_LEN || len_insert < 0)
	{
		syslog(LOG_WARNING, "dbupdatestreaminterfacelivestatistics #2 snprintf buf is to small(%d/%d)!",
				len_insert, BIG_QUERY_LEN);
	}

	free(mode);
	free(dbstate);
	free(source);
	free(destination);
	free(pathname);
	mode = dbstate = source = destination = pathname = NULL;

#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n", update_query);
#endif
	res = PQexec(*conn, update_query);
	
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
		fprintf(stderr, "dbupdatestreaminterfacelivestatistic: Answer not ok\n");
#endif
		PQclear(res);
		DBClose(*conn);
		// Assume link is down and restablish, fix to better handling
		*conn = DBConnect();
		syslog(LOG_WARNING, "dbupdatestreaminterfacelivestatistic: Cant write stats");
		return 2;
	}
	

	if (atoi(PQcmdTuples(res)) < 1)
	{
		PQclear(res);
#ifdef NODAEMON_DEBUG_DB_QUERY
		fprintf(stderr, "%s\n", insert_query);
#endif
		
		res = PQexec(*conn, insert_query);
		if (PQresultStatus(res) != PGRES_COMMAND_OK)
		{
			PQclear(res);
#ifdef NODAEMON_DEBUG_DB
			fprintf(stderr, "dbupdatestreaminterfacelivestatistic: Answer not ok\n");
#endif
			return 3;
		}
	}
	
	PQclear(res);
	return 0;
}

int dbinsertstreamstats(PGconn **conn, s_streamstats *streamstats)
{
	int retCode = 0;
	char query[BIG_QUERY_LEN];
	char *source_host;
	char *sink_host;
	char *mode;
	char multipath[8], mploadbalance[8];
	PGresult *res;

	if (conn == NULL || streamstats == NULL)
	{
		syslog(LOG_CRIT, "dbinsertstreamstats argument is NULL!");
		return 1;
	}

	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "dbinsertstreamstats cant reconnect DB connection");
		return 1;
	}
	mode = pqescape(streamstats->mode);
	if (mode == NULL)
	{
		return 1;
	}
	source_host = pqescape(streamstats->source_host);
	if (source_host == NULL)
	{
		free(mode);
		return 1;
	}
	sink_host = pqescape(streamstats->sink_host);
	if (sink_host == NULL)
	{
		free(mode);
		free(source_host);
		return 1;
	}

	if(streamstats->multipath)
	{
		sprintf(multipath, "t");
	}
	else
	{
		sprintf(multipath, "f");
	}
	
	if(streamstats->mploadbalance)
	{
		sprintf(mploadbalance, "t");
	}
	else
	{
		sprintf(mploadbalance, "f");
	}
	
	int len = snprintf(query, BIG_QUERY_LEN,
		"INSERT INTO streamstats "\
		" (streaminterface_id, timestamp, streammode, source_address, source_port, "\
		" sink_address, sink_port, drops, bitrate, bitrate_in, bitrate_out, "\
		" resendcounter, rudprtt, pathnumber, pathname, linkdrops, multipath, mploadbalance) "\
		" VALUES "\
		" (%" PRIu64 ", '%s', '%s', '%s', '%" PRIu16 "', "\
		" '%s', '%" PRIu16 "', '%" PRIu16"', '%" PRIu64"', '%" PRIu64"', '%" PRIu64"', "\
		" '%" PRIu64"', '%" PRIu16"', '%" PRIu64"', '%s', '%" PRIu64"', '%s', '%s' )",
		streamstats->streaminterface_id,
		"now()",
		mode,
		source_host, streamstats->source_port,
		sink_host, streamstats->sink_port,
		streamstats->drops,
		streamstats->bitrate,
		streamstats->bitrate_in,
		streamstats->bitrate_out,
		streamstats->resendcounter,
		streamstats->rudprtt,
		streamstats->pathnumber,
		streamstats->pathname,
		streamstats->linkdrops,
		multipath,
		mploadbalance);

	if (len == BIG_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "dbinsertstreamstats snprintf buf is to small(%d/%d)!",
			len, BIG_QUERY_LEN);
	}

	free(source_host);
	free(sink_host);
	free(mode);
	source_host = sink_host = mode = NULL;

#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n", query);
#endif
	res = PQexec(*conn, query);
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "dbinsertstreamstats: Answer not ok\n");
#endif
		retCode=2;
// ======= FALL THROUGH ========
	}

	PQclear(res);
	return retCode;
}

int dbupdatestreaminterfaceruninfo(PGconn **conn, s_interfaceruninfo *streaminterfaceruninfo)
{
	char update_query[BIG_QUERY_LEN];
	char insert_query[BIG_QUERY_LEN];
	char *startstopinfo;
	PGresult *res;

	if (conn == NULL || streaminterfaceruninfo == NULL)
	{
		syslog(LOG_CRIT, "dbupdatestreaminterfaceruninfo argument is NULL!");
		return 1;
	}
	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "dbupdatestreaminterfaceruninfo cant reconnect DB connection");
		return 1;
	}
	
	startstopinfo = pqescape(streaminterfaceruninfo->startstopinfo);
	if (startstopinfo == NULL)
	{
		return 1;
	}
	
	int update_len = snprintf(update_query, BIG_QUERY_LEN,
			"UPDATE streaminterfaceruninfos " \
			" SET lastchanged=now(), pid=0, startstopinfo='%s' " \
			" WHERE streaminterface_id=%" PRIu64,
			streaminterfaceruninfo->startstopinfo,
			streaminterfaceruninfo->streaminterface_id);
	if (update_len == BIG_QUERY_LEN || update_len < 0)
	{
		syslog(LOG_WARNING, "dbupdatestreaminterfaceruninfo #1 snprintf buf is to small(%d/%d)!",
				update_len, BIG_QUERY_LEN);
	}

	int insert_len = snprintf(insert_query, BIG_QUERY_LEN,
		"INSERT INTO streaminterfaceruninfos "\
		" (streaminterface_id, lastchanged, pid, startstopinfo) "\
		" VALUES "\
		" (%" PRIu64 ", now(), 0, '%s')",
		streaminterfaceruninfo->streaminterface_id, startstopinfo );
	if (insert_len == BIG_QUERY_LEN || insert_len < 0)
	{
		syslog(LOG_WARNING, "dbupdatestreaminterfaceruninfo #2 snprintf buf is to small(%d/%d)!",
				insert_len, BIG_QUERY_LEN);
	}

	free(startstopinfo);
	startstopinfo=NULL;

#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n", update_query);
#endif
	res = PQexec(*conn, update_query);

	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "dbupdatestreaminterfaceruninfo: Answer not ok\n");
#endif
		PQclear(res);
		return 2;
	}
	
	if (atoi(PQcmdTuples(res)) < 1)
	{
#ifdef NODAEMON_DEBUG_DB_QUERY
		fprintf(stderr, "%s\n", insert_query);
#endif
		res = PQexec(*conn, insert_query);
		if (PQresultStatus(res) != PGRES_COMMAND_OK)
		{
#ifdef NODAEMON_DEBUG_DB
			fprintf(stderr, "dbupdatestreaminterfaceruninfo: Answer not ok\n");
#endif
			PQclear(res);
			return 3;
		}
	}
	
	PQclear(res);
	return 0;
}


int dbaddstreaminterfacelog(PGconn **conn, s_interfacelog *streaminterfacelog)
{
	char query[DOUBLE_BIG_QUERY_LEN];
	char *text;
	
	PGresult *res;
	if (conn == NULL ||  streaminterfacelog == NULL)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "dbaddstreaminterfacelog: Argument is null!\n");
#endif
		syslog(LOG_CRIT, "dbaddstreaminterfacelog: parameter is null!");	
		return 1;
	}
	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "dbaddstreaminterfacelog: cant reconnect DB connection");
		return 1;
	}

	text = pqescape(streaminterfacelog->text);
	if (text == NULL)
	{
		return 2;
	}
	int len = snprintf(query, DOUBLE_BIG_QUERY_LEN,
			"INSERT INTO streaminterfacelogs "\
			" (streaminterface_id, level, text) "\
			" VALUES "\
			" (%" PRIu64",%d,'%s')",
			streaminterfacelog->streaminterface_id, streaminterfacelog->level, text);
	if (len == DOUBLE_BIG_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "dbaddstreaminterfacelog snprintf buf is to small(%d/%d)!", len, DOUBLE_BIG_QUERY_LEN);
	}

	free(text);
	text = NULL;

#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n", query);
#endif
	res = PQexec(*conn, query);

	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "dbaddstreaminterfacelog: Answer not ok: %s\n", PQerrorMessage(*conn));
#endif
		PQclear(res);
		return 2;
	}
	
	PQclear(res);
	return 0;
}

int dbfetch_interfacelog(PGconn **conn, uint64_t streaminterface_id, s_interfacelog *streaminterfacelog, uint16_t loglen, uint16_t *fetched_rows)
{
	char query[MEDIUM_QUERY_LEN];
	PGresult *res;

	if (conn == NULL || streaminterfacelog == NULL || loglen != 10)
	{
		syslog(LOG_CRIT, "dbfetch_interfacelog argument is NULL!");
		return 1;
	}
	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "dbfetch_interfacelog cant reconnect DB connection");
		return 1;
	}

	int len = snprintf(query, MEDIUM_QUERY_LEN,
		"SELECT timestamp, text " \
		" FROM streaminterfacelogs "\
		" WHERE streaminterface_id=%"PRIu64" "\
		" ORDER BY timestamp DESC LIMIT 10",
		streaminterface_id);

	if (len == MEDIUM_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "dbfetch_interfacelog snprintf buf is to small(%d/%d)!",
			len, MEDIUM_QUERY_LEN);
	}

#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr,"%s\n", query);
#endif

	res = PQexec(*conn, query);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "dbfetch_interfacelog: Answer not ok\n");
#endif
		PQclear(res);
		return 2;
	}
	
	int resultrows, i;
	resultrows = PQntuples(res);
	if (resultrows == 0)
	{
		PQclear(res);
		return 3;
	}
	
	// walk through the rows
	for (i = 0; i < resultrows && i < 10; i++)
	{
		// ====== timestamp ======
		strncpy(streaminterfacelog[i].timestamp, PQgetvalue(res, i, 0), DB_MAX_TIMESTAMP);

		// ====== text ======
		strncpy(streaminterfacelog[i].text, PQgetvalue(res, i, 1), DB_MAX_LOGTEXT);
	}
	*fetched_rows = resultrows;

	PQclear(res);
	return 0;
}


int dbfetch_rootcertificates(PGconn **conn, uint64_t id, s_rootcertificates *rootcertificates)
{
	char query[MEDIUM_QUERY_LEN];
	PGresult *res;

	if (conn == NULL || rootcertificates == NULL)
	{
		syslog(LOG_CRIT, "dbfetch_rootcertificates argument is NULL!");
		return 1;
	}
	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "dbfetch_rootcertificates cant reconnect DB connection");
		return 1;
	}

	int len = snprintf(query, MEDIUM_QUERY_LEN,
		"SELECT id, created, expiration_days, " \
		" organisation_id, name, public_part, private_part "\
		" FROM rootcertificates "\
		" WHERE id=%" PRIu64,
		id);
	if (len == MEDIUM_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "dbfetch_rootcertificates snprintf buf is too small(%d/%d)!",
			len, MEDIUM_QUERY_LEN);
	}

#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n", query);
#endif

	res = PQexec(*conn, query);

	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "dbfetch_rootcertificates: Answer not ok\n");
#endif
		PQclear(res);
		return 2;
	}

	if (PQntuples(res) != 1)
	{
		PQclear(res);
		return 3;
	}

	if (PQgetisnull(res, 0, 0))
	{
		rootcertificates->id = 0;
		// Something is wrong - can this happen?
	}
	rootcertificates->id = atoll(PQgetvalue(res, 0, 0));

	strncpy(rootcertificates->created, PQgetvalue(res, 0, 1), DB_MAX_TIMESTAMP);
	rootcertificates->created[DB_MAX_TIMESTAMP-1] = '\0';

	if (PQgetisnull(res,0,2))
	{
		rootcertificates->expiration_days = 0;
	}
	else
	{
		rootcertificates->expiration_days = atoll(PQgetvalue(res,0,2));
	}
	if (PQgetisnull(res,0,3))
	{
		rootcertificates->organisation_id = 0;
	}
	else
	{
		rootcertificates->organisation_id = atoll(PQgetvalue(res,0,3));
	}
	strncpy(rootcertificates->name, PQgetvalue(res,0,4), DB_MAX_NAME_LENGTH);
	rootcertificates->name[DB_MAX_NAME_LENGTH-1] = '\0';

	strncpy(rootcertificates->public_part, PQgetvalue(res,0,5), DB_MAX_CERTIFICATE_LEN);
	rootcertificates->public_part[DB_MAX_CERTIFICATE_LEN-1] = '\0';

	strncpy(rootcertificates->private_part, PQgetvalue(res,0,6), DB_MAX_CERTIFICATE_LEN);
	rootcertificates->private_part[DB_MAX_CERTIFICATE_LEN-1] = '\0';

	PQclear(res);
	return 0;
}

int dbfetch_clientcertificates(PGconn **conn, uint64_t id, s_clientcertificates *clientcertificates)
{
	char query[MEDIUM_QUERY_LEN];
	PGresult *res;

	if (conn == NULL || clientcertificates == NULL)
	{
		syslog(LOG_CRIT, "dbfetch_clientcertificates argument is NULL!");
		return 1;
	}
	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "dbfetch_clientcertificates cant reconnect DB connection");
		return 1;
	}

	int len = snprintf(query, MEDIUM_QUERY_LEN,
		"SELECT id, created, expiration_days, " \
		" rootcertificate_id, name, public_part, private_part "\
		" FROM clientcertificates "\
		" WHERE id=%" PRIu64,
		id);
	if (len == MEDIUM_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "dbfetch_clientcertificates snprintf buf is too small(%d/%d)!", len, MEDIUM_QUERY_LEN);
	}

#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n", query);
#endif

	res = PQexec(*conn, query);

	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "dbfetch_clientcertificates: Answer not ok\n");
#endif
		PQclear(res);
		return 2;
	}

	if (PQntuples(res) != 1)
	{
		PQclear(res);
		return 3;
	}

	if (PQgetisnull(res, 0, 0))
	{
		clientcertificates->id = 0;
		// Something is wrong - can this happen?
	}
	clientcertificates->id = atoll(PQgetvalue(res,0,0));

	strncpy(clientcertificates->created, PQgetvalue(res,0,1), DB_MAX_TIMESTAMP);
	clientcertificates->created[DB_MAX_TIMESTAMP-1] = '\0';

	if (PQgetisnull(res,0,2))
	{
		clientcertificates->expiration_days = 0;
	}
	else
	{
		clientcertificates->expiration_days = atoll(PQgetvalue(res,0,2));
	}
	if (PQgetisnull(res,0,3))
	{
		clientcertificates->rootcertificate_id = 0;
	}
	else
	{
		clientcertificates->rootcertificate_id = atoll(PQgetvalue(res,0,3));
	}

	strncpy(clientcertificates->name, PQgetvalue(res,0,4), DB_MAX_NAME_LENGTH);
	clientcertificates->name[DB_MAX_NAME_LENGTH-1] = '\0';

	strncpy(clientcertificates->public_part, PQgetvalue(res,0,5), DB_MAX_CERTIFICATE_LEN);
	clientcertificates->public_part[DB_MAX_CERTIFICATE_LEN-1] = '\0';

	strncpy(clientcertificates->private_part, PQgetvalue(res,0,6), DB_MAX_CERTIFICATE_LEN);
	clientcertificates->private_part[DB_MAX_CERTIFICATE_LEN-1] = '\0';

	PQclear(res);
	return 0;
}

int db_delete_streaminterfacelivestatistic(PGconn **conn, uint64_t streaminterfaceid)
{
	char query[SMALL_QUERY_LEN];
	PGresult *res;

	if (conn == NULL)
	{
		syslog(LOG_CRIT, "db_delete_streaminterfacelivestatistic argument is NULL!");
		return 1;
	}
	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "db_delete_streaminterfacelivestatistic cant reconnect DB connection");
		return 1;
	}

	int len = snprintf(query, SMALL_QUERY_LEN,
			"DELETE FROM streaminterfacelivestatistics "\
			" WHERE streaminterface_id=%" PRIu64,
			streaminterfaceid);

	if (len == SMALL_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "db_delete_streaminterfacelivestatistic snprintf buf is to small(%d/%d)!",
			len, SMALL_QUERY_LEN);
	}

#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr,"%s\n", query);
#endif

	res = PQexec(*conn, query);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "db_delete_streaminterfacelivestatistic: Answer not ok\n");
#endif
		PQclear(res);
		return 2;
	}

	PQclear(res);
	return 0;
}
