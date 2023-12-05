#include "dbcommon.h"
#include "dbsysteminfo.h"

int db_fetch_systeminfo_internal(const std::string &query, PGconn * dbconn, DBSystemInfo::Data &data)
{
	PGresult *res;
	int numtuples;

	if (dbconn == NULL)
	{
		syslog(LOG_CRIT, "db_fetch_systeminfo_internal connection invalid");
		return 1;
	}
	if (check_DB_connection_and_reconnect(&dbconn))
	{
		syslog(LOG_CRIT, "db_fetch_systeminfo_internal cant reconnect DB connection");
		return 1;
	}

	res = PQexec(dbconn, query.c_str());
	
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "db_fetch_systeminfo_internal: Answer not ok\n");
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
		data.dbversion = 0;
	}
	else
	{
		data.dbversion = atoi(PQgetvalue(res, 0, 1));
	}

	if (PQgetisnull(res, 0, 2))
	{
		data.systemname = "";
	}
	else
	{
		data.systemname = PQgetvalue(res, 0, 2);
	}

	if (PQgetisnull(res, 0, 3))
	{
		data.statdb_remote_url = "";
	}
	else
	{
		data.statdb_remote_url = PQgetvalue(res, 0, 3);
	}

	if (PQgetisnull(res, 0, 4))
	{
		data.statdb_sleeptime = 0;
	}
	else
	{
		data.statdb_sleeptime = atoi(PQgetvalue(res, 0, 4));
	}
	
	if (PQgetisnull(res, 0, 5))
	{
		data.statdb_chunksize = 0;
	}
	else
	{
		data.statdb_chunksize = atoi(PQgetvalue(res, 0, 5));
	}
	
	PQclear(res);
	return 0;
}

int db_fetch_password_internal(const std::string &query, PGconn * dbconn, DBSystemInfo::Data &data)
{
	PGresult *res;
	int numtuples;

	if (dbconn == NULL)
	{
		syslog(LOG_CRIT, "db_fetch_systeminfo_internal connection invalid");
		return 1;
	}
	if (check_DB_connection_and_reconnect(&dbconn))
	{
		syslog(LOG_CRIT, "db_fetch_systeminfo_internal cant reconnect DB connection");
		return 1;
	}

	res = PQexec(dbconn, query.c_str());
	
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "db_fetch_systeminfo_internal: Answer not ok\n");
		PQclear(res);
	}

	numtuples = PQntuples(res);
	if(numtuples == 0)
	{
		return 1;
	}

	if (PQgetisnull(res, 0, 0))
	{
		data.labelapipassw = "";
	}
	else
	{
		data.labelapipassw = PQgetvalue(res, 0, 0);
	}
	
	PQclear(res);
	return 0;
}
/*
int DBSystemInfo::db_update_systeminfo_internal(const std::string &query)
{
	PGresult *res;
	if (this->dbconn == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!", __FUNCTION__);
		return 1;
	}

	if (check_DB_connection_and_reconnect(this->dbconn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection.", __FUNCTION__);
		return 1;
	}

	res = PQexec(*this->dbconn, query.c_str());
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
*/

int DBSystemInfo::db_fetch(PGconn *dbconn, DBSystemInfo::Data *data)
{
	std::string query = "SELECT id, dbversion, systemname, statdb_remote_url, statdb_sleeptime, statdb_chunksize FROM systeminfo";
	return db_fetch_systeminfo_internal(query, dbconn, *data);
}


int DBSystemInfo::db_fetch_password(PGconn *dbconn, std::string &password)
{
	std::string query = "SELECT labelapipassw FROM systeminfo";
	DBSystemInfo::Data data;
	if(!db_fetch_systeminfo_internal(query, dbconn, data))
	{
		password = data.labelapipassw;
		return 0;
	}
	else
	{
		return 1;
	}
}



