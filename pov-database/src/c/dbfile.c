#include "dbcommon.h"
#include "dbfile.h"
#include <limits.h>

// Internal query function
int db_fetch_file_internal(PGconn **conn, s_file *file, int maxsize, int *retvalues, uint64_t streaminterface_id, char *query);
int db_update_file_internal(PGconn **conn, char *query);
int db_delete_internal(PGconn **conn, char *query);

// Query everything
int db_fetch_file(PGconn **conn, s_file *file, int maxsize, int *retvalues, uint64_t streaminterface_id)
{
	char query[MEDIUM_QUERY_LEN];
	int len = snprintf(query, MEDIUM_QUERY_LEN,
		"SELECT id, filename, size, " \
		" priority, extract(epoch from queuedtime), " \
		" extract(epoch from startsendtime), extract(epoch from stopsendtime), " \
		" transferredbytes, status, comment " \
		" FROM files "\
		" WHERE streaminterface_id=%" PRIu64 " ORDER BY ID LIMIT %d",
		streaminterface_id, maxsize);

	if (len == MEDIUM_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "db_fetch_file snprintf buf is too small(%d/%d)!",
			len, MEDIUM_QUERY_LEN);
	}

	return db_fetch_file_internal(conn, file, maxsize, retvalues, streaminterface_id, query);
}

int db_fetch_file_create(PGconn **conn, s_file *file, int maxsize, int *retvalues, uint64_t streaminterface_id)
{
	char query[MEDIUM_QUERY_LEN];
	int len = snprintf(query, MEDIUM_QUERY_LEN,
		"SELECT id, filename, size, " \
		" priority, extract(epoch from queuedtime), " \
		" extract(epoch from startsendtime), extract(epoch from stopsendtime), " \
		" transferredbytes, status, comment " \
		" FROM files "\
		" WHERE streaminterface_id=%" PRIu64 " AND status='Create' ORDER BY ID LIMIT %d",
		streaminterface_id, maxsize);

	if (len == MEDIUM_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "db_fetch_file snprintf buf is too small(%d/%d)!",
			len, MEDIUM_QUERY_LEN);
	}

	return db_fetch_file_internal(conn, file, maxsize, retvalues, streaminterface_id, query);
}

int db_fetch_file_queued(PGconn **conn, s_file *file, int maxsize, int *retvalues, uint64_t streaminterface_id)
{
	char query[MEDIUM_QUERY_LEN];
	int len = snprintf(query, MEDIUM_QUERY_LEN,
		"SELECT id, filename, size, " \
		" priority, extract(epoch from queuedtime), " \
		" extract(epoch from startsendtime), extract(epoch from stopsendtime), " \
		" transferredbytes, status, comment " \
		" FROM files "\
		" WHERE streaminterface_id=%" PRIu64 " AND status='Queued' ORDER BY ID LIMIT %d",
		streaminterface_id, maxsize);

	if (len == MEDIUM_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "db_fetch_file snprintf buf is too small(%d/%d)!",
			len, MEDIUM_QUERY_LEN);
	}

	return db_fetch_file_internal(conn, file, maxsize, retvalues, streaminterface_id, query);
}

int db_fetch_file_intransit(PGconn **conn, s_file *file, int maxsize, int *retvalues, uint64_t streaminterface_id)
{
	char query[MEDIUM_QUERY_LEN];
	int len = snprintf(query, MEDIUM_QUERY_LEN,
		"SELECT id, filename, size, " \
		" priority, extract(epoch from queuedtime), " \
		" extract(epoch from startsendtime), extract(epoch from stopsendtime), " \
		" transferredbytes, status, comment " \
		" FROM files "\
		" WHERE streaminterface_id=%" PRIu64 " AND status='Intransit' ORDER BY ID LIMIT %d",
		streaminterface_id, maxsize);

	if (len == MEDIUM_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "db_fetch_file snprintf buf is too small(%d/%d)!",
			len, MEDIUM_QUERY_LEN);
	}

	return db_fetch_file_internal(conn, file, maxsize, retvalues, streaminterface_id, query);
}

// Query streaminterface_id AND status='Onhold'
int db_fetch_file_onhold(PGconn **conn, s_file *file, int maxsize, int *retvalues, uint64_t streaminterface_id)
{
	char query[MEDIUM_QUERY_LEN];
	int len = snprintf(query, MEDIUM_QUERY_LEN,
		"SELECT id, filename, size, " \
		" priority, extract(epoch from queuedtime), " \
		" extract(epoch from startsendtime), extract(epoch from stopsendtime), " \
		" transferredbytes, status, comment " \
		" FROM files "\
		" WHERE streaminterface_id=%" PRIu64 " AND status='Onhold' ORDER BY ID LIMIT %d",
		streaminterface_id, maxsize);

	if (len == MEDIUM_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "db_fetch_file snprintf buf is too small(%d/%d)!",
			len, MEDIUM_QUERY_LEN);
	}

	return db_fetch_file_internal(conn, file, maxsize, retvalues, streaminterface_id, query);
}

