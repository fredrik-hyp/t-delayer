#include "dbdelayers.h"

int db_fetch_delayercoms(PGconn **conn, s_delayercoms *delayercom, int maxsize, int *retvalues, uint64_t streaminterface_id)
{
	char query[MEDIUM_QUERY_LEN];
	PGresult *res;
	int i,numtuples;
	
	if (conn == NULL || delayercom == NULL)
	{
		syslog(LOG_CRIT, "db_fetch_delayercoms argument is NULL!");
		return 1;
	}

	// Set everything in the struct to 0
	memset(delayercom, 0, maxsize*sizeof(struct _delayercoms));

	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "db_fetch_delayercoms cant reconnect DB connection");
		return 1;
	}

	int len = snprintf(query, MEDIUM_QUERY_LEN,
		"SELECT id, extract(epoch from timestamp), " \
		" streaminterface_id, pathnumber, action, " \
		" jumptopos, extract(epoch from jumptopos), "\
		" startplayat, extract(epoch from startplayat), "\
		" playto, extract(epoch from playto) "\
		" FROM delayercoms "\
		" WHERE streaminterface_id=%" PRIu64 " ORDER BY ID LIMIT %d",
		streaminterface_id, maxsize);
	if (len == MEDIUM_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "db_fetch_delayercoms snprintf buf is too small(%d/%d)!", len, MEDIUM_QUERY_LEN);
	}

#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n", query);
#endif

	res = PQexec(*conn, query);

	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "db_fetch_delayercoms: Answer not ok\n");
#endif
		PQclear(res);
		return 2;
	}

	if (PQntuples(res) != 1)
	{
		PQclear(res);
		return 3;
	}

	if (PQgetisnull(res, 0, 0) || PQgetisnull(res, 0, 1))
	{
		PQclear(res);
		return 4;
	}

#ifdef NODAEMON_DEBUG_DB
	fprintf(stderr, "size: %zd\n\n", sizeof(struct _delayercoms));
#endif

	numtuples = PQntuples(res);
	for(i=0;i<numtuples; i++)
	{
		delayercom[i].id = atoll(PQgetvalue(res, i, 0));
		if (PQgetisnull(res, i, 1))
		{
			delayercom[i].create_timestamp = 0;
		}
		else if (strtold(PQgetvalue(res, i, 1), NULL) < 0)
		{
			syslog(LOG_CRIT, "db_fetch_delayercoms: create timestamp is preEPOCH");
			delayercom[i].create_timestamp = 0;
		}
		else
		{
			delayercom[i].create_timestamp = (uint64_t)(strtold(PQgetvalue(res, i, 1), NULL)*1000000);
		}
		delayercom[i].streaminterface_id = atoll(PQgetvalue(res, i, 2));
		delayercom[i].pathnumber = atoll(PQgetvalue(res, i, 3));

		strncpy(delayercom[i].action, PQgetvalue(res, i, 4), DB_MAX_MODE_LENGTH);
		if (strncmp(delayercom[i].action, "jump", 4) == 0)
		{
			delayercom[i].e_action = JUMP;
		}
		else if (strncmp(delayercom[i].action, "playto", 6) == 0)
		{
			delayercom[i].e_action = PLAYTO;
		}
		else if (strncmp(delayercom[i].action, "play", 4) == 0)
		{
			delayercom[i].e_action = PLAY;
		}
		else if (strncmp(delayercom[i].action, "pause", 5) == 0)
		{
			delayercom[i].e_action = PAUSE;
		}
		else
		{
			syslog(LOG_CRIT, "db_fetch_delayercoms: unknown action %s", delayercom[i].action);
			PQclear(res);
			return 5;
		}
	
		strncpy(delayercom[i].jumptopos, PQgetvalue(res, i, 5), DB_MAX_TIMESTAMP);
		if (PQgetisnull(res, i, 6))
		{
			delayercom[i].jumptopos_timestamp = 0 ;
		}
		else if (strtold(PQgetvalue(res, i, 6), NULL) < 0)
		{
			syslog(LOG_CRIT, "db_fetch_delayercoms: jumptopos is preEPOCH");
			delayercom[i].jumptopos_timestamp = 0;
		}
		else
		{
			delayercom[i].jumptopos_timestamp = (uint64_t)(strtold(PQgetvalue(res, i, 6), NULL)*1000000);
		}

		strncpy(delayercom[i].startplayat, PQgetvalue(res, i, 7), DB_MAX_TIMESTAMP);
		if (PQgetisnull(res, i, 8))
		{
			delayercom[i].startplayat_timestamp = 0;
		}
		else if (strtold(PQgetvalue(res, i, 8), NULL) < 0)
		{
			syslog(LOG_CRIT, "db_fetch_delayercoms: startplayat is preEPOCH");
			delayercom[i].startplayat_timestamp = 0;
		}
		else
		{
			delayercom[i].startplayat_timestamp = (uint64_t)(strtold(PQgetvalue(res, i, 8), NULL)*1000000);
		}

		strncpy(delayercom[i].playto, PQgetvalue(res, i, 9), DB_MAX_TIMESTAMP);
		if (PQgetisnull(res, i, 10))
		{
			delayercom[i].playto_timestamp = 0;
		}
		else if (strtold(PQgetvalue(res, i, 10), NULL) < 0)
		{
			syslog(LOG_CRIT, "db_fetch_delayercoms: playto is preEPOCH");
			delayercom[i].playto_timestamp = 0;
		}
		else
		{
			delayercom[i].playto_timestamp = (uint64_t)(strtold(PQgetvalue(res, i, 10), NULL)*1000000);
		}
	}
	*retvalues = numtuples;
	PQclear(res);
	return 0;
}

