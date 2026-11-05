#include "dbrecplay.h"

// ===== Recordings ======
int db_fetch_recordings(PGconn **conn, s_recordings *recording, int maxsize, int *retvalues, uint64_t streaminterface_id, e_recorder_state req_state)
{
	char query[MEDIUM_QUERY_LEN];
	char req_state_str[DB_MAX_STATE];
	PGresult *res;
	int i, numtuples;
	
	*retvalues = 0;
	if (conn == NULL || recording == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!", __FUNCTION__);
		return 1;
	}

	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection.",
			__FUNCTION__);
		return 2;
	}

	// Set everything in the struct to 0
	memset(recording, 0, maxsize*sizeof(struct _recordings));
	memset(query, 0, MEDIUM_QUERY_LEN);
	memset(req_state_str, 0, DB_MAX_STATE);

	switch (req_state)
	{
		case INIT:
			strncpy(req_state_str, "init", DB_MAX_STATE);
			break;
		case RECORDING:
			strncpy(req_state_str, "recording", DB_MAX_STATE);
			break;
		case DONE:
			strncpy(req_state_str, "done", DB_MAX_STATE);
			break;
		case PAUSED:
			strncpy(req_state_str, "paused", DB_MAX_STATE);
			break;
		default:
			return 3;
	}

	int len;
	if (streaminterface_id == 0)
	{
		// If no streaminterface are given its a
		// general query after a list of recordings
		len = snprintf(query, MEDIUM_QUERY_LEN,
			"SELECT id, name, " \
			" extract(epoch from recordingstart), " \
			" extract(epoch from recordingend), "\
			" sourceaddress, sourceport "\
			" FROM recordings "\
			" WHERE state='%s' "\
			" ORDER BY ID LIMIT %d",
			req_state_str, maxsize);
	}
	else
	{
		len = snprintf(query, MEDIUM_QUERY_LEN,
			"SELECT id, name, " \
			" extract(epoch from recordingstart), " \
			" extract(epoch from recordingend), "\
			" sourceaddress, sourceport "\
			" FROM recordings "\
			" WHERE streaminterface_id=%" PRIu64 " AND state='%s' "\
			" ORDER BY ID LIMIT %d",
			streaminterface_id, req_state_str, maxsize);
	}

	if (len == MEDIUM_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is too small(%d/%d)!",
			 __FUNCTION__, len, MEDIUM_QUERY_LEN);
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
		return 4;
	}

#ifdef NODAEMON_DEBUG_DB
	fprintf(stderr, "size: %zd\n\n", sizeof(struct _recordings));
#endif

	numtuples = PQntuples(res);
	for (i = 0; i < numtuples; i++)
	{

		// == streaminterface_id and state are given to the function ==
		recording[i].streaminterface_id = streaminterface_id;
		strncpy(recording[i].state, req_state_str, DB_MAX_STATE);

		// == database id ==
		recording[i].id = atoll(PQgetvalue(res, i, 0));

		// == recording name ==
		strncpy(recording[i].name, PQgetvalue(res, i, 1), DB_MAX_NAME_LENGTH);
	
		// == Recording start time ==
		if (PQgetisnull(res, i, 2))
		{
			recording[i].recordingstart_timestamp = 0 ;
		}
		else if (strtold(PQgetvalue(res, i, 2), NULL) < 0)
		{
			syslog(LOG_CRIT, "%s: recordingstart is preEPOCH",
				__FUNCTION__);
			recording[i].recordingstart_timestamp = 0;
		}
		else
		{
			recording[i].recordingstart_timestamp = (uint64_t)(strtold(PQgetvalue(res, i, 2), NULL)*1000000);
		}

		// == Recording end ==
		if (PQgetisnull(res, i, 3))
		{
			recording[i].recordingend_timestamp = 0 ;
		}
		else if (strtold(PQgetvalue(res, i, 3), NULL) < 0)
		{
			syslog(LOG_CRIT, "%s: recordingend is preEPOCH",
				__FUNCTION__);
			recording[i].recordingend_timestamp = 0;
		}
		else
		{
			recording[i].recordingend_timestamp = (uint64_t)(strtold(PQgetvalue(res, i, 3), NULL)*1000000);
		}

		// == Source address ==
		strncpy(recording[i].sourceaddress, PQgetvalue(res, i, 4), DB_MAX_TIMESTAMP);

		// == Source port ==
		if (PQgetisnull(res,i,5))
		{
			recording[i].sourceport = 0;
		}
		else
		{
			recording[i].sourceport = atoi(PQgetvalue(res, i, 5));
		}
	}
	*retvalues = numtuples;
	PQclear(res);
	return 0;
}

int db_fetch_recording(PGconn **conn, s_recordings *recording, uint64_t recorderid)
{
	char query[MEDIUM_QUERY_LEN];
	char req_state_str[DB_MAX_STATE];
	PGresult *res;
	int i, numtuples, error=0;
	
	if (conn == NULL || recording == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!", __FUNCTION__);
		return 1;
	}

	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection.",
			__FUNCTION__);
		return 2;
	}

	// Set everything in the struct to 0
	memset(recording, 0, 1*sizeof(struct _recordings));
	memset(query, 0, MEDIUM_QUERY_LEN);
	memset(req_state_str, 0, DB_MAX_STATE);

	int len;
	len = snprintf(query, MEDIUM_QUERY_LEN,
		"SELECT id, name, " \
		" extract(epoch from recordingstart), " \
		" extract(epoch from recordingend), "\
		" sourceaddress, sourceport, state, streaminterface_id "\
		" FROM recordings "\
		" WHERE id='%"PRIu64"'",
		recorderid);

	if (len == MEDIUM_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is too small(%d/%d)!",
			 __FUNCTION__, len, MEDIUM_QUERY_LEN);
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
		return 4;
	}

	numtuples = PQntuples(res);

	// Check number of rows returned 
	if (numtuples != 1)
	{
		PQclear(res);
		return 5;
	}

#ifdef NODAEMON_DEBUG_DB
	fprintf(stderr, "size: %zd\n\n", sizeof(struct _recordings));
#endif

	i = 0;
	// == database id ==
	recording[i].id = atoll(PQgetvalue(res, i, 0));

	// == recording name ==
	strncpy(recording[i].name, PQgetvalue(res, i, 1), DB_MAX_NAME_LENGTH);

	// == Recording start time ==
	if (PQgetisnull(res, i, 2))
	{
		recording[i].recordingstart_timestamp = 0 ;
	}
	else if (strtold(PQgetvalue(res, i, 2), NULL) < 0)
	{
		syslog(LOG_CRIT, "%s: recordingstart is preEPOCH",
			__FUNCTION__);
		recording[i].recordingstart_timestamp = 0;
	}
	else
	{
		recording[i].recordingstart_timestamp = (uint64_t)(strtold(PQgetvalue(res, i, 2), NULL)*1000000);
	}

	// == Recording end ==
	if (PQgetisnull(res, i, 3))
	{
		recording[i].recordingend_timestamp = 0 ;
	}
	else if (strtold(PQgetvalue(res, i, 3), NULL) < 0)
	{
		syslog(LOG_CRIT, "%s: recordingend is preEPOCH",
			__FUNCTION__);
		recording[i].recordingend_timestamp = 0;
	}
	else
	{
		recording[i].recordingend_timestamp = (uint64_t)(strtold(PQgetvalue(res, i, 3), NULL)*1000000);
	}

	// == Source address ==
	strncpy(recording[i].sourceaddress, PQgetvalue(res, i, 4), DB_MAX_TIMESTAMP);

	// == Source port ==
	if (PQgetisnull(res,i,5))
	{
		recording[i].sourceport = 0;
	}
	else
	{
		recording[i].sourceport = atoi(PQgetvalue(res, i, 5));
	}

	// == recording name ==
	strncpy(recording[i].state, PQgetvalue(res, i, 6), DB_MAX_STATE);
	if (strncmp(recording[i].state, "init", 4) == 0)
		recording[i].e_state = INIT;
	else if (strncmp(recording[i].state, "recording", 9) == 0)
		recording[i].e_state = RECORDING;
	else if (strncmp(recording[i].state, "done", 4) == 0)
		recording[i].e_state = DONE;
	else if (strncmp(recording[i].state, "paused", 5) == 0)
		recording[i].e_state = PAUSED;
	else
	{
		syslog(LOG_CRIT, "%s: unknown state %s",
			__FUNCTION__, recording[i].state);
		error = 6;
	}

	// == streaminterface_id ==
	if (PQgetisnull(res, i, 7))
	{
		recording[i].streaminterface_id = 0;
	}
	else
	{
		recording[i].streaminterface_id = atoll(PQgetvalue(res, i, 7));
	}

	PQclear(res);
	return error;
}

