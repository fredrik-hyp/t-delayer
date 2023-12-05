#include "dbcommon.h"
#include "dbstreaminterfacelivestatistic.h"

int db_fetch_streaminterfacelivestatistic_internal(const std::string &query, PGconn * dbconn, DBStreaminterfaceLivestatistic::Data &data)
{
	PGresult *res;
	int numtuples;

	if (dbconn == NULL)
	{
		syslog(LOG_CRIT, "db_fetch_streaminterfacelivestatistic_internal connection invalid");
		return 1;
	}
	if (check_DB_connection_and_reconnect(&dbconn))
	{
		syslog(LOG_CRIT, "db_fetch_streaminterfacelivestatistic_internal cant reconnect DB connection");
		return 1;
	}

	res = PQexec(dbconn, query.c_str());
	
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "db_fetch_streaminterfacelivestatistic_internal: Answer not ok\n");
		PQclear(res);
		return 1;
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
		data.mode = "";
	}
	else
	{
		data.mode = PQgetvalue(res, 0, 2);
	}

	if (PQgetisnull(res, 0, 3))
	{
		data.state = "";
	}
	else
	{
		data.state = PQgetvalue(res, 0, 3);
	}
	if (PQgetisnull(res, 0, 4))
	{
		data.bitratein = 0;
	}
	else
	{
		data.bitratein = atoi(PQgetvalue(res, 0, 4));
	}
	if (PQgetisnull(res, 0, 5))
	{
		data.bitrateout = 0;
	}
	else
	{
		data.bitrateout = atoi(PQgetvalue(res, 0, 5));
	}
	if (PQgetisnull(res, 0, 6))
	{
		data.bufferlevel = 0;
	}
	else
	{
		data.bufferlevel = atoi(PQgetvalue(res, 0, 6));
	}
	if (PQgetisnull(res, 0, 7))
	{
		data.rtt = 0;
	}
	else
	{
		data.rtt = atoi(PQgetvalue(res, 0, 7));
	}
	if (PQgetisnull(res, 0, 8))
	{
		data.timestamp = 0;
	}
	else
	{
		data.timestamp = atoi(PQgetvalue(res, 0, 8));
	}
	if (PQgetisnull(res, 0, 9))
	{
		data.source = "";
	}
	else
	{
		data.source = PQgetvalue(res, 0, 9);
	}
	if (PQgetisnull(res, 0, 10))
	{
		data.destination = "";
	}
	else
	{
		data.destination = PQgetvalue(res, 0, 10);
	}
	if (PQgetisnull(res, 0, 11))
	{
		data.udpdrops = "";
	}
	else
	{
		data.udpdrops = PQgetvalue(res, 0, 11);
	}
	if (PQgetisnull(res, 0, 12))
	{
		data.acc_rtp_drops = 0;
	}
	else
	{
		data.acc_rtp_drops = atoi(PQgetvalue(res, 0, 12));
	}
	if (PQgetisnull(res, 0, 13))
	{
		data.pathnumber = 0;
	}
	else
	{
		data.pathnumber = atoi(PQgetvalue(res, 0, 13));
	}
	if (PQgetisnull(res, 0, 14))
	{
		data.pathname = "";
	}
	else
	{
		data.pathname = PQgetvalue(res, 0, 14);
	}
	if (PQgetisnull(res, 0, 15))
	{
		//data.multipath = 0;
	}
	else
	{
		if (strcmp(PQgetvalue(res, 0, 15), "t") == 0)
		{
			data.multipath = 1;
		}
		else
		{
			data.multipath = 0;
		}
	}
	if (PQgetisnull(res, 0, 16))
	{
		//data.mploadbalance = 0;
	}
	else
	{
		if (strcmp(PQgetvalue(res, 0, 16), "t") == 0)
		{
			data.mploadbalance = 1;
		}
		else
		{
			data.mploadbalance = 0;
		}
	}
	if (PQgetisnull(res, 0, 17))
	{
		data.ttl = 0;
	}
	else
	{
		data.ttl = atoi(PQgetvalue(res, 0, 17));
	}
	if (PQgetisnull(res, 0, 18))
	{
		data.hostclientcertificate_id = 0;
	}
	else
	{
		data.hostclientcertificate_id = atoi(PQgetvalue(res, 0, 18));
	}
	if (PQgetisnull(res, 0, 19))
	{
		data.talkbackclientcertificate_id = 0;
	}
	else
	{
		data.talkbackclientcertificate_id = atoi(PQgetvalue(res, 0, 19));
	}
	
	PQclear(res);
	return 0;
}