int db_delete_delayercoms(PGconn **conn, uint64_t streaminterface_id)
{
	char query[MEDIUM_QUERY_LEN];
	PGresult *res;

	if (conn == NULL)
	{
		syslog(LOG_CRIT, "db_delete_delayercoms argument is NULL!");
		return 1;
	}

	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "db_delete_delayercoms cant reconnect DB connection");
		return 1;
	}

	int len = snprintf(query, MEDIUM_QUERY_LEN,
		"DELETE FROM delayercoms "\
		" WHERE streaminterface_id=%" PRIu64,
		streaminterface_id);
	if (len == MEDIUM_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "db_delete_delayercoms snprintf buf is too small(%d/%d)!", len, MEDIUM_QUERY_LEN);
	}

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
		syslog(LOG_WARNING, "Could not delete from delayercoms!");
		PQclear(res);
		return 2;
	}

	PQclear(res);
	return 0;
}

int db_delete_delayercomsbyid(PGconn **conn, uint64_t id)
{
	char query[MEDIUM_QUERY_LEN];
	PGresult *res;

	if (conn == NULL)
	{
		syslog(LOG_CRIT, "db_delete_delayercoms argument is NULL!");
		return 1;
	}

	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "db_delete_delayercoms cant reconnect DB connection");
		return 1;
	}

	int len = snprintf(query, MEDIUM_QUERY_LEN,
		"DELETE FROM delayercoms "\
		" WHERE id=%" PRIu64,
		id);
	if (len == MEDIUM_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "db_delete_delayercoms snprintf buf is too small(%d/%d)!", len, MEDIUM_QUERY_LEN);
	}

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
		syslog(LOG_WARNING, "Could not delete from delayercoms!");
		PQclear(res);
		return 2;
	}

	PQclear(res);
	return 0;
}

