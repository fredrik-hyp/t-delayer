#include "dbcommon.h"
#include "dbstreaminterface.h"

int db_fetch_streaminterface_internal(const std::string &query, PGconn * dbconn, DBStreaminterface::Data &data)
{
	PGresult *res;
	int numtuples;

	if (dbconn == NULL)
	{
		syslog(LOG_CRIT, "db_fetch_streaminterface_internal connection invalid");
		return 1;
	}
	if (check_DB_connection_and_reconnect(&dbconn))
	{
		syslog(LOG_CRIT, "db_fetch_streaminterface_internal cant reconnect DB connection");
		return 1;
	}

	res = PQexec(dbconn, query.c_str());
	
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "db_fetch_streaminterface_internal: Answer not ok\n");
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
		data.interfacetype = "";
	}
	else
	{
		data.interfacetype = PQgetvalue(res, 0, 1);
	}

	if (PQgetisnull(res, 0, 2))
	{
		data.name = "";
	}
	else
	{
		data.name = PQgetvalue(res, 0, 2);
	}

	if (PQgetisnull(res, 0, 3))
	{
		data.usertext = "";
	}
	else
	{
		data.usertext = PQgetvalue(res, 0, 3);
	}

	PQclear(res);
	return 0;
}

int db_fetch_streaminterfaces_internal(const std::string &query, PGconn * dbconn, std::vector<DBStreaminterface::Data> &data)
{
	PGresult *res;
	int numtuples;

	if (dbconn == NULL)
	{
		syslog(LOG_CRIT, "db_fetch_streaminterfaces_internal connection invalid");
		return 1;
	}
	if (check_DB_connection_and_reconnect(&dbconn))
	{
		syslog(LOG_CRIT, "db_fetch_streaminterfaces_internal cant reconnect DB connection");
		return 1;
	}

	res = PQexec(dbconn, query.c_str());
	
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "db_fetch_streaminterfaces_internal: Answer not ok\n");
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
			data[i].interfacetype = "";
		}
		else
		{
			data[i].interfacetype = PQgetvalue(res, i, 1);
		}

		if (PQgetisnull(res, i, 2))
		{
			data[i].name = "";
		}
		else
		{
			data[i].name = PQgetvalue(res, i, 2);
		}

		if (PQgetisnull(res, i, 3))
		{
			data[i].usertext = "";
		}
		else
		{
			data[i].usertext = PQgetvalue(res, i, 3);
		}
	}

	PQclear(res);
	return 0;
}
/*
int db_update_streaminterface_internal(const std::string &query)
{
	PGresult *res;
	if (this->dbconn == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!", __FUNCTION__);
		return 1;
	}

	if (check_DB_connection_and_reconnect(&this->dbconn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection.", __FUNCTION__);
		return 1;
	}

	res = PQexec(this->dbconn, query.c_str());
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

int DBStreaminterface::db_fetch_by_id(PGconn *dbconn, uint64_t id, DBStreaminterface::Data *data)
{
	std::string query = "SELECT id, interfacetype, name, usertext FROM streaminterfaces WHERE id = " + std::to_string(id);
	return db_fetch_streaminterface_internal(query, dbconn, *data);
}

int DBStreaminterface::db_fetch_all(PGconn *dbconn, std::vector<DBStreaminterface::Data> *data)
{
	std::string query = "SELECT id, interfacetype, name, usertext FROM streaminterfaces";
	return db_fetch_streaminterfaces_internal(query, dbconn, *data);
}