int db_fetch_recordingbyname(PGconn **conn, s_recordings *recording, char *name)
{
	char query[MEDIUM_QUERY_LEN];
	char req_state_str[DB_MAX_STATE];
	PGresult *res;
	int i, numtuples, error=0;
	char *namestr;
	
	if (conn == NULL || recording == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!", __FUNCTION__);
		return 1;
	}

	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection.",
			__FUNCTION__);
		return 2;
	}

	// Set everything in the struct to 0
	memset(recording, 0, 1*sizeof(struct _recordings));
	memset(query, 0, MEDIUM_QUERY_LEN);
	memset(req_state_str, 0, DB_MAX_STATE);

	namestr = pqescape(name);
	if(namestr == NULL)
	{
		syslog(LOG_ERR, "Could not allocate memory for namestr");
		return 2;
	}
	
	int len;
	len = snprintf(query, MEDIUM_QUERY_LEN,
		"SELECT id, name, " \
		" extract(epoch from recordingstart), " \
		" extract(epoch from recordingend), "\
		" sourceaddress, sourceport, state, streaminterface_id "\
		" FROM recordings "\
		" WHERE name='%s'",
		namestr);

	if (len == MEDIUM_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is too small(%d/%d)!",
			 __FUNCTION__, len, MEDIUM_QUERY_LEN);
	}
	
#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n", query);
#endif

	res = PQexec(*conn, query);
	free(namestr);
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
		return 0;
	}
	else if (numtuples != 1)
	{
		PQclear(res);
		return 5;
	}

#ifdef NODAEMON_DEBUG_DB
	fprintf(stderr, "size: %zd\n\n", sizeof(struct _recordings));
#endif

	i = 0;
	// == database id ==
	recording[i].id = atoll(PQgetvalue(res, i, 0));

	// == recording name ==
	strncpy(recording[i].name, PQgetvalue(res, i, 1), DB_MAX_NAME_LENGTH);

	// == Recording start time ==
	if (PQgetisnull(res, i, 2))
	{
		recording[i].recordingstart_timestamp = 0 ;
	}
	else if (strtold(PQgetvalue(res, i, 2), NULL) < 0)
	{
		syslog(LOG_CRIT, "%s: recordingstart is preEPOCH",
			__FUNCTION__);
		recording[i].recordingstart_timestamp = 0;
	}
	else
	{
		recording[i].recordingstart_timestamp = (uint64_t)(strtold(PQgetvalue(res, i, 2), NULL)*1000000);
	}

	// == Recording end ==
	if (PQgetisnull(res, i, 3))
	{
		recording[i].recordingend_timestamp = 0 ;
	}
	else if (strtold(PQgetvalue(res, i, 3), NULL) < 0)
	{
		syslog(LOG_CRIT, "%s: recordingend is preEPOCH",
			__FUNCTION__);
		recording[i].recordingend_timestamp = 0;
	}
	else
	{
		recording[i].recordingend_timestamp = (uint64_t)(strtold(PQgetvalue(res, i, 3), NULL)*1000000);
	}

	// == Source address ==
	strncpy(recording[i].sourceaddress, PQgetvalue(res, i, 4), DB_MAX_TIMESTAMP);

	// == Source port ==
	if (PQgetisnull(res,i,5))
	{
		recording[i].sourceport = 0;
	}
	else
	{
		recording[i].sourceport = atoi(PQgetvalue(res, i, 5));
	}

	// == recording name ==
	strncpy(recording[i].state, PQgetvalue(res, i, 6), DB_MAX_STATE);
	if (strncmp(recording[i].state, "init", 4) == 0)
		recording[i].e_state = INIT;
	else if (strncmp(recording[i].state, "recording", 9) == 0)
		recording[i].e_state = RECORDING;
	else if (strncmp(recording[i].state, "done", 4) == 0)
		recording[i].e_state = DONE;
	else if (strncmp(recording[i].state, "paused", 5) == 0)
		recording[i].e_state = PAUSED;
	else
	{
		syslog(LOG_CRIT, "%s: unknown state %s",
			__FUNCTION__, recording[i].state);
		error = 6;
	}

	// == streaminterface_id ==
	if (PQgetisnull(res, i, 7))
	{
		recording[i].streaminterface_id = 0;
	}
	else
	{
		recording[i].streaminterface_id = atoll(PQgetvalue(res, i, 7));
	}

	PQclear(res);
	return error;
}

// Assume that recording structure are queried before and only update fields
// * state
// * recordingstart
// * recordingend
// * sourceaddress
// * sourceport
int db_update_recordings(PGconn **conn, s_recordings *recording)
{
	PGresult *res;
	char update_query[BIG_QUERY_LEN];
	int update_len;
	char *sourceaddress = NULL;
	char req_state_str[DB_MAX_STATE],
		*req_state=NULL,
		recordingstart_u_str[DB_MAX_MODE_LENGTH], 
		*recordingstart=NULL,
		recordingend_u_str[DB_MAX_MODE_LENGTH],
		*recordingend=NULL;

	if (conn == NULL || recording == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!", __FUNCTION__);
		return 1;
	}

	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection.", __FUNCTION__);
		return 2;
	}

	memset(update_query, 0, BIG_QUERY_LEN);
	memset(req_state_str, 0, DB_MAX_STATE);
	memset(recordingstart_u_str, 0, DB_MAX_MODE_LENGTH);
	memset(recordingend_u_str, 0, DB_MAX_MODE_LENGTH);

	switch (recording->e_state)
	{
		case INIT:
			strncpy(req_state_str, "init", DB_MAX_STATE);
			break;
		case RECORDING:
			strncpy(req_state_str, "recording", DB_MAX_STATE);
			break;
		case DONE:
			strncpy(req_state_str, "done", DB_MAX_STATE);
			break;
		case PAUSED:
			strncpy(req_state_str, "paused", DB_MAX_STATE);
			break;
		default:
			return 3;
	}
	req_state = pqescape(req_state_str);
	if (req_state == NULL)
	{
		return 4;
	}

	if (strlen(recording->sourceaddress) == 0)
	{
		sourceaddress = pqescape("NULL");
	}
	else
	{
		sourceaddress = pqescape(recording->sourceaddress);
	}

	if (sourceaddress == NULL)
	{
		free (req_state);
		return 5;
	}

	if (recording->recordingstart_timestamp == 0)
	{
		snprintf(recordingstart_u_str, DB_MAX_MODE_LENGTH, "NULL");	
	}
	else
	{
		snprintf(recordingstart_u_str, DB_MAX_MODE_LENGTH,
			"to_timestamp(%"PRIu64")",
			recording->recordingstart_timestamp/1000000);
	}
	recordingstart = pqescape(recordingstart_u_str);
	if (recordingstart == NULL)
	{
		free(req_state);
		free(sourceaddress);
		return 6;
	}

	if (recording->recordingend_timestamp == 0)
	{
		snprintf(recordingend_u_str, DB_MAX_MODE_LENGTH, "NULL");	
	}
	else
	{
		snprintf(recordingend_u_str, DB_MAX_MODE_LENGTH,
			"to_timestamp(%"PRIu64")",
			recording->recordingend_timestamp/1000000);
	}
	recordingend = pqescape(recordingend_u_str);
	if (recordingend == NULL)
	{
		free(recordingstart);
		free(req_state);
		free(sourceaddress);
		return 7;
	}

	update_len = snprintf(update_query, BIG_QUERY_LEN,
		"UPDATE recordings SET "\
		" state='%s', "\
		" recordingstart=%s, "\
		" recordingend=%s, "\
		" sourceaddress='%s', "\
		" sourceport='%d' "\
		" WHERE "\
		" id=%" PRIu64,
		req_state,
		recordingstart,
		recordingend,
		sourceaddress,
		recording->sourceport,
		recording->id);

	if (update_len == BIG_QUERY_LEN || update_len < 0)
	{
		syslog(LOG_WARNING, "%s: snprintf buf is to small(%d/%d)!",
			__FUNCTION__, update_len, BIG_QUERY_LEN);
	}

	free(recordingend);
	free(recordingstart);
	free(req_state);
	free(sourceaddress);