int db_fetch_file_to_transfer(PGconn **conn, s_file *file, int maxsize, int *retvalues, uint64_t streaminterface_id)
{
	char query[MEDIUM_QUERY_LEN];
	int len = snprintf(query, MEDIUM_QUERY_LEN,
		"SELECT id, filename, size, " \
		" priority, extract(epoch from queuedtime), " \
		" extract(epoch from startsendtime), extract(epoch from stopsendtime), " \
		" transferredbytes, status, comment " \
		" FROM files "\
		" WHERE streaminterface_id=%" PRIu64 " AND (status='Queued' OR status='Onhold') " \
		" ORDER BY priority DESC, queuedtime LIMIT %d",
		streaminterface_id, maxsize);

	if (len == MEDIUM_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "db_fetch_file snprintf buf is too small(%d/%d)!",
			len, MEDIUM_QUERY_LEN);
	}

	return db_fetch_file_internal(conn, file, maxsize, retvalues, streaminterface_id, query);
}

int db_fetch_file_not_done(PGconn **conn, s_file *file, int maxsize, int *retvalues, uint64_t streaminterface_id)
{
	char query[MEDIUM_QUERY_LEN];
	int len = snprintf(query, MEDIUM_QUERY_LEN,
		"SELECT id, filename, size, " \
		" priority, extract(epoch from queuedtime), " \
		" extract(epoch from startsendtime), extract(epoch from stopsendtime), " \
		" transferredbytes, status, comment " \
		" FROM files "\
		" WHERE streaminterface_id=%" PRIu64 " AND status<>'Done' ORDER BY ID LIMIT %d",
		streaminterface_id, maxsize);

	if (len == MEDIUM_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "db_fetch_file snprintf buf is too small(%d/%d)!",
			len, MEDIUM_QUERY_LEN);
	}

	return db_fetch_file_internal(conn, file, maxsize, retvalues, streaminterface_id, query);
}

int db_fetch_file_by_name(PGconn **conn, s_file *file, int maxsize, int *retvalues, char *fname, uint64_t streaminterface_id)
{
	char query[MEDIUM_QUERY_LEN];
	int len = snprintf(query, MEDIUM_QUERY_LEN,
		"SELECT id, filename, size, " \
		" priority, extract(epoch from queuedtime), " \
		" extract(epoch from startsendtime), extract(epoch from stopsendtime), " \
		" transferredbytes, status, comment " \
		" FROM files "\
		" WHERE filename='%s' AND status<>'Done' ORDER BY priority DESC, queuedtime LIMIT %d", fname, maxsize);

	if (len == MEDIUM_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "db_fetch_file snprintf buf is too small(%d/%d)!",
			len, MEDIUM_QUERY_LEN);
	}

	return db_fetch_file_internal(conn, file, maxsize, retvalues, streaminterface_id, query);
}


int db_fetch_file_internal(PGconn **conn, s_file *file, int maxsize, int *retvalues, uint64_t streaminterface_id, char *query)
{
	PGresult *res;
	int i, numtuples;

	if (conn == NULL || file == NULL || retvalues == NULL || query == NULL)
	{
		syslog(LOG_CRIT, "db_fetch_file argument is NULL!");
		return 1;
	}

	// Set everything in the struct to 0
	memset(file, 0, maxsize*sizeof(s_file));

	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "db_fetch_file cant reconnect DB connection");
		return 1;
	}

#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n", query);
#endif

	res = PQexec(*conn, query);

	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "db_fetch_file: Answer not ok\n");
#endif
		PQclear(res);
		return 2;
	}

#ifdef NODAEMON_DEBUG_DB
	fprintf(stderr, "size: %zd\n\n", sizeof(s_file));
