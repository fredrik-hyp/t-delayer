#include "dbcommon.h"
#include "dbstreaminterfacesource.h"
#include "cgiutils.h"

int db_fetch_streaminterfacesource_internal(const std::string &query, PGconn * dbconn, DBStreaminterfaceSource::Data &data)
{
	PGresult *res;
	int numtuples;

	if (dbconn == NULL)
	{
		syslog(LOG_CRIT, "db_fetch_streaminterfacesource_internal connection invalid");
		return 1;
	}
	if (check_DB_connection_and_reconnect(&dbconn))
	{
		syslog(LOG_CRIT, "db_fetch_streaminterfacesource_internal cant reconnect DB connection");
		return 1;
	}

	res = PQexec(dbconn, query.c_str());
	
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "db_fetch_streaminterfacesource_internal: Answer not ok\n");
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
		data.streaminterfaceconfig_id = 0;
	}
	else
	{
		data.streaminterfaceconfig_id = atoi(PQgetvalue(res, 0, 1));
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
		data.priority = 0;
	}
	else
	{
		data.priority = atoi(PQgetvalue(res, 0, 3));
	}
	if (PQgetisnull(res, 0, 4))
	{
		data.sourcetype = ""; 
	}
	else
	{
		data.sourcetype = PQgetvalue(res, 0, 4);
	}
	if (PQgetisnull(res, 0, 5))
	{
		data.host = ""; 
	}
	else
	{
		data.host = PQgetvalue(res, 0, 5);
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
		data.talkbackhost = ""; 
	}
	else
	{
		data.talkbackhost = PQgetvalue(res, 0, 7);
	}
	if (PQgetisnull(res, 0, 8))
	{
		data.talkbackport = 0; 
	}
	else
	{
		data.talkbackport = atoi(PQgetvalue(res, 0, 8));
	}
	if (PQgetisnull(res, 0, 9))
	{
		data.virtualsource_id = 0; 
	}
	else
	{
		data.virtualsource_id = atoi(PQgetvalue(res, 0, 9));
	}
	if (PQgetisnull(res, 0, 10))
	{
		data.vsourcerevision = 0; 
	}
	else
	{
		data.vsourcerevision = atoi(PQgetvalue(res, 0, 10));
	}
	if (PQgetisnull(res, 0, 11))
	{
		data.hostclientcertificate_id = 0; 
	}
	else
	{
		data.hostclientcertificate_id = atoi(PQgetvalue(res, 0, 11));
	}
	if (PQgetisnull(res, 0, 12))
	{
		data.talkbackclientcertificate_id = 0;
	}
	else
	{
		data.talkbackclientcertificate_id = atoi(PQgetvalue(res, 0, 12));
	}
	if (PQgetisnull(res, 0, 13))
	{
		data.maxretrytime = 0;
	}
	else
	{
		data.maxretrytime = atoi(PQgetvalue(res, 0, 13));
	}
	if (PQgetisnull(res, 0, 14))
	{
		data.maxretries = 0;
	}
	else
	{
		data.maxretries = atoi(PQgetvalue(res, 0, 14));
	}
	if (PQgetisnull(res, 0, 15))
	{
		data.streammissfailovertimeout = 0;
	}
	else
	{
		data.streammissfailovertimeout = atoi(PQgetvalue(res, 0, 15));
	}
	if (PQgetisnull(res, 0, 16))
	{
		data.recording_id = 0;
	}
	else
	{
		data.recording_id = atoi(PQgetvalue(res, 0, 16));
	}
	if (PQgetisnull(res, 0, 17))
	{
		//data.addrtp = 0; // true/false???
	}
	else
	{
		if (strcmp(PQgetvalue(res, 0, 17), "t") == 0)
		{
			data.addrtp = 1;
		}
		else
		{
			data.addrtp = 0;
		}
	}

	PQclear(res);
	return 0;
}