#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n", update_query);
#endif
	res = PQexec(*conn, update_query);
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "%s: Answer not ok\n",
			__FUNCTION__);
#endif
		PQclear(res);
		return 8;
	}

	if (atoi(PQcmdTuples(res)) < 1)
	{
		PQclear(res);
		return 9;
	}
	PQclear(res);
	return 0;
}

int db_insert_recordings(PGconn **conn, s_recordings *recording)
{
	PGresult *res;
	char insert_query[BIG_QUERY_LEN];
	int query_len;
	char *sourceaddress=NULL, *name=NULL;
	char req_state_str[DB_MAX_STATE],
		*req_state=NULL,
		recordingstart_i_str[DB_MAX_MODE_LENGTH], 
		*recordingstart=NULL,
		recordingend_i_str[DB_MAX_MODE_LENGTH],
		*recordingend=NULL,
		streaminterfaceid_str[256];
		
	if (conn == NULL || recording == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!", __FUNCTION__);
		return 1;
	}

	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection.", __FUNCTION__);
		return 2;
	}

	memset(insert_query, 0, BIG_QUERY_LEN);
	memset(req_state_str, 0, DB_MAX_STATE);
	memset(recordingstart_i_str, 0, DB_MAX_MODE_LENGTH);
	memset(recordingend_i_str, 0, DB_MAX_MODE_LENGTH);
	memset(streaminterfaceid_str,0,256);

	if(recording->streaminterface_id == 0)
	{
		sprintf(streaminterfaceid_str, "null");
	}
	else
	{
		snprintf(streaminterfaceid_str, 255, "%"PRIu64, recording->streaminterface_id);
	}
	switch (recording->e_state)
	{
		case INIT:
			strncpy(req_state_str, "init", DB_MAX_STATE);
			break;
		case RECORDING:
			strncpy(req_state_str, "recording", DB_MAX_STATE);
			break;
		case DONE:
			strncpy(req_state_str, "done", DB_MAX_STATE);
			break;
		case PAUSED:
			strncpy(req_state_str, "paused", DB_MAX_STATE);
			break;
		default:
			return 3;
	}
	req_state = pqescape(req_state_str);
	if (req_state == NULL)
	{
		return 4;
	}

	if (strlen(recording->sourceaddress) == 0)
	{
		sourceaddress = pqescape("NULL");	
	}
	else
	{
		sourceaddress = pqescape(recording->sourceaddress);
	}
	if (sourceaddress == NULL)
	{
		free(req_state);
		return 5;
	}

	if (strlen(recording->name) == 0)
	{
		name = pqescape("NULL");
	}
	else
	{
		name = pqescape(recording->name);
	}
	if (name == NULL)
	{
		free(req_state);
		free(sourceaddress);
		return 6;
	}

/*	if (recording->recordingstart_timestamp == 0)
	{
		snprintf(recordingstart_i_str, DB_MAX_MODE_LENGTH, "NULL");	
	}
	else
	{
		snprintf(recordingstart_i_str, DB_MAX_MODE_LENGTH,
			"to_timestamp(%"PRIu64")",
			recording->recordingstart_timestamp/1000000);
	}
	recordingstart = pqescape(recordingstart_i_str);
	if (recordingstart == NULL)
	{
		free(name);
		free(req_state);
		free(sourceaddress);
		return 7;
	}

	if (recording->recordingend_timestamp == 0)
	{
		snprintf(recordingend_i_str, DB_MAX_MODE_LENGTH, "NULL");	
	}
	else
	{
		snprintf(recordingend_i_str, DB_MAX_MODE_LENGTH,
			"to_timestamp(%"PRIu64")",
			recording->recordingend_timestamp/1000000);
	}
	recordingend = pqescape(recordingend_i_str);
	if (recordingend == NULL)
	{
		free(recordingstart);
		free(name);
		free(req_state);
		free(sourceaddress);
		return 8;
	}
*/

	query_len = snprintf(insert_query, BIG_QUERY_LEN,
		"INSERT INTO recordings (streaminterface_id, name, state, "\
		" recordingstart, recordingend, sourceaddress, sourceport) "\
		" VALUES (%s, '%s', '%s', null, null, '%s', '%d')",
		streaminterfaceid_str,
		name,
		req_state,
		sourceaddress,
		recording->sourceport);

	if (query_len == BIG_QUERY_LEN || query_len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is to small(%d/%d)!",
			__FUNCTION__, query_len, BIG_QUERY_LEN);
	}

	free(recordingend);
	free(recordingstart);
	free(name);
	free(req_state);
	free(sourceaddress);

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
		return 9;
	}

	if (atoi(PQcmdTuples(res)) < 1)
	{
		PQclear(res);
		return 10;
	}
	PQclear(res);
	return 0;
}

int db_delete_recording(PGconn **conn, uint64_t recordingid)
{
	PGresult *res;
	char delete_query[MEDIUM_QUERY_LEN];
	int query_len;
	
	query_len = snprintf(delete_query, MEDIUM_QUERY_LEN, "DELETE FROM recordings WHERE id=%"PRIu64, recordingid);
	if (query_len == BIG_QUERY_LEN || query_len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is to small(%d/%d)!",
			__FUNCTION__, query_len, MEDIUM_QUERY_LEN);
	}


#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n", delete_query);
#endif
	res = PQexec(*conn, delete_query);
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "%s: Answer not ok\n", __FUNCTION__);
#endif
		PQclear(res);
		return 1;
	}

	if (atoi(PQcmdTuples(res)) < 1)
	{
		PQclear(res);
		return 2;
	}
	
	PQclear(res);
	
	return 0;
}

