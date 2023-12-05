#include "dbcommon.h"
#include "dbstreamstat.h"
#include <string>
#include <vector>

int db_fetch_streamstat_internal(const std::string &query, PGconn * dbconn, DBStreamStat::Data &data)
{
	PGresult *res;
	int numtuples;

	if (dbconn == NULL)
	{
		syslog(LOG_CRIT, "db_fetch_streamstat_internal connection invalid");
		return 1;
	}
	if (check_DB_connection_and_reconnect(&dbconn))
	{
		syslog(LOG_CRIT, "db_fetch_streamstat_internal cant reconnect DB connection");
		return 1;
	}

	res = PQexec(dbconn, query.c_str());
	
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "db_fetch_streamstat_internal: Answer not ok\n");
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
		data.streaminterface_id = atoi(PQgetvalue(res, 0, 1));
	}

	if (PQgetisnull(res, 0, 2))
	{
		data.timestamp = "";
	}
	else
	{
		data.timestamp = PQgetvalue(res, 0, 2);
	}

	if (PQgetisnull(res, 0, 3))
	{
		data.streammode = "";
	}
	else
	{
		data.streammode = PQgetvalue(res, 0, 3);
	}

	if (PQgetisnull(res, 0, 4))
	{
		data.source_address = "";
	}
	else
	{
		data.source_address = PQgetvalue(res, 0, 4);
	}
	
	if (PQgetisnull(res, 0, 5))
	{
		data.source_port = 0;
	}
	else
	{
		data.source_port = atoi(PQgetvalue(res, 0, 5));
	}
	if (PQgetisnull(res, 0, 6))
	{
		data.sink_address = "";
	}
	else
	{
		data.sink_address = PQgetvalue(res, 0, 6);
	}
	if (PQgetisnull(res, 0, 7))
	{
		data.sink_port = 0;
	}
	else
	{
		data.sink_port = atoi(PQgetvalue(res, 0, 7));
	}
	if (PQgetisnull(res, 0, 8))
	{
		data.drops = 0;
	}
	else
	{
		data.drops = atoi(PQgetvalue(res, 0, 8));
	}
	if (PQgetisnull(res, 0, 9))
	{
		data.bitrate = 0;
	}
	else
	{
		data.bitrate = atoi(PQgetvalue(res, 0, 9));
	}
	if (PQgetisnull(res, 0, 10))
	{
		data.bitrate_in = 0;
	}
	else
	{
		data.bitrate_in = atoi(PQgetvalue(res, 0, 10));
	}
	if (PQgetisnull(res, 0, 11))
	{
		data.bitrate_out = 0;
	}
	else
	{
		data.bitrate_out = atoi(PQgetvalue(res, 0, 11));
	}
	if (PQgetisnull(res, 0, 12))
	{
		data.resendcounter = 0;
	}
	else
	{
		data.resendcounter = atoi(PQgetvalue(res, 0, 12));
	}
	if (PQgetisnull(res, 0, 13))
	{
		data.rudprtt = 0;
	}
	else
	{
		data.rudprtt = atoi(PQgetvalue(res, 0, 13));
	}
	if (PQgetisnull(res, 0, 14))
	{
		data.pathnumber = 0;
	}
	else
	{
		data.pathnumber = atoi(PQgetvalue(res, 0, 14));
	}
	if (PQgetisnull(res, 0, 15))
	{
		data.pathname = "";
	}
	else
	{
		data.pathname = PQgetvalue(res, 0, 15);
	}
	if (PQgetisnull(res, 0, 16))
	{
		data.linkdrops = 0;
	}
	else
	{
		data.linkdrops = atoi(PQgetvalue(res, 0, 16));
	}
	if (PQgetisnull(res, 0, 17))
	{
		//data.multipath = 0; // true/false??
	}
	else
	{
		if (strcmp(PQgetvalue(res, 0, 17), "t") == 0)
		{
			data.multipath = 1;
		}
		else
		{
			data.multipath = 0;
		}
	}
	if (PQgetisnull(res, 0, 18))
	{
		//data.mploadbalance = 0; // true/false??
	}
	else
	{
		if (strcmp(PQgetvalue(res, 0, 18), "t") == 0)
		{
			data.mploadbalance = 1;
		}
		else
		{
			data.mploadbalance = 0;
		}
	}
	
	PQclear(res);
	return 0;
}