int db_fetch_streaminterfacesources_internal(const std::string &query, PGconn * dbconn, std::vector<DBStreaminterfaceSource::Data> &data)
{
	PGresult *res;
	int numtuples;

	if (dbconn == NULL)
	{
		syslog(LOG_CRIT, "db_fetch_streaminterfacesources_internal connection invalid");
		return 1;
	}
	if (check_DB_connection_and_reconnect(&dbconn))
	{
		syslog(LOG_CRIT, "db_fetch_streaminterfacesources_internal cant reconnect DB connection");
		return 1;
	}

	res = PQexec(dbconn, query.c_str());
	
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "db_fetch_streaminterfacesources_internal: Answer not ok\n");
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
			data[i].streaminterfaceconfig_id = 0;
		}
		else
		{
			data[i].streaminterfaceconfig_id = atoi(PQgetvalue(res, i, 1));
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
			data[i].priority = 0;
		}
		else
		{
			data[i].priority = atoi(PQgetvalue(res, i, 3));
		}
		if (PQgetisnull(res, i, 4))
		{
			data[i].sourcetype = ""; 
		}
		else
		{
			data[i].sourcetype = PQgetvalue(res, i, 4);
		}
		if (PQgetisnull(res, i, 5))
		{
			data[i].host = ""; 
		}
		else
		{
			data[i].host = PQgetvalue(res, i, 5);
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
			data[i].talkbackhost = ""; 
		}
		else
		{
			data[i].talkbackhost = PQgetvalue(res, i, 7);
		}
		if (PQgetisnull(res, i, 8))
		{
			data[i].talkbackport = 0; 
		}
		else
		{
			data[i].talkbackport = atoi(PQgetvalue(res, i, 8));
		}
		if (PQgetisnull(res, i, 9))
		{
			data[i].virtualsource_id = 0; 
		}
		else
		{
			data[i].virtualsource_id = atoi(PQgetvalue(res, i, 9));
		}
		if (PQgetisnull(res, i, 10))
		{
			data[i].vsourcerevision = 0; 
		}
		else
		{
			data[i].vsourcerevision = atoi(PQgetvalue(res, i, 10));
		}
		if (PQgetisnull(res, i, 11))
		{
			data[i].hostclientcertificate_id = 0; 
		}
		else
		{
			data[i].hostclientcertificate_id = atoi(PQgetvalue(res, i, 11));
		}
		if (PQgetisnull(res, i, 12))
		{
			data[i].talkbackclientcertificate_id = 0;
		}
		else
		{
			data[i].talkbackclientcertificate_id = atoi(PQgetvalue(res, i, 12));
		}
		if (PQgetisnull(res, i, 13))
		{
			data[i].maxretrytime = 0;
		}
		else
		{
			data[i].maxretrytime = atoi(PQgetvalue(res, i, 13));
		}
		if (PQgetisnull(res, i, 14))
		{
			data[i].maxretries = 0;
		}
		else
		{
			data[i].maxretries = atoi(PQgetvalue(res, i, 14));
		}
		if (PQgetisnull(res, i, 15))
		{
			data[i].streammissfailovertimeout = 0;
		}
		else
		{
			data[i].streammissfailovertimeout = atoi(PQgetvalue(res, i, 15));
		}
		if (PQgetisnull(res, i, 16))
		{
			data[i].recording_id = 0;
		}
		else
		{
			data[i].recording_id = atoi(PQgetvalue(res, i, 16));
		}
		if (PQgetisnull(res, 0, 17))
		{
			//data[i].addrtp = 0; // true/false???
		}
		else
		{
			if (strcmp(PQgetvalue(res, i, 17), "t") == 0)
			{
				data[i].addrtp = 1;
			}
			else
			{
				data[i].addrtp = 0;
			}
		}
	}

	PQclear(res);
	return 0;
}

int db_update_streaminterfacesource_internal(const std::string &query, PGconn *dbconn)
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

	res = PQexec(dbconn, query.c_str());
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

int db_update_streaminterfacesource_internal_prepared(const std::string &query, std::vector<const char*> &paramvalues, PGconn *dbconn)
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
	
//	Oid param_types[3] = {0,0,0};
//	int paramFormats[3] = {0,0,0};

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


int DBStreaminterfaceSource::db_fetch_by_id(PGconn *dbconn, uint64_t id, DBStreaminterfaceSource::Data *data)
{
	std::string query = "SELECT id, streaminterfaceconfig_id, name, priority, sourcetype, host, port, talkbackhost, talkbackport, virtualsource_id, vsourcerevision, hostclientcertificate_id, talkbackclientcertificate_id, maxretrytime, maxretries, streammissfailovertimeout, recording_id, addrtp FROM streaminterfacesources WHERE id = " + std::to_string(id);
	return db_fetch_streaminterfacesource_internal(query, dbconn, *data);
}

