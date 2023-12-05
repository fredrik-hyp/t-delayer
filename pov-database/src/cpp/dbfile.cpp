#include "dbcommon.h"
#include "dbfile.h"

int db_fetch_file_internal(PGconn * dbconn, const std::string &query, DBFile::Data &data)
{
	PGresult *res;
	int numtuples;

	if (dbconn == NULL)
	{
		syslog(LOG_CRIT, "db_fetch_file_internal connection invalid");
		return 1;
	}
	if (check_DB_connection_and_reconnect(&dbconn))
	{
		syslog(LOG_CRIT, "db_fetch_file_internal cant reconnect DB connection");
		return 1;
	}

	res = PQexec(dbconn, query.c_str());
	
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "db_fetch_file_internal: Answer not ok\n");
		PQclear(res);
	}

	numtuples = PQntuples(res);
	if(numtuples == 0)
	{
		return 1;
	}

	if (PQgetisnull(res, 0, 0))
	{
		data.id = 0;
	}
	else
	{
		data.id = atoll(PQgetvalue(res, 0, 0));
	}

	if (PQgetisnull(res, 0, 1))
	{
		data.streaminterface_id = 0;
	}
	else
	{
		data.streaminterface_id = atoll(PQgetvalue(res, 0, 1));
	}

	if (PQgetisnull(res, 0, 2))
	{
		data.filename = "";
	}
	else
	{
		data.filename = (PQgetvalue(res, 0, 2));
	}

	if (PQgetisnull(res, 0, 3))
	{
		data.size = 0;
	}
	else
	{
		data.size = atoll(PQgetvalue(res, 0, 3));
	}

	if (PQgetisnull(res, 0, 4))
	{
		data.priority = 0;
	}
	else
	{
		data.priority = atoll(PQgetvalue(res, 0, 4));
	}
	
	if (PQgetisnull(res, 0, 5))
	{
		data.queuedtime = "";
	}
	else
	{
		data.queuedtime = PQgetvalue(res, 0, 5);
	}
	
	if (PQgetisnull(res, 0, 6))
	{
		data.startsendtime = "";
	}
	else
	{
		data.startsendtime = PQgetvalue(res, 0, 6);
	}

	if (PQgetisnull(res, 0, 7))
	{
		data.stopsendtime = "";
	}
	else
	{
		data.stopsendtime = PQgetvalue(res, 0, 7);
	}

	if (PQgetisnull(res, 0, 8))
	{
		data.transferredbytes = 0;
	}
	else
	{
		data.transferredbytes = atoll(PQgetvalue(res, 0, 8));
	}

	if (PQgetisnull(res, 0, 9))
	{
		data.status = "Null";
		data.e_status = QUEUED; 
	}
	else
	{
		data.status = PQgetvalue(res, 0, 9);
		if (data.status == "Create")
		{
			data.e_status = CREATE;
		}
		else if (data.status == "Queued")
		{
			data.e_status = QUEUED;
		}
		else if (data.status == "Intransit")
		{
			data.e_status = INTRANSIT;
		}
		else if (data.status == "Onhold")
		{
			data.e_status = ONHOLD;
		}
		else if (data.status == "Chksfail")
		{
			data.e_status = CHKSFAIL;
		}
		else if (data.status == "Done")
		{
			data.e_status = DONE;
		}
		else
		{
			data.status = "UnknownStatus";
		}

		if (PQgetisnull(res, 0, 10))
		{
			data.comment = "";
		}
		else
		{
			data.comment = PQgetvalue(res, 0, 10);
		}

		if (PQgetisnull(res, 0, 11))
		{
			memset(data.md5sum, 0, MD5_DIGEST_LENGTH);
		}
		else
		{
			//data.md5sum = PQgetvalue(res, 0, 11);
			memcpy(data.md5sum, PQgetvalue(res, 0, 11), MD5_DIGEST_LENGTH);
		}
	}
	PQclear(res);
	return 0;
}