int db_insert_delayercom(PGconn **conn, s_delayercoms *delayercom)
{
	char *action, *startplayat, *jumptopos, *playto;
	char startplayatq[512], jumptoposq[512], playtoq[512], create_timestampq[512];
	PGresult *res;
	char insert_query[BIG_QUERY_LEN];
	int insert_len;

	memset(startplayatq, 0, 512);
	memset(jumptoposq, 0, 512);
	memset(playtoq, 0, 512);
	memset(create_timestampq, 0, 512);
	
	if (conn == NULL || delayercom == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!", __FUNCTION__);
		return 4;
	}

	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection", __FUNCTION__);
		return 5;
	}

	action = pqescape(delayercom->action);
	if (action == NULL)
	{
		return 10;
	}


	if (delayercom->create_timestamp == 0)
	{
		snprintf(create_timestampq, 511, "now()");
	}
	else
	{
		snprintf(create_timestampq, 511, "to_timestamp(%"PRIu64")",
			delayercom->create_timestamp);
	}

	if (strlen(delayercom->startplayat) == 0)
	{
		startplayat = pqescape("NULL");
		if (startplayat != NULL)	snprintf(startplayatq,511, "%s", startplayat);
	}
	else
	{
		startplayat = pqescape(delayercom->startplayat);
		if (startplayat != NULL ) snprintf(startplayatq,511, "to_timestamp(%s)", startplayat);
	}

	if (startplayat == NULL)
	{
		free(action);
		return 11;
	}
	
	if (strlen(delayercom->jumptopos) == 0)
	{
		jumptopos = pqescape("NULL");
		if (jumptopos != NULL ) snprintf(jumptoposq,511, "%s", jumptopos);
	}
	else
	{
		jumptopos = pqescape(delayercom->jumptopos);
		if (jumptopos != NULL ) snprintf(jumptoposq,511, "to_timestamp(%s)", jumptopos);
	}
	if (jumptopos == NULL)
	{
		free(startplayat);
		free(action);
		return 12;
	}

	if (strlen(delayercom->playto) == 0)
	{
		playto = pqescape("NULL");
		if (playto != NULL) snprintf(playtoq,511, "%s", playto);
	}
	else
	{
		playto = pqescape(delayercom->playto);
		if (playto != NULL) snprintf(playtoq,511, "to_timestamp(%s)", playto);
	}
	if (playto == NULL)
	{
		free(jumptopos);
		free(startplayat);
		free(action);
		return 13;
	}

	insert_len = snprintf(insert_query, BIG_QUERY_LEN,
		"INSERT INTO delayercoms "\
		" (timestamp, streaminterface_id, pathnumber, action, startplayat, jumptopos, playto)"\
		" VALUES "\
		" (%s, %" PRIu64 ", %" PRIu64 ", '%s', %s, %s, %s)",
		create_timestampq,
		delayercom->streaminterface_id,
		delayercom->pathnumber,
		action,
		startplayatq,
		jumptoposq,
		playtoq);

	if (insert_len == BIG_QUERY_LEN || insert_len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is to small(%d/%d)!",
			__FUNCTION__, insert_len, BIG_QUERY_LEN);
	}


	free(playto);
	free(startplayat);
	free(jumptopos);
	
#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n", insert_query);
#endif
	res = PQexec(*conn, insert_query);
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "%s: Answer not ok\n", __FUNCTION__);
#endif
		PQclear(res);
		return 2;
	}

	if (atoi(PQcmdTuples(res)) < 1)
	{
#ifdef NODAEMON_DEBUG_DB
			fprintf(stderr, "%s: Inserted nothing.\n", __FUNCTION__);
#endif
			PQclear(res);
			return 3;
	}
	
	PQclear(res);
	return 0;
}

int db_count_delayercoms(PGconn **conn, uint64_t streaminterfacenr)
{
	uint64_t result = 0;
	int  ret=0;
	PGresult *res;
	char query[MEDIUM_QUERY_LEN];

	memset(query, 0, MEDIUM_QUERY_LEN);
	if (conn == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!", __FUNCTION__);
		return -1;
	}

	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection", __FUNCTION__);
		return -2;
	}
	
	ret = snprintf(query, MEDIUM_QUERY_LEN, "select count(*) from delayercoms where streaminterface_id=%"PRIu64, streaminterfacenr);
	if (ret == MEDIUM_QUERY_LEN || ret < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is to small(%d/%d)!",
			__FUNCTION__, ret, MEDIUM_QUERY_LEN);		
	}
	
	res = PQexec(*conn, query);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "%s: Answer not ok\n", __FUNCTION__);
#endif
		PQclear(res);
		return -3;
	}
	
	result = atoll(PQgetvalue(res, 0, 0));
	PQclear(res);
	return (int) result;
}