// ====== recorder stats =======
int db_insert_recorderstats(PGconn **conn, s_recorderstats *recorderstats)
{
	PGresult *res;
	char insert_query[BIG_QUERY_LEN];
	char update_query[BIG_QUERY_LEN];
	int insert_len, update_len;
	char req_state_str[DB_MAX_STATE];
	char *req_state;

	char startrecordat_i_str[DB_MAX_MODE_LENGTH],
		startrecordat_u_str[DB_MAX_MODE_LENGTH],
		startpos_i_str[DB_MAX_MODE_LENGTH],
		startpos_u_str[DB_MAX_MODE_LENGTH],
		lastpos_i_str[DB_MAX_MODE_LENGTH],
		lastpos_u_str[DB_MAX_MODE_LENGTH],
		currpos_i_str[DB_MAX_MODE_LENGTH],
		currpos_u_str[DB_MAX_MODE_LENGTH],
		recordto_i_str[DB_MAX_MODE_LENGTH],
		recordto_u_str[DB_MAX_MODE_LENGTH];
	char *startrecordat_i=NULL, *startrecordat_u=NULL,
		*startpos_i=NULL, *startpos_u=NULL,
		*lastpos_i=NULL, *lastpos_u=NULL,
		*currpos_i=NULL, *currpos_u=NULL,
		*recordto_i=NULL, *recordto_u=NULL;

	if (conn == NULL || recorderstats == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!",
			__FUNCTION__);
		return 1;
	}

	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection.",
			__FUNCTION__);
		return 2;
	}

	memset(insert_query, 0, BIG_QUERY_LEN);
	memset(update_query, 0, BIG_QUERY_LEN);
	memset(req_state_str, 0, DB_MAX_STATE);
	memset(startrecordat_i_str, 0, DB_MAX_MODE_LENGTH);
	memset(startrecordat_u_str, 0, DB_MAX_MODE_LENGTH);
	memset(startpos_i_str, 0, DB_MAX_MODE_LENGTH);
	memset(startpos_u_str, 0, DB_MAX_MODE_LENGTH);
	memset(lastpos_i_str, 0, DB_MAX_MODE_LENGTH);
	memset(lastpos_u_str, 0, DB_MAX_MODE_LENGTH);
	memset(currpos_i_str, 0, DB_MAX_MODE_LENGTH);
	memset(currpos_u_str, 0, DB_MAX_MODE_LENGTH);
	memset(recordto_i_str, 0, DB_MAX_MODE_LENGTH);
	memset(recordto_u_str, 0, DB_MAX_MODE_LENGTH);

	// == state ==
	switch (recorderstats->e_state)
	{
		case INIT:
			strncpy(req_state_str, "init", DB_MAX_STATE);
			break;
		case RECORDING:
			strncpy(req_state_str, "recording", DB_MAX_STATE);
			break;
		case DONE:
			strncpy(req_state_str, "done", DB_MAX_STATE);
			break;
		case PAUSED:
			strncpy(req_state_str, "paused", DB_MAX_STATE);
			break;
		default:
			return 3;
	}

	// == startrecordat ==
	if (recorderstats->startrecordat_timestamp == 0)
	{
		snprintf(startrecordat_i_str, DB_MAX_MODE_LENGTH,
			"NULL");
		snprintf(startrecordat_u_str, DB_MAX_MODE_LENGTH,
			"startrecordat=NULL");	
	}
	else
	{
		snprintf(startrecordat_i_str, DB_MAX_MODE_LENGTH,
			" to_timestamp(%"PRIu64")",
			recorderstats->startrecordat_timestamp/1000000);
		snprintf(startrecordat_u_str, DB_MAX_MODE_LENGTH,
			" startrecordat=to_timestamp(%"PRIu64")",
			recorderstats->startrecordat_timestamp/1000000);
	}

	// == startpos ==
	if (recorderstats->startpos_timestamp == 0)
	{
		snprintf(startpos_i_str, DB_MAX_MODE_LENGTH,
			"NULL");
		snprintf(startpos_u_str, DB_MAX_MODE_LENGTH,
			"startpos=NULL");	
	}
	else
	{
		snprintf(startpos_i_str, DB_MAX_MODE_LENGTH,
			" to_timestamp(%"PRIu64")",
			recorderstats->startpos_timestamp/1000000);
		snprintf(startpos_u_str, DB_MAX_MODE_LENGTH,
			" startpos=to_timestamp(%"PRIu64")",
			recorderstats->startpos_timestamp/1000000);
	}

	if (recorderstats->lastpos_timestamp == 0)
	{
		snprintf(lastpos_i_str, DB_MAX_MODE_LENGTH,
			"NULL");
		snprintf(lastpos_u_str, DB_MAX_MODE_LENGTH,
			"lastpos=NULL");	
	}
	else
	{
		snprintf(lastpos_i_str, DB_MAX_MODE_LENGTH,
			" to_timestamp(%"PRIu64")",
			recorderstats->lastpos_timestamp/1000000);
		snprintf(lastpos_u_str, DB_MAX_MODE_LENGTH,
			" lastpos=to_timestamp(%"PRIu64")",
			recorderstats->lastpos_timestamp/1000000);
	}

	if (recorderstats->currpos_timestamp == 0)
	{
		snprintf(currpos_i_str, DB_MAX_MODE_LENGTH,
			"NULL");
		snprintf(currpos_u_str, DB_MAX_MODE_LENGTH,
			"currpos=NULL");	
	}
	else
	{
		snprintf(currpos_i_str, DB_MAX_MODE_LENGTH,
			" to_timestamp(%"PRIu64")",
			recorderstats->currpos_timestamp/1000000);
		snprintf(currpos_u_str, DB_MAX_MODE_LENGTH,
			" currpos=to_timestamp(%"PRIu64")",
			recorderstats->currpos_timestamp/1000000);
	}


	// 
	if (recorderstats->recordto_timestamp == 0)
	{
		snprintf(recordto_i_str, DB_MAX_MODE_LENGTH,
			"NULL");
		snprintf(recordto_u_str, DB_MAX_MODE_LENGTH,
			"recordto=NULL");	
	}
	else
	{
		snprintf(recordto_i_str, DB_MAX_MODE_LENGTH,
			" to_timestamp(%"PRIu64")",
			recorderstats->recordto_timestamp/1000000);
		snprintf(recordto_u_str, DB_MAX_MODE_LENGTH,
			" recordto=to_timestamp(%"PRIu64")",
			recorderstats->recordto_timestamp/1000000);
	}



	req_state = pqescape(req_state_str);
	startrecordat_i = pqescape(startrecordat_i_str);
	startrecordat_u = pqescape(startrecordat_u_str);
	startpos_i = pqescape(startpos_i_str);
	startpos_u = pqescape(startpos_u_str);
	lastpos_i = pqescape(lastpos_i_str);
	lastpos_u = pqescape(lastpos_u_str);
	currpos_i = pqescape(currpos_i_str);
	currpos_u = pqescape(currpos_u_str);
	recordto_i = pqescape(recordto_i_str);
	recordto_u = pqescape(recordto_u_str);
	if (req_state == NULL ||
		startrecordat_i == NULL ||
		startrecordat_u == NULL ||
		startpos_i == NULL ||
		startpos_u == NULL ||
		lastpos_i == NULL ||
		lastpos_u == NULL ||
		currpos_i == NULL ||
		currpos_u == NULL ||
		recordto_i == NULL ||
		recordto_u == NULL)
	{
		if (recordto_u != NULL)
			free(recordto_u);
		if (recordto_i != NULL)
			free(recordto_i);
		if (currpos_u != NULL)
			free(currpos_u);
		if (currpos_i != NULL)
			free(currpos_i);
		if (lastpos_u != NULL)
			free(lastpos_u);
		if (lastpos_i != NULL)
			free(lastpos_i);
		if (startpos_u != NULL)
			free(startpos_u);
		if (startpos_i != NULL)
			free(startpos_i);
		if (startrecordat_u != NULL)
			free(startrecordat_u);
		if (startrecordat_i != NULL)
			free(startrecordat_i);
		if (req_state != NULL)
			free(req_state);
		return 4;
	}
	insert_len = snprintf(insert_query, BIG_QUERY_LEN,
		"INSERT INTO recorderstats "\
		" (streaminterface_id, state, startrecordat, "\
		" startpos, lastpos, currpos, recordto) "\
		" VALUES "\
		" (%" PRIu64 ", '%s', %s, "\
		" %s, %s, %s, %s) ",
		recorderstats->streaminterface_id,
		req_state_str,
		startrecordat_i,
		startpos_i,
		lastpos_i,
		currpos_i,
		recordto_i);

	if (insert_len == BIG_QUERY_LEN || insert_len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is to small(%d/%d)!",
			__FUNCTION__, insert_len, BIG_QUERY_LEN);
	}
	update_len = snprintf(update_query, BIG_QUERY_LEN,
		"UPDATE recorderstats SET "\
		" state='%s', %s, "\
		" %s, %s, "\
		" %s, %s "\
		" WHERE "\
		" streaminterface_id=%" PRIu64,
		req_state_str,
		startrecordat_u,
		startpos_u,
		lastpos_u,
		currpos_u,
		recordto_u,
		recorderstats->streaminterface_id);

	if (update_len == BIG_QUERY_LEN || update_len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is to small(%d/%d)!",
			__FUNCTION__, update_len, BIG_QUERY_LEN);
	}

	free(recordto_u);
	free(recordto_i);
	free(currpos_u);
	free(currpos_i);
	free(lastpos_u);
	free(lastpos_i);
	free(startpos_u);
	free(startpos_i);
	free(startrecordat_u);
	free(startrecordat_i);
	free(req_state);

#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n", update_query);
#endif
	res = PQexec(*conn, update_query);
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "%s: Answer not ok\n", __FUNCTION__);
#endif
		PQclear(res);
		return 5;
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
			fprintf(stderr, "%s: Answer not ok\n", __FUNCTION__);
#endif
			PQclear(res);
			return 6;
		}
	}
	
	PQclear(res);
	return 0;
}

int db_fetch_recorderstatsbystreaminterfaceid(PGconn **conn, s_recorderstats *recorderstats, int maxsize, int *retvalues, uint64_t streaminterface_id)
{
	char query[MEDIUM_QUERY_LEN];
	PGresult *res;
	int i,numtuples;
	
	if(conn == NULL || recorderstats == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!", __FUNCTION__);
		return 1;
	}
	
	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection.", __FUNCTION__);
		return 2;
	}

	memset(query, 0, MEDIUM_QUERY_LEN);
	int len = snprintf(query, MEDIUM_QUERY_LEN, "SELECT id, "\
					" state, "\
					" extract(epoch from startrecordat), "\
					" extract(epoch from startpos), "\
					" extract(epoch from lastpos), "\
					" extract(epoch from currpos), "\
					" extract(epoch from recordto) "\
					" FROM recorderstats WHERE "\
					" streaminterface_id=%" PRIu64 \
					" LIMIT %d",
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
		return 3;
	}

#ifdef NODAEMON_DEBUG_DB
	fprintf(stderr, "size: %zd\n\n", sizeof(struct _recorderstats));