int db_fetch_files_internal(PGconn *dbconn, const std::string &query, std::vector<DBFile::Data> &data)
{
	PGresult *res;
	int numtuples;

	if (dbconn == NULL)
	{
		syslog(LOG_CRIT, "db_fetch_files_internal connection invalid");
		return 1;
	}
	if (check_DB_connection_and_reconnect(&dbconn))
	{
		syslog(LOG_CRIT, "db_fetch_files_internal cant reconnect DB connection");
		return 1;
	}

	res = PQexec(dbconn, query.c_str());
	
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "db_fetch_files_internal: Answer not ok\n");
		PQclear(res);
	}

	numtuples = PQntuples(res);
	data.resize(numtuples);
	
	if(numtuples == 0)
	{
		return 1;
	}
	
	for (int i = 0; i < numtuples; i++)
	{
		if (PQgetisnull(res, i, 0))
		{
			data[i].id = 0;
		}
		else
		{
			data[i].id = atoll(PQgetvalue(res, i, 0));
		}

		if (PQgetisnull(res, i, 1))
		{
			data[i].streaminterface_id = 0;
		}
		else
		{
			data[i].streaminterface_id = atoll(PQgetvalue(res, i, 1));
		}
		if (PQgetisnull(res, i, 2))
		{
			data[i].filename = "";
		}
		else
		{
			data[i].filename = (PQgetvalue(res, i, 2));
		}

		if (PQgetisnull(res, i, 3))
		{
			data[i].size = 0;
		}
		else
		{
			data[i].size = atoll(PQgetvalue(res, i, 3));
		}

		if (PQgetisnull(res, i, 4))
		{
			data[i].priority = 0;
		}
		else
		{
			data[i].priority = atoll(PQgetvalue(res, i, 4));
		}
	
		if (PQgetisnull(res, i, 5))
		{
			data[i].queuedtime = "";
		}
		else
		{
			data[i].queuedtime = PQgetvalue(res, i, 5);
		}
	
		if (PQgetisnull(res, i, 6))
		{
			data[i].startsendtime = "";
		}
		else
		{
			data[i].startsendtime = PQgetvalue(res, i, 6);
		}

		if (PQgetisnull(res, i, 7))
		{
			data[i].stopsendtime = "";
		}
		else
		{
			data[i].stopsendtime = PQgetvalue(res, i, 7);
		}

		if (PQgetisnull(res, i, 8))
		{
			data[i].transferredbytes = 0;
		}
		else
		{
			data[i].transferredbytes = atoll(PQgetvalue(res, i, 8));
		}

		if (PQgetisnull(res, i, 9))
		{
			data[i].status = "NoStatus";
		}
		else
		{
			data[i].status = PQgetvalue(res, i, 9);
		}

		if (PQgetisnull(res, i, 10))
		{
			data[i].comment = "";
		}
		else
		{
			data[i].comment = PQgetvalue(res, i, 10);
		}
	
		if (PQgetisnull(res, i, 11))
		{
			//data[i].md5sum = "";
			memset(data[i].md5sum, 0, MD5_DIGEST_LENGTH);
		}
		else
		{
			//data[i].md5sum = PQgetvalue(res, i, 11);
			memcpy(data[i].md5sum, PQgetvalue(res, i, 11), MD5_DIGEST_LENGTH);
		}
	}
	PQclear(res);
	return 0;
}

int db_query_file_internal(PGconn *dbconn, const std::string &query)
{
	PGresult *res;
	if (dbconn == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!", __FUNCTION__);
		return 1;
	}

	if (check_DB_connection_and_reconnect(&dbconn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection.", __FUNCTION__);
		return 1;
	}

	res = PQexec(dbconn, query.c_str());
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
		PQclear(res);
		return 1;
	}

	if (atoi(PQcmdTuples(res)) < 1)
	{
		PQclear(res);
		return 1;
	}
	PQclear(res);
	return 0;
}