#endif

	numtuples = PQntuples(res);
	for (i = 0; i < numtuples; i++)
	{
		// ---- streaminterface_id ---
		file[i].streaminterface_id = streaminterface_id;

		// ---- ID ----
		if (PQgetisnull(res, i, 0))
		{
			file[i].id = 0;
		}
		else
		{
			file[i].id = atoll(PQgetvalue(res, i, 0));
		}

		// ---- filename ----
		if (PQgetisnull(res, i, 1))
		{
			file[i].filename[0] = '\0';
		}
		else
		{
			strncpy(file[i].filename, PQgetvalue(res, i, 1), DB_MAX_NAME_LENGTH);
		}

		// ---- size ----
		if (PQgetisnull(res, i, 2))
		{
			file[i].size = 0;
		}
		else
		{
			file[i].size = atoll(PQgetvalue(res, i, 2));
		}

		// ---- priority ----
		if (PQgetisnull(res, i, 3))
		{
			file[i].priority = 0;
		}
		else
		{
			file[i].priority = atoll(PQgetvalue(res, i, 3));
		}

		// ---- queuedtime ----
		long long queuedtime;
		if (PQgetisnull(res, i, 4))
		{
			file[i].queuedtime = 0;
		}
		else if ((queuedtime = strtoll(PQgetvalue(res, i, 4), NULL, 10)) < 0)
		{
			syslog(LOG_CRIT, "db_fetch_file: queuedtime is preEPOCH");
			file[i].queuedtime = 0;
		}
		else if (errno == ERANGE)
		{
			syslog(LOG_CRIT, "db_fetch_file: conversion error (%s)",
				PQgetvalue(res, i, 4));
			file[i].queuedtime = 0;
		}
		else
		{
			file[i].queuedtime = (uint64_t)(queuedtime*1000000);
		}

		// Cant assume rest are set - need to check for NULL fields
		// ---- startsendtime ----
		long long startsendtime;
		if (PQgetisnull(res, i, 5))
		{
			file[i].startsendtime = 0;
		}
		else if ((startsendtime = strtoll(PQgetvalue(res, i, 5), NULL, 10)) < 0)
		{
			syslog(LOG_CRIT, "db_fetch_file: startsendtime timestamp is preEPOCH");
			file[i].startsendtime = 0;
		}
		else if (errno == ERANGE)
		{
			syslog(LOG_CRIT, "db_fetch_file: conversion error (%s)",
				PQgetvalue(res, i, 5));
			file[i].queuedtime = 0;
		}
		else
		{
			file[i].startsendtime = (uint64_t)(startsendtime*1000000);
		}

		// ---- stopsendtime ----
		long long stopsendtime;
		if (PQgetisnull(res, i, 6))
		{
			file[i].stopsendtime = 0;
		}
		else if ((stopsendtime = strtoll(PQgetvalue(res, i, 6), NULL, 10)) < 0)
		{
			syslog(LOG_CRIT, "db_fetch_file: startsendtime timestamp is preEPOCH");
			file[i].stopsendtime = 0;
		}
		else if (errno == ERANGE)
		{
			syslog(LOG_CRIT, "db_fetch_file: conversion error (%s)",
				PQgetvalue(res, i, 6));
			file[i].queuedtime = 0;
		}
		else
		{
			file[i].stopsendtime = (uint64_t)(stopsendtime*1000000);
		}

		// ---- transferredbytes ----
		long long transferredbytes;
		if (PQgetisnull(res, i, 7))
		{
			file[i].transferredbytes = 0;
		}
		else if ((transferredbytes = strtoll(PQgetvalue(res, i, 7), NULL, 10)) < 0)
		{
			syslog(LOG_CRIT, "db_fetch_file: negative transferredbytes");
			file[i].transferredbytes = 0;
		}
		else if (errno == ERANGE)
		{
			syslog(LOG_CRIT, "db_fetch_file: conversion error (%s)",
				PQgetvalue(res, i, 7));
			file[i].queuedtime = 0;
		}
		else
		{
			file[i].transferredbytes = (uint64_t)(transferredbytes);
		}

		// ---- e_status & status ----
		if (PQgetisnull(res, i, 8))
		{
			file[i].status[0] = '\0';
			file[i].e_status = QUEUED; 
		}
		else
		{
			strncpy(file[i].status, PQgetvalue(res, i, 8), DB_MAX_MODE_LENGTH);
			if (strncmp(file[i].status, "Create", 6) == 0)
			{
				file[i].e_status = CREATE;
			}
			else if (strncmp(file[i].status, "Queued", 6) == 0)
			{
				file[i].e_status = QUEUED;
			}
			else if (strncmp(file[i].status, "Intransit", 9) == 0)
			{
				file[i].e_status = INTRANSIT;
			}
			else if (strncmp(file[i].status, "Onhold", 6) == 0)
			{
				file[i].e_status = ONHOLD;
			}
			else if (strncmp(file[i].status, "Chksfail", 8) == 0)
			{
				file[i].e_status = CHKSFAIL;
			}
			else if (strncmp(file[i].status, "Done", 4) == 0)
			{
				file[i].e_status = DONE;
			}
			else
			{
				syslog(LOG_CRIT, "db_fetch_file: unknown action %s",
					file[i].status);
				PQclear(res);
				return 5;
			}
		}

		// ---- comment ----
		if (PQgetisnull(res, i, 9))
		{
			file[i].comment[0] = '\0';
		}
		else
		{
			strncpy(file[i].comment, PQgetvalue(res, i, 9), DB_MAX_NAME_LENGTH);
		}
	}
	*retvalues = numtuples;
	PQclear(res);
	return 0;
}

int db_delete_filebyinterface(PGconn **conn, uint64_t streaminterface_id)
{
	char query[SMALL_QUERY_LEN];

	int len = snprintf(query, SMALL_QUERY_LEN,
		"DELETE FROM files "\
		" WHERE streaminterface_id=%" PRIu64,
		streaminterface_id);
	if (len == SMALL_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "db_delete_filebyinterface snprintf buf is too small(%d/%d)!",
			len, SMALL_QUERY_LEN);
	}

	return db_delete_internal(conn, query);
}

