#include "dbcommon.h"
#include "dbvirtualsource.h"
#include <string>
#include <vector>

int db_fetch_virtualsource_internal(const std::string &query, PGconn * dbconn, DBVirtualSource::Data &data)
{
	PGresult *res;
	int numtuples;

	if (dbconn == NULL)
	{
		syslog(LOG_CRIT, "db_fetch_virtualsource_internal connection invalid");
		return 1;
	}
	if (check_DB_connection_and_reconnect(&dbconn))
	{
		syslog(LOG_CRIT, "db_fetch_virtualsource_internal cant reconnect DB connection");
		return 1;
	}

	res = PQexec(dbconn, query.c_str());
	
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "db_fetch_virtualsource_internal: Answer not ok\n");
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
		data.revision = 0;
	}
	else
	{
		data.revision = atoi(PQgetvalue(res, 0, 1));
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
		data.httpurl = "";
	}
	else
	{
		data.httpurl = PQgetvalue(res, 0, 3);
	}

	if (PQgetisnull(res, 0, 4))
	{
		data.url = "";
	}
	else
	{
		data.url = PQgetvalue(res, 0, 4);
	}
	
	if (PQgetisnull(res, 0, 5))
	{
		data.address = "";
	}
	else
	{
		data.address = PQgetvalue(res, 0, 5);
	}
	if (PQgetisnull(res, 0, 6))
	{
		data.port = 0;
	}
	else
	{
		data.port = atoi(PQgetvalue(res, 0, 6));
	}
	if (PQgetisnull(res, 0, 7))
	{
		data.virtualsourcegroup_id = 0;
	}
	else
	{
		data.virtualsourcegroup_id = atoi(PQgetvalue(res, 0, 7));
	}
	
	PQclear(res);
	return 0;
}

int db_fetch_virtualsources_internal(const std::string &query, PGconn *dbconn, std::vector<DBVirtualSource::Data> &data)
{
	PGresult *res;
	int numtuples;

	if (dbconn == NULL)
	{
		syslog(LOG_CRIT, "db_fetch_virtualsources_internal connection invalid");
		return 1;
	}
	if (check_DB_connection_and_reconnect(&dbconn))
	{
		syslog(LOG_CRIT, "db_fetch_virtualsources_internal cant reconnect DB connection");
		return 1;
	}

	res = PQexec(dbconn, query.c_str());
	
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "db_fetch_virtualsources_internal: Answer not ok\n");
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
			data[i].revision = 0;
		}
		else
		{
			data[i].revision = atoi(PQgetvalue(res, i, 1));
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
			data[i].httpurl = "";
		}
		else
		{
			data[i].httpurl = PQgetvalue(res, i, 3);
		}

		if (PQgetisnull(res, i, 4))
		{
			data[i].url = "";
		}
		else
		{
			data[i].url = PQgetvalue(res, i, 4);
		}
	
		if (PQgetisnull(res, i, 5))
		{
			data[i].address = "";
		}
		else
		{
			data[i].address = PQgetvalue(res, i, 5);
		}
		if (PQgetisnull(res, i, 6))
		{
			data[i].port = 0;
		}
		else
		{
			data[i].port = atoi(PQgetvalue(res, i, 6));
		}
		if (PQgetisnull(res, i, 7))
		{
			data[i].virtualsourcegroup_id = 0;
		}
		else
		{
			data[i].virtualsourcegroup_id = atoi(PQgetvalue(res, i, 7));
		}
	}
	PQclear(res);
	return 0;
}

/*
int DBSystemInfo::db_update_virtualsource_internal(const std::string &query)
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


int DBVirtualSource::db_fetch_by_id(PGconn *dbconn, uint64_t id, DBVirtualSource::Data *data)
{
	std::string query = "SELECT id, revision, name, httpurl, url, address, port, virtualsourcegroup_id FROM virtualsources WHERE id = " + std::to_string(id);
	return db_fetch_virtualsource_internal(query, dbconn, *data);
}

int DBVirtualSource::db_fetch_all(PGconn *dbconn, std::vector<DBVirtualSource::Data> *data)
{
	std::string query = "SELECT id, revision, name, httpurl, url, address, port, virtualsourcegroup_id FROM virtualsources";
	return db_fetch_virtualsources_internal(query, dbconn, *data);
}