int db_insert_delayerstats(PGconn **conn, s_delayerstats *delayerstats)
{
	char *state;
	PGresult *res;
	char insert_query[BIG_QUERY_LEN];
	char update_query[BIG_QUERY_LEN];
	int insert_len, update_len;


	if (conn == NULL || delayerstats == NULL)
	{
		syslog(LOG_CRIT, "db_insert_delayerstats argument is NULL!");
		return 1;
	}

	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "db_insert_delayerstats cant reconnect DB connection");
		return 1;
	}
	state = pqescape(delayerstats->state);
	if (state == NULL)
	{
		return 1;
	}

	char startplayat_i[DB_MAX_MODE_LENGTH], startplayat_u[DB_MAX_MODE_LENGTH],
		startpos_i[DB_MAX_MODE_LENGTH], startpos_u[DB_MAX_MODE_LENGTH],
		lastpos_i[DB_MAX_MODE_LENGTH], lastpos_u[DB_MAX_MODE_LENGTH],
		currpos_i[DB_MAX_MODE_LENGTH], currpos_u[DB_MAX_MODE_LENGTH],
		playto_i[DB_MAX_MODE_LENGTH], playto_u[DB_MAX_MODE_LENGTH];

	if (delayerstats->startplayat_timestamp == 0)
	{
		snprintf(startplayat_i, DB_MAX_MODE_LENGTH, "NULL");
		snprintf(startplayat_u, DB_MAX_MODE_LENGTH, "startplayat=NULL");	
	}
	else
	{
		snprintf(startplayat_i, DB_MAX_MODE_LENGTH, " to_timestamp(%"PRIu64")",
			delayerstats->startplayat_timestamp/1000000);
		snprintf(startplayat_u, DB_MAX_MODE_LENGTH, " startplayat=to_timestamp(%"PRIu64")",
			delayerstats->startplayat_timestamp/1000000);
	}

	if (delayerstats->startpos_timestamp == 0)
	{
		snprintf(startpos_i, DB_MAX_MODE_LENGTH, "NULL");
		snprintf(startpos_u, DB_MAX_MODE_LENGTH, "startpos=NULL");	
	}
	else
	{
		snprintf(startpos_i, DB_MAX_MODE_LENGTH, " to_timestamp(%"PRIu64")",
			delayerstats->startpos_timestamp/1000000);
		snprintf(startpos_u, DB_MAX_MODE_LENGTH, " startpos=to_timestamp(%"PRIu64")",
			delayerstats->startpos_timestamp/1000000);
	}

	if (delayerstats->lastpos_timestamp == 0)
	{
		snprintf(lastpos_i, DB_MAX_MODE_LENGTH, "NULL");
		snprintf(lastpos_u, DB_MAX_MODE_LENGTH, "lastpos=NULL");	
	}
	else
	{
		snprintf(lastpos_i, DB_MAX_MODE_LENGTH, " to_timestamp(%"PRIu64")",
			delayerstats->lastpos_timestamp/1000000);
		snprintf(lastpos_u, DB_MAX_MODE_LENGTH, " lastpos=to_timestamp(%"PRIu64")",
			delayerstats->lastpos_timestamp/1000000);
	}

	if (delayerstats->currpos_timestamp == 0)
	{
		snprintf(currpos_i, DB_MAX_MODE_LENGTH, "NULL");
		snprintf(currpos_u, DB_MAX_MODE_LENGTH, "currpos=NULL");	
	}
	else
	{
		snprintf(currpos_i, DB_MAX_MODE_LENGTH, " to_timestamp(%"PRIu64")",
			delayerstats->currpos_timestamp/1000000);
		snprintf(currpos_u, DB_MAX_MODE_LENGTH, " currpos=to_timestamp(%"PRIu64")",
			delayerstats->currpos_timestamp/1000000);
	}

	if (delayerstats->playto_timestamp == 0)
	{
		snprintf(playto_i, DB_MAX_MODE_LENGTH, "NULL");
		snprintf(playto_u, DB_MAX_MODE_LENGTH, "playto=NULL");	
	}
	else
	{
		snprintf(playto_i, DB_MAX_MODE_LENGTH, " to_timestamp(%"PRIu64")",
			delayerstats->playto_timestamp/1000000);
		snprintf(playto_u, DB_MAX_MODE_LENGTH, " playto=to_timestamp(%"PRIu64")",
			delayerstats->playto_timestamp/1000000);
	}

	insert_len = snprintf(insert_query, BIG_QUERY_LEN,
		"INSERT INTO delayerstats "\
		" (streaminterface_id, pathnumber, state, startplayat, startpos, "\
		" lastpos, currpos, playto) "\
		" VALUES "\
		" (%" PRIu64 ", %" PRIu64", '%s', "\
		" %s, %s, "\
		" %s, %s, "\
		" %s )",
		delayerstats->streaminterface_id,
		delayerstats->pathnumber,
		state,
		startplayat_i,
		startpos_i,
		lastpos_i,
		currpos_i,
		playto_i);

	if (insert_len == BIG_QUERY_LEN || insert_len < 0)
	{
		syslog(LOG_WARNING, "db_insert_delayerstats snprintf buf is to small(%d/%d)!",
			insert_len, BIG_QUERY_LEN);
	}
	update_len = snprintf(update_query, BIG_QUERY_LEN,
		"UPDATE delayerstats SET "\
		" state='%s', %s, "\
		" %s, %s, "\
		" %s, %s "\
		" WHERE "\
		" streaminterface_id=%" PRIu64 " AND pathnumber = %" PRIu64,
		state,
		startplayat_u,
		startpos_u,
		lastpos_u,
		currpos_u,
		playto_u,
		delayerstats->streaminterface_id,
		delayerstats->pathnumber);

		//delayerstats->startpos_timestamp/1000000,
	if (update_len == BIG_QUERY_LEN || update_len < 0)
	{
		syslog(LOG_WARNING, "db_insert_delayerstats snprintf buf is to small(%d/%d)!",
			update_len, BIG_QUERY_LEN);
	}

	free(state);
	state = NULL;