int db_fetch_streaminterfacelivestatistics_internal(const std::string &query, PGconn * dbconn, std::vector<DBStreaminterfaceLivestatistic::Data> &data)
{
	PGresult *res;
	int numtuples;

	if (dbconn == NULL)
	{
		syslog(LOG_CRIT, "db_fetch_streaminterfacelivestatistics_internal connection invalid");
		return 1;
	}
	if (check_DB_connection_and_reconnect(&dbconn))
	{
		syslog(LOG_CRIT, "db_fetch_streaminterfacelivestatistics_internal cant reconnect DB connection");
		return 1;
	}

	res = PQexec(dbconn, query.c_str());
	
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "db_fetch_streaminterfacelivestatistics_internal: Answer not ok\n");
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
			data[i].mode = "";
		}
		else
		{
			data[i].mode = PQgetvalue(res, i, 2);
		}

		if (PQgetisnull(res, i, 3))
		{
			data[i].state = "";
		}
		else
		{
			data[i].state = PQgetvalue(res, i, 3);
		}
		if (PQgetisnull(res, i, 4))
		{
			data[i].bitratein = 0;
		}
		else
		{
			data[i].bitratein = atoi(PQgetvalue(res, i, 4));
		}
		if (PQgetisnull(res, i, 5))
		{
			data[i].bitrateout = 0;
		}
		else
		{
			data[i].bitrateout = atoi(PQgetvalue(res, i, 5));
		}
		if (PQgetisnull(res, i, 6))
		{
			data[i].bufferlevel = 0;
		}
		else
		{
			data[i].bufferlevel = atoi(PQgetvalue(res, i, 6));
		}
		if (PQgetisnull(res, i, 7))
		{
			data[i].rtt = 0;
		}
		else
		{
			data[i].rtt = atoi(PQgetvalue(res, i, 7));
		}
		if (PQgetisnull(res, i, 8))
		{
			data[i].timestamp = 0;
		}
		else
		{
			data[i].timestamp = atoi(PQgetvalue(res, i, 8));
		}
		if (PQgetisnull(res, i, 9))
		{
			data[i].source = "";
		}
		else
		{
			data[i].source = PQgetvalue(res, i, 9);
		}
		if (PQgetisnull(res, i, 10))
		{
			data[i].destination = "";
		}
		else
		{
			data[i].destination = PQgetvalue(res, i, 10);
		}
		if (PQgetisnull(res, i, 11))
		{
			data[i].udpdrops = "";
		}
		else
		{
			data[i].udpdrops = PQgetvalue(res, i, 11);
		}
		if (PQgetisnull(res, i, 12))
		{
			data[i].acc_rtp_drops = 0;
		}
		else
		{
			data[i].acc_rtp_drops = atoi(PQgetvalue(res, i, 12));
		}
		if (PQgetisnull(res, i, 13))
		{
			data[i].pathnumber = 0;
		}
		else
		{
			data[i].pathnumber = atoi(PQgetvalue(res, i, 13));
		}
		if (PQgetisnull(res, i, 14))
		{
			data[i].pathname = "";
		}
		else
		{
			data[i].pathname = PQgetvalue(res, i, 14);
		}
		if (PQgetisnull(res, i, 15))
		{
			//data[i].multipath = 0;
		}
		else
		{
			if (strcmp(PQgetvalue(res, i, 15), "t") == 0)
			{
				data[i].multipath = 1;
			}
			else
			{
				data[i].multipath = 0;
			}
		}
		if (PQgetisnull(res, i, 16))
		{
			//data[i].mploadbalance = 0;
		}
		else
		{
			if (strcmp(PQgetvalue(res, i, 16), "t") == 0)
			{
				data[i].mploadbalance = 1;
			}
			else
			{
				data[i].mploadbalance = 0;
			}
		}
		if (PQgetisnull(res, i, 17))
		{
			data[i].ttl = 0;
		}
		else
		{
			data[i].ttl = atoi(PQgetvalue(res, i, 17));
		}
		if (PQgetisnull(res, i, 18))
		{
			data[i].hostclientcertificate_id = 0;
		}
		else
		{
			data[i].hostclientcertificate_id = atoi(PQgetvalue(res, i, 18));
		}
		if (PQgetisnull(res, i, 19))
		{
			data[i].talkbackclientcertificate_id = 0;
		}
		else
		{
			data[i].talkbackclientcertificate_id = atoi(PQgetvalue(res, i, 19));
		}
	}

	PQclear(res);
	return 0;
}
/*
int DBStreaminterface::db_update_streaminterface_internal(const std::string &query)
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

int DBStreaminterfaceLivestatistic::db_fetch_by_id(PGconn *dbconn, uint64_t id, DBStreaminterfaceLivestatistic::Data *data)
{
	std::string query = "SELECT id, streaminterface_id, mode, state, bitratein, bitrateout, bufferlevel, rtt, extract(epoch from timestamp), source, destination, udpdrops, acc_rtp_drops, pathnumber, pathname, multipath, mploadbalance, ttl, hostclientcertificate_id, talkbackclientcertificate_id FROM streaminterfacelivestatistics WHERE streaminterface_id = " + std::to_string(id);
	return db_fetch_streaminterfacelivestatistic_internal(query, dbconn, *data);
}

int DBStreaminterfaceLivestatistic::db_fetch_all_by_streaminterface_id(PGconn *dbconn, uint64_t id, std::vector<DBStreaminterfaceLivestatistic::Data> *data)
{
	std::string query = "SELECT id, streaminterface_id, mode, state, bitratein, bitrateout, bufferlevel, rtt, extract(epoch from timestamp), source, destination, udpdrops, acc_rtp_drops, pathnumber, pathname, multipath, mploadbalance, ttl, hostclientcertificate_id, talkbackclientcertificate_id FROM streaminterfacelivestatistics WHERE streaminterface_id = " + std::to_string(id);
	return db_fetch_streaminterfacelivestatistics_internal(query, dbconn, *data);
}


int DBStreaminterfaceLivestatistic::db_fetch_all(PGconn *dbconn, std::vector<DBStreaminterfaceLivestatistic::Data> *data)
{
	std::string query = "SELECT id, streaminterface_id, mode, state, bitratein, bitrateout, bufferlevel, rtt, extract(epoch from timestamp), source, destination, udpdrops, acc_rtp_drops, pathnumber, pathname, multipath, mploadbalance, ttl, hostclientcertificate_id, talkbackclientcertificate_id FROM streaminterfacelivestatistics";
	return db_fetch_streaminterfacelivestatistics_internal(query, dbconn, *data);
}