#endif

	int error = 0;
	numtuples = PQntuples(res);
	for (i = 0; i < numtuples; i++)
	{
		// == streaminterface id ==
		recorderstats[i].streaminterface_id = streaminterface_id;

		// == id ==
		if (PQgetisnull(res, i, 0))
		{
			error = 4;
			break;
		}
		recorderstats[i].id = atoll(PQgetvalue(res, i, 0));

		// == state ==
		strncpy(recorderstats[i].state, PQgetvalue(res, i, 1), DB_MAX_STATE);
		if (strncmp(recorderstats[i].state, "init", 4) == 0)
			recorderstats[i].e_state = INIT;
		else if (strncmp(recorderstats[i].state, "recording", 9) == 0)
			recorderstats[i].e_state = RECORDING;
		else if (strncmp(recorderstats[i].state, "done", 4) == 0)
			recorderstats[i].e_state = DONE;
		else if (strncmp(recorderstats[i].state, "paused", 5) == 0)
			recorderstats[i].e_state = PAUSED;
		else
		{
			syslog(LOG_CRIT, "%s: unknown state %s",
				__FUNCTION__, recorderstats[i].state);
			error = 5;
			break;
		}

		// == startrecordat ==
		if (PQgetisnull(res, i, 2))
		{
			recorderstats[i].startrecordat_timestamp = 0 ;
		}
		else if (strtold(PQgetvalue(res, i, 2), NULL) < 0)
		{
			syslog(LOG_CRIT, "%s: startrecordat is preEPOCH", __FUNCTION__);
			recorderstats[i].startrecordat_timestamp = 0;
		}
		else
		{
			recorderstats[i].startrecordat_timestamp = (uint64_t)(strtold(PQgetvalue(res, i, 2), NULL));
		}

		// == startpos ==
		if (PQgetisnull(res, i, 3))
		{
			recorderstats[i].startpos_timestamp = 0;
		}
		else if (strtold(PQgetvalue(res, i, 3), NULL) < 0)
		{
			syslog(LOG_CRIT, "%s: startpos is preEPOCH", __FUNCTION__);
			recorderstats[i].startpos_timestamp = 0;
		}
		else
		{
			recorderstats[i].startpos_timestamp = (uint64_t)(strtold(PQgetvalue(res, i, 3), NULL));
		}

		// == lastpos ==
		if (PQgetisnull(res, i, 4))
		{
			recorderstats[i].lastpos_timestamp = 0;
		}
		else if (strtold(PQgetvalue(res, i, 4), NULL) < 0)
		{
			syslog(LOG_CRIT, "%s: lastpos is preEPOCH", __FUNCTION__);
			recorderstats[i].lastpos_timestamp = 0;
		}
		else
		{
			recorderstats[i].lastpos_timestamp = (uint64_t)(strtold(PQgetvalue(res, i, 4), NULL));
		}

		// == currpos ==
		if (PQgetisnull(res, i, 5))
		{
			recorderstats[i].currpos_timestamp = 0;
		}
		else if (strtold(PQgetvalue(res, i, 5), NULL) < 0)
		{
			syslog(LOG_CRIT, "%s: currpos is preEPOCH", __FUNCTION__);
			recorderstats[i].currpos_timestamp = 0;
		}
		else
		{
			recorderstats[i].currpos_timestamp = (uint64_t)(strtold(PQgetvalue(res, i, 5), NULL));
		}

		// == recordto ==
		if (PQgetisnull(res, i, 6))
		{
			recorderstats[i].recordto_timestamp = 0;
		}
		else if (strtold(PQgetvalue(res, i, 6), NULL) < 0)
		{
			syslog(LOG_CRIT, "%s: recordto is preEPOCH", __FUNCTION__);
			recorderstats[i].recordto_timestamp = 0;
		}
		else
		{
			recorderstats[i].recordto_timestamp = (uint64_t)(strtold(PQgetvalue(res, i, 6), NULL));
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

int db_delete_recorderstats(PGconn **conn, uint64_t streaminterface_id)
{
	char query[MEDIUM_QUERY_LEN];
	PGresult *res;

	if (conn == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!", __FUNCTION__);
		return 1;
	}

	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection.", __FUNCTION__);
		return 2;
	}

	memset(query, 0, MEDIUM_QUERY_LEN);
	int len = snprintf(query, MEDIUM_QUERY_LEN,
		"DELETE FROM recorderstats "\
		" WHERE streaminterface_id=%" PRIu64,
		streaminterface_id);
	if (len == MEDIUM_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is too small(%d/%d)!",
			__FUNCTION__, len, MEDIUM_QUERY_LEN);
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
		syslog(LOG_WARNING, "%s: Could not delete from recorderstats!",
			__FUNCTION__);
		PQclear(res);
		return 3;
	}

	PQclear(res);
	return 0;
}


// ====== Playercom ======
int db_fetch_playercoms(PGconn **conn, s_playercoms *playercom, int maxsize, int *retvalues, uint64_t streaminterface_id)
{
	char query[MEDIUM_QUERY_LEN];
	PGresult *res;
	int i, numtuples;
	
	if (conn == NULL || playercom == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!", __FUNCTION__);
		return 1;
	}

	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection.",
			__FUNCTION__);
		return 2;
	}

	// Set everything in the struct to 0
	memset(playercom, 0, maxsize*sizeof(struct _playercoms));
	memset(query, 0, MEDIUM_QUERY_LEN);

	int len = snprintf(query, MEDIUM_QUERY_LEN,
		"SELECT id, pathnumber, action, " \
		" extract(epoch from timestamp), " \
		" jumptopos, extract(epoch from jumptopos), "\
		" startplayat, extract(epoch from startplayat), "\
		" playto, extract(epoch from playto) "\
		" FROM playercoms "\
		" WHERE streaminterface_id=%" PRIu64 " ORDER BY ID LIMIT %d",
		streaminterface_id, maxsize);
	if (len == MEDIUM_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "%s: snprintf buf is too small(%d/%d)!",
			__FUNCTION__, len, MEDIUM_QUERY_LEN);
	}

#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n", query);
#endif

	res = PQexec(*conn, query);

	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "%s: Answer not ok\n",
			__FUNCTION__);
#endif
		PQclear(res);
		return 3;
	}

	if (PQntuples(res) != 1)
	{
		PQclear(res);
		return 4;
	}

	if (PQgetisnull(res, 0, 0) || PQgetisnull(res, 0, 1))
	{
		PQclear(res);
		return 5;
	}

#ifdef NODAEMON_DEBUG_DB
	fprintf(stderr, "size: %zd\n\n", sizeof(struct _playercoms));