#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n", update_query);
#endif
	res = PQexec(*conn, update_query);
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "db_insert_delayerstats: Answer not ok\n");
#endif
		PQclear(res);
		return 2;
	}

	if (atoi(PQcmdTuples(res)) < 1)
	{
#ifdef NODAEMON_DEBUG_DB_QUERY
		fprintf(stderr, "%s\n", insert_query);
#endif
		PQclear(res);
		res = PQexec(*conn, insert_query);
		if (PQresultStatus(res) != PGRES_COMMAND_OK)
		{
#ifdef NODAEMON_DEBUG_DB
			fprintf(stderr, "db_insert_delayerstats: Answer not ok\n");
#endif
			PQclear(res);
			return 3;
		}
	}
	
	PQclear(res);
	return 0;
}

int db_fetch_delayerstatsbystreaminterfaceid(PGconn **conn, s_delayerstats *delayerstats,  int maxsize, int *retvalues, uint64_t streaminterface_id)
{
	char query[MEDIUM_QUERY_LEN];
	PGresult *res;
	int i,numtuples;
	
	if(conn == NULL || delayerstats == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!", __FUNCTION__);
		return 1;
	}
	
	if(check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection.", __FUNCTION__);
		return 1;
	}
	
	
	int len = snprintf(query, MEDIUM_QUERY_LEN, "SELECT id, "\
						" streaminterface_id, state, "\
						" startplayat, extract(epoch from startplayat), "\
						" startpos, extract(epoch from startpos), "\
						" lastpos, extract(epoch from lastpos), "\
						" currpos, extract(epoch from currpos), "\
						" playto, extract(epoch from playto), pathnumber "\
						" FROM delayerstats WHERE "\
						" streaminterface_id=%" PRIu64 " ORDER BY pathnumber LIMIT %d",
						streaminterface_id, maxsize);
	
	if (len == MEDIUM_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is too small(%d/%d)!", __FUNCTION__, len, MEDIUM_QUERY_LEN);
	}

#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n", query);
#endif

	res = PQexec(*conn, query);

	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "%s: Answer not ok\n", __FUNCTION__);
#endif
		PQclear(res);
		return 2;
	}

#ifdef NODAEMON_DEBUG_DB
	fprintf(stderr, "size: %zd\n\n", sizeof(struct _delayerstats));