int db_fetch_streamstats_internal(const std::string &query, PGconn *dbconn, std::vector<DBStreamStat::Data> &data)
{
	PGresult *res;
	int numtuples;

	if (dbconn == NULL)
	{
		syslog(LOG_CRIT, "db_fetch_streamstats_internal connection invalid");
		return 1;
	}
	if (check_DB_connection_and_reconnect(&dbconn))
	{
		syslog(LOG_CRIT, "db_fetch_streamstats_internal cant reconnect DB connection");
		return 1;
	}

	res = PQexec(dbconn, query.c_str());
	
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "db_fetch_streamstats_internal: Answer not ok\n");
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
			data[i].streaminterface_id = atoi(PQgetvalue(res, i, 1));
		}

		if (PQgetisnull(res, i, 2))
		{
			data[i].timestamp = "";
		}
		else
		{
			data[i].timestamp = PQgetvalue(res, i, 2);
		}

		if (PQgetisnull(res, i, 3))
		{
			data[i].streammode = "";
		}
		else
		{
			data[i].streammode = PQgetvalue(res, i, 3);
		}

		if (PQgetisnull(res, i, 4))
		{
			data[i].source_address = "";
		}
		else
		{
			data[i].source_address = PQgetvalue(res, i, 4);
		}
	
		if (PQgetisnull(res, i, 5))
		{
			data[i].source_port = 0;
		}
		else
		{
			data[i].source_port = atoi(PQgetvalue(res, i, 5));
		}
		if (PQgetisnull(res, i, 6))
		{
			data[i].sink_address = "";
		}
		else
		{
			data[i].sink_address = PQgetvalue(res, i, 6);
		}
		if (PQgetisnull(res, i, 7))
		{
			data[i].sink_port = 0;
		}
		else
		{
			data[i].sink_port = atoi(PQgetvalue(res, i, 7));
		}
		if (PQgetisnull(res, i, 8))
		{
			data[i].drops = 0;
		}
		else
		{
			data[i].drops = atoi(PQgetvalue(res, i, 8));
		}
		if (PQgetisnull(res, i, 9))
		{
			data[i].bitrate = 0;
		}
		else
		{
			data[i].bitrate = atoi(PQgetvalue(res, i, 9));
		}
		if (PQgetisnull(res, i, 10))
		{
			data[i].bitrate_in = 0;
		}
		else
		{
			data[i].bitrate_in = atoi(PQgetvalue(res, i, 10));
		}
		if (PQgetisnull(res, i, 11))
		{
			data[i].bitrate_out = 0;
		}
		else
		{
			data[i].bitrate_out = atoi(PQgetvalue(res, i, 11));
		}
		if (PQgetisnull(res, i, 12))
		{
			data[i].resendcounter = 0;
		}
		else
		{
			data[i].resendcounter = atoi(PQgetvalue(res, i, 12));
		}
		if (PQgetisnull(res, i, 13))
		{
			data[i].rudprtt = 0;
		}
		else
		{
			data[i].rudprtt = atoi(PQgetvalue(res, i, 13));
		}
		if (PQgetisnull(res, i, 14))
		{
			data[i].pathnumber = 0;
		}
		else
		{
			data[i].pathnumber = atoi(PQgetvalue(res, i, 14));
		}
		if (PQgetisnull(res, i, 15))
		{
			data[i].pathname = "";
		}
		else
		{
			data[i].pathname = PQgetvalue(res, i, 15);
		}
		if (PQgetisnull(res, i, 16))
		{
			data[i].linkdrops = 0;
		}
		else
		{
			data[i].linkdrops = atoi(PQgetvalue(res, i, 16));
		}
		if (PQgetisnull(res, i, 17))
		{
			//data[i].multipath = 0; // true/false??
		}
		else
		{
			if (strcmp(PQgetvalue(res, i, 17), "t") == 0)
			{
				data[i].multipath = 1;
			}
			else
			{
				data[i].multipath = 0;
			}
		}
		if (PQgetisnull(res, i, 18))
		{
			//data[i].mploadbalance = 0; // true/false??
		}
		else
		{
			if (strcmp(PQgetvalue(res, i, 18), "t") == 0)
			{
				data[i].mploadbalance = 1;
			}
			else
			{
				data[i].mploadbalance = 0;
			}
		}
	}
	PQclear(res);
	return 0;
}

/*
int DBSystemInfo::db_update_streamstat_internal(const std::string &query)
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


int DBStreamStat::db_fetch_by_id(PGconn *dbconn, uint64_t id, DBStreamStat::Data *data)
{
	std::string query = "SELECT id, streaminterface_id, timestamp, streammode, source_address, source_port, sink_address, sink_port, drops, bitrate, bitrate_in, bitrate_out, resendcounter, rudprtt, pathnumber, pathname, linkdrops, multipath, mploadbalance FROM streamstats WHERE id = " + std::to_string(id);
	return db_fetch_streamstat_internal(query, dbconn, *data);
}

int DBStreamStat::db_fetch_by_streaminterfaceid_between_timestamps(PGconn *dbconn, uint64_t id, const std::string &begin, const std::string &end, std::vector<DBStreamStat::Data> *data)
{
	std::string query = "SELECT id, streaminterface_id, timestamp, streammode, source_address, source_port, sink_address, sink_port, drops, bitrate, bitrate_in, bitrate_out, resendcounter, rudprtt, pathnumber, pathname, linkdrops, multipath, mploadbalance FROM streamstats WHERE timestamp BETWEEN '" + begin + "'::timestamp AND '" + end + "'::timestamp AND pathnumber = 0 AND streaminterface_id = " + std::to_string(id);
	return db_fetch_streamstats_internal(query, dbconn, *data);
}

int DBStreamStat::db_fetch_by_streaminterfaceid_between_epoch_timestamps(PGconn *dbconn, uint64_t id, const std::string &begin, const std::string &end, std::vector<DBStreamStat::Data> *data)
{
	std::string query = "SELECT id, streaminterface_id, timestamp, streammode, source_address, source_port, sink_address, sink_port, drops, bitrate, bitrate_in, bitrate_out, resendcounter, rudprtt, pathnumber, pathname, linkdrops, multipath, mploadbalance FROM streamstats WHERE timestamp BETWEEN to_timestamp(" + begin + ") AND to_timestamp(" + end + ") AND pathnumber = 0 AND streaminterface_id = " + std::to_string(id);
	return db_fetch_streamstats_internal(query, dbconn, *data);
}

int DBStreamStat::db_fetch_all(PGconn *dbconn, std::vector<DBStreamStat::Data> *data)
{
	std::string query = "SELECT id, streaminterface_id, timestamp, streammode, source_address, source_port, sink_address, sink_port, drops, bitrate, bitrate_in, bitrate_out, resendcounter, rudprtt, pathnumber, pathname, linkdrops, multipath, mploadbalance FROM streamstats";
	return db_fetch_streamstats_internal(query, dbconn, *data);
}