int db_delete_filebynameandinterface(PGconn **conn, char *filename, uint64_t streaminterface_id)
{
	char query[SMALL_QUERY_LEN];

	int len = snprintf(query, SMALL_QUERY_LEN,
		"DELETE FROM files "\
		" WHERE streaminterface_id='%"PRIu64"' AND "\
		" filename='%s'",
		streaminterface_id, filename);
	if (len == SMALL_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "db_delete_filebyinterface snprintf buf is too small(%d/%d)!",
			len, SMALL_QUERY_LEN);
	}

	return db_delete_internal(conn, query);
}

int db_delete_filebyid(PGconn **conn, uint64_t id)
{
	char query[SMALL_QUERY_LEN];

	int len = snprintf(query, SMALL_QUERY_LEN,
		"DELETE FROM files "\
		" WHERE id=%" PRIu64,
		id);

	if (len == SMALL_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "db_delete_filebyid snprintf buf is too small(%d/%d)!",
			len, SMALL_QUERY_LEN);
	}

	return db_delete_internal(conn, query);
}

int db_delete_intransit(PGconn **conn, uint64_t streaminterface_id)
{
	char query[SMALL_QUERY_LEN];

	int len = snprintf(query, SMALL_QUERY_LEN,
		"DELETE FROM files "\
		" WHERE streaminterface_id=%" PRIu64 " AND status='Intransit'",
		streaminterface_id);

	if (len == SMALL_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "db_delete_filebyid snprintf buf is too small(%d/%d)!",
			len, SMALL_QUERY_LEN);
	}

	return db_delete_internal(conn, query);
}

int db_delete_internal(PGconn **conn, char *query)
{
	PGresult *res;

	if (conn == NULL)
	{
		syslog(LOG_CRIT, "db_delete_filebyinterface argument is NULL!");
		return 1;
	}

	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "db_delete_filebyinterface cant reconnect DB connection");
		return 1;
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
		syslog(LOG_WARNING, "Could not delete from files!");
		PQclear(res);
		return 2;
	}

	PQclear(res);
	return 0;
}

int db_fetch_file_id(PGconn **conn, s_file *file)
{
	PGresult *res;
	char select_query[MEDIUM_QUERY_LEN];
	int query_len;
	char *filename;

	if (conn == NULL || file == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!", __FUNCTION__);
		return 1;
	}

	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection.", __FUNCTION__);
		return 2;
	}

	memset(select_query, 0, MEDIUM_QUERY_LEN);

	// Fix the dynamic allocated memory first
	// --- FILENAME ---
	if (strlen(file->filename) == 0)
	{
		filename = pqescape("NULL");
	}
	else
	{
		filename = pqescape(file->filename);
	}

	if (filename == NULL)
	{
		return 3;
	}

	// QUERY ID FOR JUST INSERTED LINE

	query_len = snprintf(select_query, MEDIUM_QUERY_LEN,
		"SELECT id FROM files "\
		" WHERE streaminterface_id='%"PRIu64"' AND "\
		" filename='%s' AND "\
		" status='Onhold' ORDER by id DESC LIMIT 1",
		file->streaminterface_id,
		filename);
	if (query_len == MEDIUM_QUERY_LEN || query_len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is to small(%d/%d)!",
			__FUNCTION__, query_len, MEDIUM_QUERY_LEN);
	}

	free(filename);

#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "select: %s\n", select_query);
#endif

	res = PQexec(*conn, select_query);
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

	if (PQgetisnull(res, 0, 0))
	{
		PQclear(res);
		return 5;
	}

	file->id = atoll(PQgetvalue(res, 0, 0));
	PQclear(res);
	return 0;
}

int db_fetch_file_transferredbytes(PGconn **conn, s_file *file)
{
	PGresult *res;
	char select_query[MEDIUM_QUERY_LEN];
	int query_len;

	if (conn == NULL || file == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!", __FUNCTION__);
		return 1;
	}

	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection.", __FUNCTION__);
		return 2;
	}

	memset(select_query, 0, MEDIUM_QUERY_LEN);

	query_len = snprintf(select_query, MEDIUM_QUERY_LEN,
		"SELECT transferredbytes FROM files "\
		" WHERE id='%"PRIu64"'",
		file->id);

	if (query_len == MEDIUM_QUERY_LEN || query_len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is to small(%d/%d)!",
			__FUNCTION__, query_len, MEDIUM_QUERY_LEN);
	}