#endif

	int error = 0;
	numtuples = PQntuples(res);
	for (i = 0; i < numtuples; i++)
	{
		// == Streaminterface id ==
		playercom[i].streaminterface_id = streaminterface_id;

		// == ID ==
		playercom[i].id = atoll(PQgetvalue(res, i, 0));

		// == pathnumber ==
		if (PQgetisnull(res, i, 1))
		{
			error = 6;
			break;
		}
		playercom[i].pathnumber = atoll(PQgetvalue(res, i, 1));

		// == Action ==
		strncpy(playercom[i].action, PQgetvalue(res, i, 2), DB_MAX_STATE);
		if (strncmp(playercom[i].action, "jump", 4) == 0)
		{
			playercom[i].e_action = JUMP;
		}
		else if (strncmp(playercom[i].action, "playto", 6) == 0)
		{
			playercom[i].e_action = PLAYTO;
		}
		else if (strncmp(playercom[i].action, "play", 4) == 0)
		{
			playercom[i].e_action = PLAY;
		}
		else if (strncmp(playercom[i].action, "pause", 5) == 0)
		{
			playercom[i].e_action = PAUSE;
		}
		else
		{
			syslog(LOG_CRIT, "%s: unknown action %s",
				__FUNCTION__, playercom[i].action);
			error = 8;
			break;
		}


		// == timestamp ==
		if (PQgetisnull(res, i, 3))
		{
			playercom[i].create_timestamp = 0;
		}
		else if (strtold(PQgetvalue(res, i, 3), NULL) < 0)
		{
			syslog(LOG_CRIT, "%s: create timestamp is preEPOCH",
				__FUNCTION__);
			playercom[i].create_timestamp = 0;
		}
		else
		{
			playercom[i].create_timestamp = (uint64_t)(strtold(PQgetvalue(res, i, 3), NULL)*1000000);
		}

		// == jumptopos ==
		strncpy(playercom[i].jumptopos, PQgetvalue(res, i, 4), DB_MAX_TIMESTAMP);

		// == jumptopos_timestamp ==
		if (PQgetisnull(res, i, 5))
		{
			playercom[i].jumptopos_timestamp = 0 ;
		}
		else if (strtold(PQgetvalue(res, i, 5), NULL) < 0)
		{
			syslog(LOG_CRIT, "%s: jumptopos is preEPOCH",
				__FUNCTION__);
			playercom[i].jumptopos_timestamp = 0;
		}
		else
		{
			playercom[i].jumptopos_timestamp = (uint64_t)(strtold(PQgetvalue(res, i, 5), NULL)*1000000);
		}

		// == startplayat ==
		strncpy(playercom[i].startplayat, PQgetvalue(res, i, 6), DB_MAX_TIMESTAMP);

		// == startplayat_timestamp ==
		if (PQgetisnull(res, i, 7))
		{
			playercom[i].startplayat_timestamp = 0;
		}
		else if (strtold(PQgetvalue(res, i, 7), NULL) < 0)
		{
			syslog(LOG_CRIT, "%s: startplayat is preEPOCH",
				__FUNCTION__);
			playercom[i].startplayat_timestamp = 0;
		}
		else
		{
			playercom[i].startplayat_timestamp = (uint64_t)(strtold(PQgetvalue(res, i, 7), NULL)*1000000);
		}

		// == playto ==
		strncpy(playercom[i].playto, PQgetvalue(res, i, 8), DB_MAX_TIMESTAMP);

		// == playto_timestamp ==
		if (PQgetisnull(res, i, 9))
		{
			playercom[i].playto_timestamp = 0;
		}
		else if (strtold(PQgetvalue(res, i, 9), NULL) < 0)
		{
			syslog(LOG_CRIT, "%s: playto is preEPOCH",
				__FUNCTION__);
			playercom[i].playto_timestamp = 0;
		}
		else
		{
			playercom[i].playto_timestamp = (uint64_t)(strtold(PQgetvalue(res, i, 9), NULL)*1000000);
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

int db_delete_playercoms(PGconn **conn, uint64_t streaminterface_id)
{
	char query[MEDIUM_QUERY_LEN];
	PGresult *res;

	if (conn == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!",
			__FUNCTION__);
		return 1;
	}

	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection.",
			__FUNCTION__);
		return 2;
	}

	memset(query, 0, MEDIUM_QUERY_LEN);
	int len = snprintf(query, MEDIUM_QUERY_LEN,
		"DELETE FROM playercoms "\
		" WHERE streaminterface_id=%" PRIu64,
		streaminterface_id);
	if (len == MEDIUM_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is too small(%d/%d)!",
			__FUNCTION__, len, MEDIUM_QUERY_LEN);
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
		syslog(LOG_WARNING, "%s Could not delete from playercoms!",
			__FUNCTION__);
		PQclear(res);
		return 3;
	}

	PQclear(res);
	return 0;
}

int db_delete_playercomsbyid(PGconn **conn, uint64_t id)
{
	char query[MEDIUM_QUERY_LEN];
	PGresult *res;

	if (conn == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!",
			__FUNCTION__);
		return 1;
	}

	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection.",
			__FUNCTION__);
		return 2;
	}

	memset(query, 0, MEDIUM_QUERY_LEN);
	int len = snprintf(query, MEDIUM_QUERY_LEN,
		"DELETE FROM playercoms "\
		" WHERE id=%" PRIu64,
		id);
	if (len == MEDIUM_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "%s: snprintf buf is too small(%d/%d)!",
			__FUNCTION__, len, MEDIUM_QUERY_LEN);
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
		syslog(LOG_WARNING, "%s Could not delete from playercoms!",
			__FUNCTION__);
		PQclear(res);
		return 3;
	}

	PQclear(res);
	return 0;
}

int db_insert_playercom(PGconn **conn, s_playercoms *playercom)
{
	PGresult *res;
	char *action=NULL,
		*startplayat=NULL,
		*jumptopos=NULL,
		*playto=NULL;
	char startplayatq[DB_MAX_TIMESTAMP+32],
		jumptoposq[DB_MAX_TIMESTAMP+32],
		playtoq[DB_MAX_TIMESTAMP+32],
		create_timestampq[DB_MAX_MODE_LENGTH];
	char insert_query[BIG_QUERY_LEN];
	int insert_len;

	if (conn == NULL || playercom == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!",
			__FUNCTION__);
		return 1;
	}

	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection.",
			__FUNCTION__);
		return 2;
	}

	memset(startplayatq, 0, sizeof(startplayatq));
	memset(jumptoposq, 0, sizeof(jumptoposq));
	memset(playtoq, 0, sizeof(playtoq));
	memset(create_timestampq, 0, DB_MAX_MODE_LENGTH);
	memset(insert_query, 0, BIG_QUERY_LEN);
	
	if (strlen(playercom->action) == 0)
	{
		switch (playercom->e_action)
		{
		case JUMP:
			action = pqescape("jump");
			break;
		case PLAY:
			action = pqescape("play");
			break;
		case PAUSE:
			action = pqescape("pause");
			break;
		case PLAYTO:
			action = pqescape("playto");
			break;
		default:
			syslog(LOG_CRIT, "%s Unknown state %d",
				__FUNCTION__, playercom->e_action);
			break;
		}
	}
	else
	{
		action = pqescape(playercom->action);
	}
	if (action == NULL)
	{
		return 3;
	}

	if (strlen(playercom->startplayat) == 0)
	{
		snprintf(startplayatq, sizeof(startplayatq),
			"%s", "NULL");
	}
	else
	{
		 snprintf(startplayatq, sizeof(startplayatq),
			"to_timestamp(%s)", playercom->startplayat);
	}
	startplayat = pqescape(startplayatq);

	if (startplayat == NULL)
	{
		free(action);
		return 4;
	}
	
	if (strlen(playercom->jumptopos) == 0)
	{
		snprintf(jumptoposq, sizeof(jumptoposq),
			"%s", "NULL");
	}
	else
	{
		snprintf(jumptoposq, sizeof(jumptoposq),
			"to_timestamp(%s)", playercom->jumptopos);
	}
	jumptopos = pqescape(jumptoposq);
	if (jumptopos == NULL)
	{
		free(startplayat);
		free(action);
		return 5;
	}

	if (strlen(playercom->playto) == 0)
	{
		snprintf(playtoq, sizeof(playtoq),
			"%s", "NULL");
	}
	else
	{
		snprintf(playtoq, sizeof(playtoq),
			"to_timestamp(%s)", playercom->playto);
	}
	playto = pqescape(playtoq);
	if (playto == NULL)
	{
		free(jumptopos);
		free(startplayat);
		free(action);
		return 6;
	}

	if (playercom->create_timestamp == 0)
	{
		snprintf(create_timestampq, DB_MAX_MODE_LENGTH, "now()");
	}
	else
	{
		snprintf(create_timestampq, DB_MAX_MODE_LENGTH,
			"to_timestamp(%"PRIu64")", playercom->create_timestamp);
	}


	insert_len = snprintf(insert_query, BIG_QUERY_LEN,
		"INSERT INTO playercoms "\
		" (timestamp, streaminterface_id, pathnumber, "\
		" action, startplayat, jumptopos, playto)"\
		" VALUES "\
		" (%s, %" PRIu64 ", %" PRIu64 ", "\
		" '%s', %s, %s, %s)",
		create_timestampq,
		playercom->streaminterface_id,
		playercom->pathnumber,
		action,
		startplayat,
		jumptopos,
		playto);

	if (insert_len == BIG_QUERY_LEN || insert_len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is to small(%d/%d)!",
			__FUNCTION__, insert_len, BIG_QUERY_LEN);
	}


	free(playto);
	free(startplayat);
	free(jumptopos);
	free(action);
	
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
		return 7;
	}

	if (atoi(PQcmdTuples(res)) < 1)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "%s: Inserted nothing.\n", __FUNCTION__);
#endif
		PQclear(res);
		return 8;
	}
	
	PQclear(res);
	return 0;
}

int db_count_playercoms(PGconn **conn, uint64_t streaminterface_id)
{
	uint64_t result = 0;
	int  ret=0;
	PGresult *res;
	char query[MEDIUM_QUERY_LEN];

	if (conn == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!",
			__FUNCTION__);
		return -1;
	}

	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection.",
			__FUNCTION__);
		return -2;
	}

	memset(query, 0, MEDIUM_QUERY_LEN);
	ret = snprintf(query, MEDIUM_QUERY_LEN,
		"SELECT COUNT(*) FROM playercoms WHERE streaminterface_id=%"PRIu64,
		 streaminterface_id);
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