int DBStreaminterfaceSource::db_fetch_by_streaminterfaceconfig_id(PGconn *dbconn, uint64_t id, DBStreaminterfaceSource::Data *data)
{
	std::string query = "SELECT id, streaminterfaceconfig_id, name, priority, sourcetype, host, port, talkbackhost, talkbackport, virtualsource_id, vsourcerevision, hostclientcertificate_id, talkbackclientcertificate_id, maxretrytime, maxretries, streammissfailovertimeout, recording_id, addrtp FROM streaminterfacesources WHERE streaminterfaceconfig_id = " + std::to_string(id);
	return db_fetch_streaminterfacesource_internal(query, dbconn, *data);
}

int DBStreaminterfaceSource::db_fetch_all(PGconn *dbconn, std::vector<DBStreaminterfaceSource::Data> *data)
{
	std::string query = "SELECT id, streaminterfaceconfig_id, name, priority, sourcetype, host, port, talkbackhost, talkbackport, virtualsource_id, vsourcerevision, hostclientcertificate_id, talkbackclientcertificate_id, maxretrytime, maxretries, streammissfailovertimeout, recording_id, addrtp FROM streaminterfacesources";
	return db_fetch_streaminterfacesources_internal(query, dbconn, *data);
}
/*
int DBStreaminterfaceSource::db_update_by_id(PGconn *dbconn, DBStreaminterfaceSource::Data *data)
{
	std::string query = "UPDATE streaminterfacesources SET ";
//	query += "id = " + std::to_string(data->id) + ", ";
//	query += "streaminterfaceconfig_id = " + std::to_string(data->streaminterfaceconfig_id) + ", ";
	query += "name = '" + data->name + "', ";
	query += "priority = " + std::to_string(data->priority) + ", ";
	query += "sourcetype = '" + data->sourcetype + "', ";
	query += "host = '" + data->host +"', ";
	query += "port = " + std::to_string(data->port) +", ";
	query += "talkbackhost = '" + data->talkbackhost + "', ";
	query += "talkbackport = " + std::to_string(data->talkbackport) + ", ";
//	query += "virtualsource_id = " + std::to_string(data->virtualsource_id) + ", ";
	query += "vsourcerevision = " + std::to_string(data->vsourcerevision) + ", ";
//	query += "hostclientcertificate_id = " + std::to_string(data->hostclientcertificate_id) + ", ";
	query += "talkbackclientcertificate_id = " + std::to_string(data->talkbackclientcertificate_id) + ", ";
	query += "maxretrytime = " + std::to_string(data->maxretrytime) + ", ";
	query += "maxretries = " + std::to_string(data->maxretries) + ", ";
	query += "streammissfailovertimeout = " + std::to_string(data->streammissfailovertimeout) + ", ";
//	query += "recording_id = " + std::to_string(data->recording_id) + ", ";
	query += "addrtp = '" + bool_to_str(data->addrtp) + "' ";
	query += "WHERE id = " + std::to_string(data->id);

	return db_update_streaminterfacesource_internal(query, dbconn);
}
*/
int DBStreaminterfaceSource::db_update_by_id(PGconn *dbconn, DBStreaminterfaceSource::Data *data)
{
	std::string query = "UPDATE streaminterfacesources SET name = $1, priority = $2, sourcetype = $3, host = $4, port = $5, talkbackhost = $6, talkbackport = $7, vsourcerevision = $8, maxretrytime = $9, maxretries = $10, streammissfailovertimeout = $11, addrtp = $12 WHERE id = " + std::to_string(data->id) + ";";

	std::string param1 = std::to_string(data->priority);
	std::string param4 = std::to_string(data->port);
	std::string param6 = std::to_string(data->talkbackport);
	std::string param7 = std::to_string(data->vsourcerevision);
	std::string param8 = std::to_string(data->maxretrytime);
	std::string param9 = std::to_string(data->maxretries);
	std::string param10 = std::to_string(data->streammissfailovertimeout);
	std::string param11 = bool_to_str(data->addrtp);	
	
	std::vector<const char*> paramvalues(12);
	paramvalues[0] = data->name.c_str();
	paramvalues[1] = std::to_string(data->priority).c_str();
	paramvalues[2] = data->sourcetype.c_str();
	paramvalues[3] = data->host.c_str();
	paramvalues[4] = std::to_string(data->port).c_str();
	paramvalues[5] = data->talkbackhost.c_str();
	paramvalues[6] = std::to_string(data->talkbackport).c_str();
	paramvalues[7] = std::to_string(data->vsourcerevision).c_str();
	paramvalues[8] = std::to_string(data->maxretrytime).c_str();
	paramvalues[9] = std::to_string(data->maxretries).c_str();
	paramvalues[10] = std::to_string(data->streammissfailovertimeout).c_str();
	paramvalues[11] = bool_to_str(data->addrtp).c_str();

	return db_update_streaminterfacesource_internal_prepared(query, paramvalues, dbconn);
}
/*
int DBStreaminterfaceSource::db_update_by_streaminterfaceconfig_id(PGconn *dbconn, DBStreaminterfaceSource::Data *data)
{
	std::string query = "UPDATE streaminterfacesources SET ";
//	query += "id = " + std::to_string(data->id) + ", ";
//	query += "streaminterfaceconfig_id = " + std::to_string(data->streaminterfaceconfig_id) + ", ";
	query += "name = '" + data->name + "', ";
	query += "priority = " + std::to_string(data->priority) + ", ";
	query += "sourcetype = '" + data->sourcetype + "', ";
	query += "host = '" + data->host +"', ";
	query += "port = " + std::to_string(data->port) +", ";
	query += "talkbackhost = '" + data->talkbackhost + "', ";
	query += "talkbackport = " + std::to_string(data->talkbackport) + ", ";
//	query += "virtualsource_id = " + std::to_string(data->virtualsource_id) + ", ";
	query += "vsourcerevision = " + std::to_string(data->vsourcerevision) + ", ";
//	query += "hostclientcertificate_id = " + std::to_string(data->hostclientcertificate_id) + ", ";
//	query += "talkbackclientcertificate_id = " + std::to_string(data->talkbackclientcertificate_id) + ", ";
	query += "maxretrytime = " + std::to_string(data->maxretrytime) + ", ";
	query += "maxretries = " + std::to_string(data->maxretries) + ", ";
	query += "streammissfailovertimeout = " + std::to_string(data->streammissfailovertimeout) + ", ";
//	query += "recording_id = " + std::to_string(data->recording_id) + ", ";
	query += "addrtp = '" + bool_to_str(data->addrtp) + "' ";
	query += "WHERE streaminterfaceconfig_id = " + std::to_string(data->streaminterfaceconfig_id);
	return db_update_streaminterfacesource_internal(query, dbconn);
}
*/
int DBStreaminterfaceSource::db_update_by_streaminterfaceconfig_id(PGconn *dbconn, DBStreaminterfaceSource::Data *data)
{
	std::string query = "UPDATE streaminterfacesources SET name = $1, priority = $2, sourcetype = $3, host = $4, port = $5, talkbackhost = $6, talkbackport = $7, vsourcerevision = $8, maxretrytime = $9, maxretries = $10, streammissfailovertimeout = $11, addrtp = $12 WHERE streaminterfaceconfig_id = " + std::to_string(data->streaminterfaceconfig_id) + ";";

	std::string param1 = std::to_string(data->priority);
	std::string param4 = std::to_string(data->port);
	std::string param6 = std::to_string(data->talkbackport);
	std::string param7 = std::to_string(data->vsourcerevision);
	std::string param8 = std::to_string(data->maxretrytime);
	std::string param9 = std::to_string(data->maxretries);
	std::string param10 = std::to_string(data->streammissfailovertimeout);
	std::string param11 = bool_to_str(data->addrtp);	
	
	std::vector<const char*> paramvalues(12);
	paramvalues[0] = data->name.c_str();
	paramvalues[1] = param1.c_str();
	paramvalues[2] = data->sourcetype.c_str();
	paramvalues[3] = data->host.c_str();
	paramvalues[4] = param4.c_str();
	paramvalues[5] = data->talkbackhost.c_str();
	paramvalues[6] = param6.c_str();
	paramvalues[7] = param7.c_str();
	paramvalues[8] = param8.c_str();
	paramvalues[9] = param9.c_str();
	paramvalues[10] = param10.c_str();
	paramvalues[11] = param11.c_str();

	return db_update_streaminterfacesource_internal_prepared(query, paramvalues, dbconn);
}