#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "select: %s\n", select_query);
#endif

	res = PQexec(*conn, select_query);
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

	if (PQgetisnull(res, 0, 0))
	{
		PQclear(res);
		return 5;
	}

	// ---- transferredbytes ----
	unsigned long long transferredbytes;
	if (PQgetisnull(res, 0, 0))
	{
		file->transferredbytes = 0;
	}
	else if ((transferredbytes = strtoull(PQgetvalue(res, 0, 0), NULL, 10)) == ULLONG_MAX)
	{
		syslog(LOG_CRIT, "db_fetch_file: negative transferredbytes");
		file->transferredbytes = 0;
	}
	else
	{
		file->transferredbytes = (uint64_t)(transferredbytes);
	}

	PQclear(res);
	return 0;
}

//
// Will insert a new file row with basic information
// * Streaminterface id
// * Name
// * Size
// * Priority
// * Comment
// Assumes multiple update of the file db entry during the growth of the file in fs
//
// Returns the database ID in file->id
// 
int db_insert_basic_file(PGconn **conn, s_file *file)
{
	PGresult *res;
	char insert_query[BIG_QUERY_LEN];
	char select_query[BIG_QUERY_LEN];
	int query_len;
	char *filename, *comment;

	if (conn == NULL || file == NULL)
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
	memset(select_query, 0, BIG_QUERY_LEN);

	// Fix the dynamic allocated memory first
	// --- FILENAME ---
	if (strlen(file->filename) == 0)
	{
		filename = pqescape("NULL");
	}
	else
	{
		filename = pqescape(file->filename);
	}

	if (filename == NULL)
	{
		return 3;
	}

	// --- COMMENT ---
	if (strlen(file->comment) == 0)
	{
		comment = pqescape("NULL");
	}
	else
	{
		comment = pqescape(file->comment);
	}

	if (comment == NULL)
	{
		free(filename);
		return 4;
	}

	// --- STATUS ---
	if (file->e_status != CREATE && strncmp(file->status, "Create", DB_MAX_STATE) == 0)
	{
		syslog(LOG_CRIT, "%s status expect to be Create at insert time.", __FUNCTION__);
	}
 
	query_len = snprintf(insert_query, BIG_QUERY_LEN,
		"INSERT INTO files "\
		"(streaminterface_id, filename, size, "\
		" priority, queuedtime, startsendtime, stopsendtime, "\
		" transferredbytes, status, comment) "\
		" VALUES "\
		"('%"PRIu64"', '%s', '%"PRIu64"', "\
		" '%"PRIu64"', NULL, NULL, NULL, "\
		" '0', 'Create', '%s')",
		file->streaminterface_id,
		filename, 
		file->size,
		file->priority,
		comment);

	if (query_len == BIG_QUERY_LEN || query_len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is to small(%d/%d)!",
			__FUNCTION__, query_len, BIG_QUERY_LEN);
	}

	// QUERY ID FOR JUST INSERTED LINE

	query_len = snprintf(select_query, BIG_QUERY_LEN,
		"SELECT id FROM files "\
		" WHERE streaminterface_id='%"PRIu64"' AND "\
		" filename='%s' AND "\
		" status='Create'",
		file->streaminterface_id,
		filename);
	if (query_len == BIG_QUERY_LEN || query_len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is to small(%d/%d)!",
			__FUNCTION__, query_len, BIG_QUERY_LEN);
	}

	free(filename);
	free(comment);

#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "insert: %s\n", insert_query);
	fprintf(stderr, "select: %s\n", select_query);
#endif
	res = PQexec(*conn, insert_query);
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "%s: Answer not ok\n", __FUNCTION__);
#endif
		PQclear(res);
		return 6;
	}
	PQclear(res);

	res = PQexec(*conn, select_query);
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

	if (PQgetisnull(res, 0, 0))
	{
		PQclear(res);
		return 5;
	}

	file->id = atoll(PQgetvalue(res, 0, 0));
	PQclear(res);
	return 0;
}