#endif

	int error = 0;
	numtuples = PQntuples(res);
	for (i = 0; i < numtuples; i++)
	{
		if (PQgetisnull(res, i, 0))
		{
			error = 3;
			break;
		}
		delayerstats[i].id = atoll(PQgetvalue(res, i, 0));

		if (PQgetisnull(res, i, 1))
		{
			error = 4;
			break;
		}
		delayerstats[i].streaminterface_id = atoll(PQgetvalue(res, i, 1));

		strncpy(delayerstats[i].state, PQgetvalue(res, i, 2), DB_MAX_MODE_LENGTH);
		/*
		uint64_t startplayat_timestamp;
		uint64_t startpos_timestamp;
		uint64_t lastpos_timestamp;
		uint64_t currpos_timestamp;
		uint64_t playto_timestamp;
		*/
		if (PQgetisnull(res, i, 4))
		{
			delayerstats[i].startplayat_timestamp = 0 ;
		}
		else if (strtold(PQgetvalue(res, i, 4), NULL) < 0)
		{
			syslog(LOG_CRIT, "%s: startplayat is preEPOCH", __FUNCTION__);
			delayerstats[i].startplayat_timestamp = 0;
		}
		else
		{
			delayerstats[i].startplayat_timestamp = (uint64_t)(strtold(PQgetvalue(res, i, 4), NULL));
		}

		if (PQgetisnull(res, i, 6))
		{
			delayerstats[i].startpos_timestamp = 0;
		}
		else if (strtold(PQgetvalue(res, i, 6), NULL) < 0)
		{
			syslog(LOG_CRIT, "%s: startpos is preEPOCH", __FUNCTION__);
			delayerstats[i].startpos_timestamp = 0;
		}
		else
		{
			delayerstats[i].startpos_timestamp = (uint64_t)(strtold(PQgetvalue(res, i, 6), NULL));
		}

		if (PQgetisnull(res, i, 8))
		{
			delayerstats[i].lastpos_timestamp = 0;
		}
		else if (strtold(PQgetvalue(res, i, 8), NULL) < 0)
		{
			syslog(LOG_CRIT, "%s: lastpos is preEPOCH", __FUNCTION__);
			delayerstats[i].lastpos_timestamp = 0;
		}
		else
		{
			delayerstats[i].lastpos_timestamp = (uint64_t)(strtold(PQgetvalue(res, i, 8), NULL));
		}

		if (PQgetisnull(res, i, 10))
		{
			delayerstats[i].currpos_timestamp = 0;
		}
		else if (strtold(PQgetvalue(res, i, 10), NULL) < 0)
		{
			syslog(LOG_CRIT, "%s: currpos is preEPOCH", __FUNCTION__);
			delayerstats[i].currpos_timestamp = 0;
		}
		else
		{
			delayerstats[i].currpos_timestamp = (uint64_t)(strtold(PQgetvalue(res, i, 10), NULL));
		}

		if (PQgetisnull(res, 0, 12))
		{
			delayerstats[i].playto_timestamp = 0;
		}
		else if (strtold(PQgetvalue(res, i, 12), NULL) < 0)
		{
			syslog(LOG_CRIT, "%s: playto is preEPOCH", __FUNCTION__);
			delayerstats[i].playto_timestamp = 0;
		}
		else
		{
			delayerstats[i].playto_timestamp = (uint64_t)(strtold(PQgetvalue(res, i, 12), NULL));
		}
		
		if (PQgetisnull(res, 0, 13))
		{
			delayerstats[i].pathnumber = 0;
		}
		else
		{
			delayerstats[i].pathnumber = (uint64_t)(strtold(PQgetvalue(res, i, 13), NULL));
		}
	}
	PQclear(res);
	if (error)
	{
		*retvalues = 0;
		return error;
	}

	*retvalues = numtuples;
	return 0;
}

int db_delete_delayerstats(PGconn **conn, uint64_t streaminterface_id)
{
	char query[MEDIUM_QUERY_LEN];
	PGresult *res;

	if (conn == NULL)
	{
		syslog(LOG_CRIT, "db_delete_delayerstats argument is NULL!");
		return 1;
	}

	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "db_delete_delayerstats cant reconnect DB connection");
		return 1;
	}

	int len = snprintf(query, MEDIUM_QUERY_LEN,
		"DELETE FROM delayerstats "\
		" WHERE streaminterface_id=%" PRIu64,
		streaminterface_id);
	if (len == MEDIUM_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "db_delete_delayerstats snprintf buf is too small(%d/%d)!",
			len, MEDIUM_QUERY_LEN);
	}

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
		syslog(LOG_WARNING, "Could not delete from delayerstats!");
		PQclear(res);
		return 2;
	}

	PQclear(res);
	return 0;
}