int db_query_file_internal_prepared(PGconn *dbconn , std::vector<const char*> &paramvalues, const std::string &query)
{
	PGresult *res;
	if (dbconn == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!", __FUNCTION__);
		return 1;
	}

	if (check_DB_connection_and_reconnect(&dbconn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection.", __FUNCTION__);
		return 2;
	}

	int noOfValues = paramvalues.size();

	PQprepare(dbconn, "", query.c_str(), noOfValues, NULL);

	int paramLengths[noOfValues];
	for(int i = 0; i < noOfValues; i++)
	{
		paramLengths[i] = strlen(paramvalues[i]);
	}
	
	res = PQexecPrepared(dbconn, "", noOfValues, &paramvalues[0], paramLengths, NULL, 1);

	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
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


int DBFile::db_fetch_by_id(PGconn *dbconn, uint64_t id, DBFile::Data *data)
{
	std::string query = "SELECT id, streaminterface_id, filename, size, priority, queuedtime, startsendtime, stopsendtime, transferredbytes, status, comment, md5sum FROM files WHERE id=" + std::to_string(id);
	return db_fetch_file_internal(dbconn, query, *data);
}

int DBFile::db_fetch_by_id_epoch_time(PGconn *dbconn, uint64_t id, DBFile::Data *data)
{
	std::string query = "SELECT id, streaminterface_id, filename, size, priority, extract(epoch from queuedtime), extract(epoch from startsendtime), extract(epoch from stopsendtime), transferredbytes, status, comment, md5sum FROM files WHERE id=" + std::to_string(id);
	return db_fetch_file_internal(dbconn, query, *data);
}

int DBFile::db_fetch_by_streaminterface_id(PGconn *dbconn, uint64_t id, std::vector<DBFile::Data> *data)
{
	std::string query = "SELECT id, streaminterface_id, filename, size, priority, queuedtime, startsendtime, stopsendtime, transferredbytes, status, comment, md5sum FROM files WHERE streaminterface_id=" + std::to_string(id);
	return db_fetch_files_internal(dbconn, query, *data);
}

int DBFile::db_fetch_all(PGconn *dbconn, std::vector<DBFile::Data> *data)
{
	std::string query = "SELECT id, streaminterface_id, filename, size, priority, queuedtime, startsendtime, stopsendtime, transferredbytes, status, comment, md5sum FROM files";
	return db_fetch_files_internal(dbconn, query, *data);
}
/*
int DBFile::db_fetch_most_recent(PGconn *dbconn, std::vector<DBFile::Data> *data)
{
	std::string query = "SELECT DISTINCT ON (filename) id, streaminterface_id, filename, size, priority, queuedtime, startsendtime, stopsendtime, transferredbytes, status, comment, md5sum FROM files ORDER BY filename, stopsendtime DESC";
	return db_fetch_files_internal(dbconn, query, *data);
}
*/
int DBFile::db_fetch_most_recent_on_receiver(PGconn *dbconn, std::vector<DBFile::Data> *data)
{
std::string query = "SELECT DISTINCT ON (filename) id, streaminterface_id, filename, size, priority, queuedtime, startsendtime, stopsendtime, transferredbytes, status, comment md5sum FROM files WHERE queuedtime IS NULL ORDER BY filename,  GREATEST (startsendtime, stopsendtime) DESC";
	return db_fetch_files_internal(dbconn, query, *data);
}

int DBFile::db_fetch_most_recent_on_transmitter(PGconn *dbconn, std::vector<DBFile::Data> *data)
{
std::string query = "(SELECT DISTINCT ON (streaminterface_id, filename) id, streaminterface_id, filename, size, priority, queuedtime, startsendtime, stopsendtime, transferredbytes, status, comment, md5sum FROM files WHERE startsendtime IS NOT NULL AND queuedtime IS NOT NULL ORDER BY streaminterface_id, filename,  GREATEST (startsendtime, stopsendtime) DESC) UNION (SELECT id, streaminterface_id, filename, size, priority, queuedtime, startsendtime, stopsendtime, transferredbytes, status, comment, md5sum FROM files WHERE queuedtime IS NOT NULL AND (status = 'Create' OR status = 'Queued' OR status = 'Onhold' OR status = 'Intransit'))";
	return db_fetch_files_internal(dbconn, query, *data);
}

int DBFile::db_fetch_by_status(PGconn *dbconn, const std::string &str, std::vector<DBFile::Data> *data)
{	
	std::string query = "SELECT id, streaminterface_id, filename, size, priority, queuedtime, startsendtime, stopsendtime, transferredbytes, status, comment, md5sum FROM files WHERE status='" + str + '\'';
	return db_fetch_files_internal(dbconn, query, *data);
}

int DBFile::db_set_priority_by_id(PGconn *dbconn, uint64_t id, int filepriority)
{
	std::string query = "UPDATE files SET priority=" + std::to_string(filepriority) + "WHERE id=" + std::to_string(id);
	return db_query_file_internal(dbconn, query);
}

int DBFile::db_set_priority_for_folder(PGconn * dbconn, std::string foldername, uint64_t interfaceid, int filepriority)
{
	std::string query = "UPDATE files SET priority = $1 WHERE filename LIKE $2 AND streaminterface_id = $3;";

	std::string param0 = std::to_string(filepriority);
	std::string param2 = std::to_string(interfaceid);

	std::vector<const char*> paramvalues(3);
	paramvalues[0] = param0.c_str();
	paramvalues[1] = (foldername + "/%").c_str();
	paramvalues[2] = param2.c_str();

	return db_query_file_internal_prepared(dbconn, paramvalues, query);
}

int DBFile::db_set_status_by_id(PGconn *dbconn, uint64_t id, const std::string &status)
{
	std::string query = "UPDATE files set status='" + status + "'WHERE id=" + std::to_string(id);
	return db_query_file_internal(dbconn, query);
}

int DBFile::db_add_file(PGconn *dbconn, const DBFile::Data *data)
{
	std::string query = "INSERT INTO files (streaminterface_id, filename, size, priority, status, comment) VALUES ($1, $2, $3, $4, $5, $6);";

	std::vector<const char*> paramvalues(6);

	std::string param0 = std::to_string(data->streaminterface_id);
	std::string param2 = std::to_string(data->size);
	std::string param3 = std::to_string(data->priority);
	
	paramvalues[0] = param0.c_str();
	paramvalues[1] = data->filename.c_str();
	paramvalues[2] = param2.c_str();
	paramvalues[3] = param3.c_str();
	paramvalues[4] = "Queued";
	paramvalues[5] = data->comment.c_str();

	return db_query_file_internal_prepared(dbconn, paramvalues, query);
	
}

int DBFile::db_delete_file_by_name_and_interface(PGconn *dbconn, const std::string &filename, uint64_t streaminterface_id)
{
	std::string query = "DELETE FROM files WHERE streaminterface_id = $1 AND filename = $2;";
	
	std::vector<const char*> paramvalues(2);

	std::string param0 = std::to_string(streaminterface_id);
	
	paramvalues[0] = param0.c_str();
	paramvalues[1] = filename.c_str();
	
	return db_query_file_internal_prepared(dbconn, paramvalues, query);
}

int DBFile::db_clear_all(PGconn *dbconn)
{
	std::string query = "DELETE from files";
	return db_query_file_internal(dbconn, query);
}

int DBFile::db_clear_interface(PGconn *dbconn, uint64_t id)
{
	std::string query = "DELETE from files WHERE streaminterface_id=" + std::to_string(id);
	return db_query_file_internal(dbconn, query);
}

int DBFile::db_clear_file(PGconn *dbconn, uint64_t id)
{
	std::string query = "DELETE from files WHERE id=" + std::to_string(id);
	return db_query_file_internal(dbconn, query);
}
/*
int DBFile::db_check_if_inactive(PGconn *dbconn, const std::string &filename, uint64_t streaminterface_id)
{
	std::string query = "SELECT * FROM files WHERE filename = $1 AND streaminterface_id = $2 AND (status = $3 OR status = $4 OR status = $5 OR status = $6);";
	
	std::vector<const char*> paramvalues(6);
	
	std::string param1 = std::to_string(streaminterface_id);	
	
	paramvalues[0] = filename.c_str();
	paramvalues[1] = param1.c_str();
	paramvalues[2] = "Onhold";
	paramvalues[3] = "Intransit";
	paramvalues[4] = "Queued";
	paramvalues[5] = "Create";

	return db_query_file_internal_prepared(dbconn, paramvalues, query);
}
*/