// This function will insert the whole s_file struct
// __ARE NOT USED BY rudp-file__
int db_insert_file(PGconn **conn, s_file *file)
{
	PGresult *res;
	char insert_query[BIG_QUERY_LEN];
	char select_query[BIG_QUERY_LEN];
	int query_len;
	char *filename, *comment;
	char queuedtime_str[DB_MAX_MODE_LENGTH],
		startsendtime_str[DB_MAX_MODE_LENGTH],
		stopsendtime_str[DB_MAX_MODE_LENGTH],
		status_str[DB_MAX_STATE];

	if (conn == NULL || file == NULL)
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
	memset(queuedtime_str, 0, DB_MAX_MODE_LENGTH);
	memset(startsendtime_str, 0, DB_MAX_MODE_LENGTH);
	memset(stopsendtime_str, 0, DB_MAX_MODE_LENGTH);
	memset(status_str, 0, DB_MAX_STATE);

	// Fix the dynamic allocated memory first
	// --- FILENAME ---
	if (strlen(file->filename) == 0)
	{
		filename = pqescape("NULL");
	}
	else
	{
		filename = pqescape(file->filename);
	}

	if (filename == NULL)
	{
		return 3;
	}

	// --- COMMENT ---
	if (strlen(file->comment) == 0)
	{
		comment = pqescape("NULL");
	}
	else
	{
		comment = pqescape(file->comment);
	}

	if (comment == NULL)
	{
		free(filename);
		return 4;
	}

	// The fixed numbers that can be abscent for example if time is 0 => 1970
	// --- QUEUEDTIME ---
	if (file->queuedtime == 0)
	{
		snprintf(queuedtime_str, DB_MAX_MODE_LENGTH, "NULL");
	}
	else
	{
		snprintf(queuedtime_str, DB_MAX_MODE_LENGTH,
			"to_timestamp(%"PRIu64")",
			file->queuedtime/1000000);
	}

	// --- STARTSENDTIME ---
	if (file->startsendtime == 0)
	{
		snprintf(startsendtime_str, DB_MAX_MODE_LENGTH, "NULL");
	}
	else
	{
		snprintf(startsendtime_str, DB_MAX_MODE_LENGTH,
			"to_timestamp(%"PRIu64")",
			file->startsendtime/1000000);
	}

	// --- STOPSENDTIME ---
	if (file->stopsendtime == 0)
	{
		snprintf(stopsendtime_str, DB_MAX_MODE_LENGTH, "NULL");
	}
	else
	{
		snprintf(stopsendtime_str, DB_MAX_MODE_LENGTH,
			"to_timestamp(%"PRIu64")",
			file->stopsendtime/1000000);
	}

	// --- STATUS ---
	switch (file->e_status)
	{
		case CREATE:
			strncpy(status_str, "Create", DB_MAX_STATE);
			break;
		case QUEUED:
			strncpy(status_str, "Queued", DB_MAX_STATE);
			break;
		case INTRANSIT:
			strncpy(status_str, "Intransit", DB_MAX_STATE);
			break;
		case ONHOLD:
			strncpy(status_str, "Onhold", DB_MAX_STATE);
			break;
		case DONE:
			strncpy(status_str, "Done", DB_MAX_STATE);
			break;
		case CHKSFAIL:
			strncpy(status_str, "Chksfail", DB_MAX_STATE);
			break;
		default:
			if (strlen(file->status) == 0)
				return 5;
			if ((strncmp(file->status, "Queued", 6) == 0) || 
					(strncmp(file->status, "Intransit", 9) == 0) ||	
					(strncmp(file->status, "Onhold", 6) == 0) ||	
					(strncmp(file->status, "Done", 4) == 0) ||
					(strncmp(file->status, "Chksfail", 8) == 0))
			{
				strncpy(status_str, file->status, DB_MAX_STATE);
				break;
			}
			else
			{
				return 6;
			}
			break;
	}
 
	query_len = snprintf(insert_query, BIG_QUERY_LEN,
		"INSERT INTO files "\
		"(streaminterface_id, filename, size, "\
		" priority, queuedtime, startsendtime, stopsendtime, "\
		" transferredbytes, status, comment) "\
		" VALUES "\
		"('%"PRIu64"', '%s', '%"PRIu64"', "\
		" '%"PRIu64"', '%s', '%s', '%s', "\
		" '%"PRIu64"', '%s', '%s')",
		file->streaminterface_id,
		filename, 
		file->size,
		file->priority,
		queuedtime_str,
		startsendtime_str,
		stopsendtime_str,
		file->transferredbytes,
		status_str,
		comment);

	if (query_len == BIG_QUERY_LEN || query_len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is to small(%d/%d)!",
			__FUNCTION__, query_len, BIG_QUERY_LEN);
	}

	// QUERY ID FOR JUST INSERTED LINE

	query_len = snprintf(select_query, BIG_QUERY_LEN,
		"SELECT id FROM files "\
		" WHERE streaminterface_id='%"PRIu64"' AND "\
		" filename='%s' AND "\
		" status='Queued'",
		file->streaminterface_id,
		filename);
	if (query_len == BIG_QUERY_LEN || query_len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is to small(%d/%d)!",
			__FUNCTION__, query_len, BIG_QUERY_LEN);
	}

	free(filename);
	free(comment);

#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "insert: %s\n", insert_query);
	fprintf(stderr, "select: %s\n", select_query);
#endif
	res = PQexec(*conn, insert_query);
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "%s: Answer not ok\n", __FUNCTION__);
#endif
		PQclear(res);
		return 6;
	}
	PQclear(res);

	res = PQexec(*conn, select_query);
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

	if (PQgetisnull(res, 0, 0))
	{
		PQclear(res);
		return 5;
	}

	file->id = atoll(PQgetvalue(res, 0, 0));
	PQclear(res);
	return 0;
}