int db_insert_playerstats(PGconn **conn, s_playerstats *playerstats)
{
	PGresult *res;
	char insert_query[BIG_QUERY_LEN];
	char update_query[BIG_QUERY_LEN];
	int insert_len, update_len;
	char req_state_str[DB_MAX_STATE];
	char *req_state;

	char startplayat_i_str[DB_MAX_MODE_LENGTH],
		startplayat_u_str[DB_MAX_MODE_LENGTH],
		startpos_i_str[DB_MAX_MODE_LENGTH],
		startpos_u_str[DB_MAX_MODE_LENGTH],
		lastpos_i_str[DB_MAX_MODE_LENGTH],
		lastpos_u_str[DB_MAX_MODE_LENGTH],
		currpos_i_str[DB_MAX_MODE_LENGTH],
		currpos_u_str[DB_MAX_MODE_LENGTH],
		playto_i_str[DB_MAX_MODE_LENGTH],
		playto_u_str[DB_MAX_MODE_LENGTH];
	char *startplayat_i=NULL, *startplayat_u=NULL,
		*startpos_i=NULL, *startpos_u=NULL,
		*lastpos_i=NULL, *lastpos_u=NULL,
		*currpos_i=NULL, *currpos_u=NULL,
		*playto_i=NULL, *playto_u=NULL;

	if (conn == NULL || playerstats == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!",
			__FUNCTION__);
		return 1;
	}

	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection.",
			__FUNCTION__);
		return 2;
	}

	memset(insert_query, 0, BIG_QUERY_LEN);
	memset(update_query, 0, BIG_QUERY_LEN);
	memset(req_state_str, 0, DB_MAX_STATE);
	memset(startplayat_i_str, 0, DB_MAX_MODE_LENGTH);
	memset(startplayat_u_str, 0, DB_MAX_MODE_LENGTH);
	memset(startpos_i_str, 0, DB_MAX_MODE_LENGTH);
	memset(startpos_u_str, 0, DB_MAX_MODE_LENGTH);
	memset(lastpos_i_str, 0, DB_MAX_MODE_LENGTH);
	memset(lastpos_u_str, 0, DB_MAX_MODE_LENGTH);
	memset(currpos_i_str, 0, DB_MAX_MODE_LENGTH);
	memset(currpos_u_str, 0, DB_MAX_MODE_LENGTH);
	memset(playto_i_str, 0, DB_MAX_MODE_LENGTH);
	memset(playto_u_str, 0, DB_MAX_MODE_LENGTH);

	// == action ==
	switch (playerstats->e_state)
	{
		case JUMP:
			strncpy(req_state_str, "jump", DB_MAX_STATE);
			break;
		case PLAY:
			strncpy(req_state_str, "play", DB_MAX_STATE);
			break;
		case PAUSE:
			strncpy(req_state_str, "pause", DB_MAX_STATE);
			break;
		case PLAYTO:
			strncpy(req_state_str, "playto", DB_MAX_STATE);
			break;
		default:
			return 3;
	}

	// == startplayat ==
	if (playerstats->startplayat_timestamp == 0)
	{
		snprintf(startplayat_i_str, DB_MAX_MODE_LENGTH,
			"NULL");
		snprintf(startplayat_u_str, DB_MAX_MODE_LENGTH,
			"startplayat=NULL");	
	}
	else
	{
		snprintf(startplayat_i_str, DB_MAX_MODE_LENGTH,
			" to_timestamp(%"PRIu64")",
			playerstats->startplayat_timestamp/1000000);
		snprintf(startplayat_u_str, DB_MAX_MODE_LENGTH,
			" startplayat=to_timestamp(%"PRIu64")",
			playerstats->startplayat_timestamp/1000000);
	}

	// == startpos ==
	if (playerstats->startpos_timestamp == 0)
	{
		snprintf(startpos_i_str, DB_MAX_MODE_LENGTH,
			"NULL");
		snprintf(startpos_u_str, DB_MAX_MODE_LENGTH,
			"startpos=NULL");	
	}
	else
	{
		snprintf(startpos_i_str, DB_MAX_MODE_LENGTH,
			" to_timestamp(%"PRIu64")",
			playerstats->startpos_timestamp/1000000);
		snprintf(startpos_u_str, DB_MAX_MODE_LENGTH,
			" startpos=to_timestamp(%"PRIu64")",
			playerstats->startpos_timestamp/1000000);
	}

	// == lastpos ==
	if (playerstats->lastpos_timestamp == 0)
	{
		snprintf(lastpos_i_str, DB_MAX_MODE_LENGTH,
			"NULL");
		snprintf(lastpos_u_str, DB_MAX_MODE_LENGTH,
			"lastpos=NULL");	
	}
	else
	{
		snprintf(lastpos_i_str, DB_MAX_MODE_LENGTH,
			" to_timestamp(%"PRIu64")",
			playerstats->lastpos_timestamp/1000000);
		snprintf(lastpos_u_str, DB_MAX_MODE_LENGTH,
			" lastpos=to_timestamp(%"PRIu64")",
			playerstats->lastpos_timestamp/1000000);
	}

	// == currpos ==
	if (playerstats->currpos_timestamp == 0)
	{
		snprintf(currpos_i_str, DB_MAX_MODE_LENGTH,
			"NULL");
		snprintf(currpos_u_str, DB_MAX_MODE_LENGTH,
			"currpos=NULL");	
	}
	else
	{
		snprintf(currpos_i_str, DB_MAX_MODE_LENGTH,
			" to_timestamp(%"PRIu64")",
			playerstats->currpos_timestamp/1000000);
		snprintf(currpos_u_str, DB_MAX_MODE_LENGTH,
			" currpos=to_timestamp(%"PRIu64")",
			playerstats->currpos_timestamp/1000000);
	}


	// == playto == 
	if (playerstats->playto_timestamp == 0)
	{
		snprintf(playto_i_str, DB_MAX_MODE_LENGTH,
			"NULL");
		snprintf(playto_u_str, DB_MAX_MODE_LENGTH,
			"playto=NULL");	
	}
	else
	{
		snprintf(playto_i_str, DB_MAX_MODE_LENGTH,
			" to_timestamp(%"PRIu64")",
			playerstats->playto_timestamp/1000000);
		snprintf(playto_u_str, DB_MAX_MODE_LENGTH,
			" playto=to_timestamp(%"PRIu64")",
			playerstats->playto_timestamp/1000000);
	}



	req_state = pqescape(req_state_str);
	startplayat_i = pqescape(startplayat_i_str);
	startplayat_u = pqescape(startplayat_u_str);
	startpos_i = pqescape(startpos_i_str);
	startpos_u = pqescape(startpos_u_str);
	lastpos_i = pqescape(lastpos_i_str);
	lastpos_u = pqescape(lastpos_u_str);
	currpos_i = pqescape(currpos_i_str);
	currpos_u = pqescape(currpos_u_str);
	playto_i = pqescape(playto_i_str);
	playto_u = pqescape(playto_u_str);

	if (req_state == NULL ||
		startplayat_i == NULL ||
		startplayat_u == NULL ||
		startpos_i == NULL ||
		startpos_u == NULL ||
		lastpos_i == NULL ||
		lastpos_u == NULL ||
		currpos_i == NULL ||
		currpos_u == NULL ||
		playto_i == NULL ||
		playto_u == NULL)
	{
		if (playto_u != NULL)
			free(playto_u);
		if (playto_i != NULL)
			free(playto_i);
		if (currpos_u != NULL)
			free(currpos_u);
		if (currpos_i != NULL)
			free(currpos_i);
		if (lastpos_u != NULL)
			free(lastpos_u);
		if (lastpos_i != NULL)
			free(lastpos_i);
		if (startpos_u != NULL)
			free(startpos_u);
		if (startpos_i != NULL)
			free(startpos_i);
		if (startplayat_u != NULL)
			free(startplayat_u);
		if (startplayat_i != NULL)
			free(startplayat_i);
		if (req_state != NULL)
			free(req_state);
		return 4;
	}
	insert_len = snprintf(insert_query, BIG_QUERY_LEN,
		"INSERT INTO playerstats "\
		" (streaminterface_id, pathnumber, recording_id, "\
		" state, startplayat, "\
		" startpos, lastpos, currpos, playto) "\
		" VALUES "\
		" (%" PRIu64 ", %" PRIu64 ", %" PRIu64 ", "\
		" '%s', %s, "\
		" %s, %s, %s, %s) ",
		playerstats->streaminterface_id,
		playerstats->pathnumber,
		playerstats->recording_id,
		req_state,
		startplayat_i,
		startpos_i,
		lastpos_i,
		currpos_i,
		playto_i);

	if (insert_len == BIG_QUERY_LEN || insert_len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is to small(%d/%d)!",
			__FUNCTION__, insert_len, BIG_QUERY_LEN);
	}
	update_len = snprintf(update_query, BIG_QUERY_LEN,
		"UPDATE playerstats SET "\
		" state='%s', %s, "\
		" %s, %s, "\
		" %s, %s "\
		" WHERE "\
		" streaminterface_id=%" PRIu64 " AND pathnumber = %" PRIu64,
		req_state,
		startplayat_u,
		startpos_u,
		lastpos_u,
		currpos_u,
		playto_u,
		playerstats->streaminterface_id,
		playerstats->pathnumber);

	if (update_len == BIG_QUERY_LEN || update_len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is to small(%d/%d)!",
			__FUNCTION__, update_len, BIG_QUERY_LEN);
	}

	free(playto_u);
	free(playto_i);
	free(currpos_u);
	free(currpos_i);
	free(lastpos_u);
	free(lastpos_i);
	free(startpos_u);
	free(startpos_i);
	free(startplayat_u);
	free(startplayat_i);
	free(req_state);