int db_update_file_internal(PGconn **conn, char *query)
{
	PGresult *res;
	if (conn == NULL || query == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!", __FUNCTION__);
		return 1;
	}

	if (check_DB_connection_and_reconnect(conn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection.", __FUNCTION__);
		return 2;
	}

#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n", query);
#endif
	res = PQexec(*conn, query);
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "%s: Answer not ok\n", __FUNCTION__);
#endif
		PQclear(res);
		return 3;
	}

	if (atoi(PQcmdTuples(res)) < 1)
	{
		PQclear(res);
		return 4;
	}
	PQclear(res);
	return 0;
}

// only update the comment
int db_update_comment(PGconn **conn, s_file *file)
{
	char query[MEDIUM_QUERY_LEN];
	int query_len;
	char *comment;

	memset(query, 0, MEDIUM_QUERY_LEN);

	// --- COMMENT ---
	if (strlen(file->comment) == 0)
	{
		comment = pqescape("NULL");
	}
	else
	{
		comment = pqescape(file->comment);
	}

	if (comment == NULL)
	{
		return 4;
	}

	query_len = snprintf(query, MEDIUM_QUERY_LEN,
		"UPDATE files  "\
		" SET comment='%s' "\
		" WHERE id='%"PRIu64"'",
		comment,
		file->id);

	if (query_len == MEDIUM_QUERY_LEN || query_len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is to small(%d/%d)!",
			__FUNCTION__, query_len, MEDIUM_QUERY_LEN);
	}
	free(comment);

	return db_update_file_internal(conn, query);
}

// Only update size field
int db_update_file_size(PGconn **conn, s_file *file)
{
	char query[SMALL_QUERY_LEN];
	int query_len;
	memset(query, 0, SMALL_QUERY_LEN);

	query_len = snprintf(query, SMALL_QUERY_LEN,
		"UPDATE files SET "\
		" size='%"PRIu64"' "\
		" WHERE id='%"PRIu64"'",
		file->size,
		file->id);

	if (query_len == SMALL_QUERY_LEN || query_len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is to small(%d/%d)!",
			__FUNCTION__, query_len, SMALL_QUERY_LEN);
	}

	return db_update_file_internal(conn, query);
}

// Only update transferredbytes
int db_update_file_transferredbytes(PGconn **conn, s_file *file)
{
	char query[SMALL_QUERY_LEN];
	int query_len;
	memset(query, 0, SMALL_QUERY_LEN);

	query_len = snprintf(query, SMALL_QUERY_LEN,
		"UPDATE files SET "\
		" transferredbytes='%"PRIu64"' "\
		" WHERE id='%"PRIu64"'",
		file->transferredbytes,
		file->id);

	if (query_len == SMALL_QUERY_LEN || query_len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is to small(%d/%d)!",
			__FUNCTION__, query_len, SMALL_QUERY_LEN);
	}

	return db_update_file_internal(conn, query);
}

// Update status to QUEUED and set queuedtime to now()
int db_update_file_to_queued(PGconn **conn, s_file *file)
{
	char query[SMALL_QUERY_LEN];
	int query_len;

	memset(query, 0, SMALL_QUERY_LEN);

	query_len = snprintf(query, SMALL_QUERY_LEN,
		"UPDATE files SET "\
		" status='Queued', "\
		" queuedtime='now()' "\
		" WHERE id='%"PRIu64"'",
		file->id);

	if (query_len == SMALL_QUERY_LEN || query_len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is to small(%d/%d)!",
			__FUNCTION__, query_len, SMALL_QUERY_LEN);
	}

	return db_update_file_internal(conn, query);
}

// Update status to INTRANSIT and set startsendtime to now()
int db_update_file_to_intransit(PGconn **conn, s_file *file)
{
	char query[SMALL_QUERY_LEN];
	int query_len;

	memset(query, 0, SMALL_QUERY_LEN);

	query_len = snprintf(query, SMALL_QUERY_LEN,
		"UPDATE files SET "\
		" status='Intransit', "\
		" startsendtime='now()' "\
		" WHERE id='%"PRIu64"'",
		file->id);

	if (query_len == SMALL_QUERY_LEN || query_len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is to small(%d/%d)!",
			__FUNCTION__, query_len, SMALL_QUERY_LEN);
	}

	return db_update_file_internal(conn, query);
}

// Update status to ONHOLD and set stopsendtime to now()
int db_update_file_to_onhold(PGconn **conn, s_file *file)
{
	char query[SMALL_QUERY_LEN];
	int query_len;

	memset(query, 0, SMALL_QUERY_LEN);

	query_len = snprintf(query, SMALL_QUERY_LEN,
		"UPDATE files SET "\
		" status='Onhold', "\
		" transferredbytes='%"PRIu64"', "\
		" stopsendtime='now()' "\
		" WHERE id='%"PRIu64"'",
		file->id,
		file->transferredbytes);

	if (query_len == SMALL_QUERY_LEN || query_len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is to small(%d/%d)!",
			__FUNCTION__, query_len, SMALL_QUERY_LEN);
	}

	return db_update_file_internal(conn, query);
}