#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n", update_query);
#endif
	res = PQexec(*conn, update_query);
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "%s: Answer not ok\n",
			__FUNCTION__);
#endif
		PQclear(res);
		return 5;
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
			fprintf(stderr, "%s: Answer not ok\n",
				__FUNCTION__);
#endif
			PQclear(res);
			return 6;
		}
	}
	
	PQclear(res);
	return 0;
}


int db_fetch_playerstatsbystreaminterfaceid(PGconn **conn, s_playerstats *playerstats, int maxsize, int *retvalues, uint64_t streaminterface_id)
{
	char query[MEDIUM_QUERY_LEN];
	PGresult *res;
	int i,numtuples;
	
	if(conn == NULL || playerstats == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!", __FUNCTION__);
		return 1;
	}
	
	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection.", __FUNCTION__);
		return 2;
	}

	memset(query, 0, MEDIUM_QUERY_LEN);
	int len = snprintf(query, MEDIUM_QUERY_LEN,
				"SELECT id, "\
				" pathnumber, "\
				" recording_id, "\
				" state, "\
				" extract(epoch from startplayat), "\
				" extract(epoch from startpos), "\
				" extract(epoch from lastpos), "\
				" extract(epoch from currpos), "\
				" extract(epoch from playto) "\
				" FROM playerstats WHERE "\
				" streaminterface_id=%" PRIu64 \
				" LIMIT %d",
				streaminterface_id, maxsize);

	if (len == MEDIUM_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is too small(%d/%d)!",
			__FUNCTION__, len, MEDIUM_QUERY_LEN);
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
		return 3;
	}

#ifdef NODAEMON_DEBUG_DB
	fprintf(stderr, "size: %zd\n\n", sizeof(struct _playerstats));
#endif

	int error = 0;
	numtuples = PQntuples(res);
	for (i = 0; i < numtuples; i++)
	{
		// == streaminterface id ==
		playerstats[i].streaminterface_id = streaminterface_id;

		// == id ==
		if (PQgetisnull(res, i, 0))
		{
			error = 4;
			break;
		}
		playerstats[i].id = atoll(PQgetvalue(res, i, 0));

		// == Pathnumber ==
		if (PQgetisnull(res, i, 1))
		{
			error = 5;
			break;
		}
		playerstats[i].pathnumber = atoll(PQgetvalue(res, i, 1));

		// == recording id ==
		if (PQgetisnull(res, i, 2))
		{
			error = 6;
			break;
		}
		playerstats[i].recording_id = atoll(PQgetvalue(res, i, 2));

		// == state ==
		strncpy(playerstats[i].state, PQgetvalue(res, i, 3), DB_MAX_STATE);
		if (strncmp(playerstats[i].state, "jump", 4) == 0)
			playerstats[i].e_state = JUMP;
		else if (strncmp(playerstats[i].state, "playto", 6) == 0)
			playerstats[i].e_state = PLAYTO;
		else if (strncmp(playerstats[i].state, "play", 4) == 0)
			playerstats[i].e_state = DONE;
		else if (strncmp(playerstats[i].state, "pause", 5) == 0)
			playerstats[i].e_state = PAUSE;
		else
		{
			syslog(LOG_CRIT, "%s: unknown state %s",
				__FUNCTION__, playerstats[i].state);
			error = 7;
			break;
		}

		// == startplayat ==
		if (PQgetisnull(res, i, 4))
		{
			playerstats[i].startplayat_timestamp = 0 ;
		}
		else if (strtold(PQgetvalue(res, i, 4), NULL) < 0)
		{
			syslog(LOG_CRIT, "%s: startplayat is preEPOCH", __FUNCTION__);
			playerstats[i].startplayat_timestamp = 0;
		}
		else
		{
			playerstats[i].startplayat_timestamp = (uint64_t)(strtold(PQgetvalue(res, i, 4), NULL));
		}

		// == startpos ==
		if (PQgetisnull(res, i, 5))
		{
			playerstats[i].startpos_timestamp = 0;
		}
		else if (strtold(PQgetvalue(res, i, 5), NULL) < 0)
		{
			syslog(LOG_CRIT, "%s: startpos is preEPOCH", __FUNCTION__);
			playerstats[i].startpos_timestamp = 0;
		}
		else
		{
			playerstats[i].startpos_timestamp = (uint64_t)(strtold(PQgetvalue(res, i, 5), NULL));
		}

		// == lastpos ==
		if (PQgetisnull(res, i, 6))
		{
			playerstats[i].lastpos_timestamp = 0;
		}
		else if (strtold(PQgetvalue(res, i, 6), NULL) < 0)
		{
			syslog(LOG_CRIT, "%s: lastpos is preEPOCH", __FUNCTION__);
			playerstats[i].lastpos_timestamp = 0;
		}
		else
		{
			playerstats[i].lastpos_timestamp = (uint64_t)(strtold(PQgetvalue(res, i, 6), NULL));
		}

		// == currpos ==
		if (PQgetisnull(res, i, 7))
		{
			playerstats[i].currpos_timestamp = 0;
		}
		else if (strtold(PQgetvalue(res, i, 7), NULL) < 0)
		{
			syslog(LOG_CRIT, "%s: currpos is preEPOCH", __FUNCTION__);
			playerstats[i].currpos_timestamp = 0;
		}
		else
		{
			playerstats[i].currpos_timestamp = (uint64_t)(strtold(PQgetvalue(res, i, 7), NULL));
		}

		// == playto ==
		if (PQgetisnull(res, i, 8))
		{
			playerstats[i].playto_timestamp = 0;
		}
		else if (strtold(PQgetvalue(res, i, 8), NULL) < 0)
		{
			syslog(LOG_CRIT, "%s: recordto is preEPOCH", __FUNCTION__);
			playerstats[i].playto_timestamp = 0;
		}
		else
		{
			playerstats[i].playto_timestamp = (uint64_t)(strtold(PQgetvalue(res, i, 8), NULL));
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

int db_delete_playerstats(PGconn **conn, uint64_t streaminterface_id)
{
	char query[MEDIUM_QUERY_LEN];
	PGresult *res;

	if (conn == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!",
			__FUNCTION__);
		return 1;
	}

	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection.",
			__FUNCTION__);
		return 2;
	}

	memset(query, 0, MEDIUM_QUERY_LEN);
	int len = snprintf(query, MEDIUM_QUERY_LEN,
		"DELETE FROM playerstats "\
		" WHERE streaminterface_id=%" PRIu64,
		streaminterface_id);
	if (len == MEDIUM_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is too small(%d/%d)!",
			__FUNCTION__, len, MEDIUM_QUERY_LEN);
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
		syslog(LOG_WARNING, "%s: Could not delete from playerstats!",
			__FUNCTION__);
		PQclear(res);
		return 3;
	}

	PQclear(res);
	return 0;
}