// Update status to DONE and set stopsendtime to now()
int db_update_file_to_done(PGconn **conn, s_file *file)
{
	char query[SMALL_QUERY_LEN];
	int query_len;

	memset(query, 0, SMALL_QUERY_LEN);

	query_len = snprintf(query, SMALL_QUERY_LEN,
		"UPDATE files SET "\
		" status='Done', "\
		" stopsendtime='now()' "\
		" WHERE id='%"PRIu64"'",
		file->id);

	if (query_len == SMALL_QUERY_LEN || query_len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is to small(%d/%d)!",
			__FUNCTION__, query_len, SMALL_QUERY_LEN);
	}

	return db_update_file_internal(conn, query);
}

// Update status to CHKSFAIL and set stopsendtime to now()
int db_update_file_to_chksfail(PGconn **conn, s_file *file)
{
	char query[SMALL_QUERY_LEN];
	int query_len;

	memset(query, 0, SMALL_QUERY_LEN);

	query_len = snprintf(query, SMALL_QUERY_LEN,
		"UPDATE files SET "\
		" status='Chksfail', "\
		" stopsendtime='now()' "\
		" WHERE id='%"PRIu64"'",
		file->id);

	if (query_len == SMALL_QUERY_LEN || query_len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is to small(%d/%d)!",
			__FUNCTION__, query_len, SMALL_QUERY_LEN);
	}

	return db_update_file_internal(conn, query);
}

int db_update_file(PGconn **conn, s_file *file)
{
	char query[BIG_QUERY_LEN];
	int query_len;
	char *comment;
	char startsendtime_str[DB_MAX_MODE_LENGTH],
		stopsendtime_str[DB_MAX_MODE_LENGTH],
		status_str[DB_MAX_STATE];

	memset(query, 0, BIG_QUERY_LEN);
	memset(startsendtime_str, 0, DB_MAX_MODE_LENGTH);
	memset(stopsendtime_str, 0, DB_MAX_MODE_LENGTH);
	memset(status_str, 0, DB_MAX_STATE);

	// Fix the dynamic allocated memory first
	// --- COMMENT ---
	if (strlen(file->comment) == 0)
	{
		comment = pqescape("NULL");
	}
	else
	{
		comment = pqescape(file->comment);
	}

	if (comment == NULL)
	{
		return 4;
	}

	// The fixed numbers that can be abscent for example if time is 0 => 1970
	// --- STARTSENDTIME ---
	if (file->startsendtime == 0)
	{
		snprintf(startsendtime_str, DB_MAX_MODE_LENGTH, " ");
	}
	else
	{
		snprintf(startsendtime_str, DB_MAX_MODE_LENGTH,
			" startsendtime=to_timestamp(%"PRIu64"), ",
			file->startsendtime/1000000);
	}

	// --- STOPSENDTIME ---
	if (file->stopsendtime == 0)
	{
		snprintf(stopsendtime_str, DB_MAX_MODE_LENGTH, " ");
	}
	else
	{
		snprintf(stopsendtime_str, DB_MAX_MODE_LENGTH,
			" stopsendtime=to_timestamp(%"PRIu64"), ",
			file->stopsendtime/1000000);
	}

	// --- STATUS ---
	switch (file->e_status)
	{
		case CREATE:
			strncpy(status_str, "Create", DB_MAX_STATE);
			break;
		case QUEUED:
			strncpy(status_str, "Queued", DB_MAX_STATE);
			break;
		case INTRANSIT:
			strncpy(status_str, "Intransit", DB_MAX_STATE);
			break;
		case ONHOLD:
			strncpy(status_str, "Onhold", DB_MAX_STATE);
			break;
		case DONE:
			strncpy(status_str, "Done", DB_MAX_STATE);
			break;
		case CHKSFAIL:
			strncpy(status_str, "Chksfail", DB_MAX_STATE);
			break;
		default:
			if (strlen(file->status) == 0)
				return 5;
			if ((strncmp(file->status, "Queued", 6) == 0) || 
					(strncmp(file->status, "Intransit", 9) == 0) ||	
					(strncmp(file->status, "Onhold", 6) == 0) ||	
					(strncmp(file->status, "Done", 4) == 0) ||
					(strncmp(file->status, "Chksfail", 8) == 0))
			{
				strncpy(status_str, file->status, DB_MAX_STATE);
				break;
			}
			else
			{
				return 6;
			}
			break;
	}
	
	query_len = snprintf(query, BIG_QUERY_LEN,
		"UPDATE files SET "\
		" %s %s "\
		" transferredbytes='%"PRIu64"', status='%s', comment='%s' "\
		" WHERE id='%"PRIu64"'",
		startsendtime_str,
		stopsendtime_str,
		file->transferredbytes,
		status_str,
		comment,
		file->id);

	if (query_len == BIG_QUERY_LEN || query_len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is to small(%d/%d)!",
			__FUNCTION__, query_len, BIG_QUERY_LEN);
	}

	free(comment);

	return db_update_file_internal(conn, query);
}
