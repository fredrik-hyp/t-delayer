#include "dbcommon.h"
#include "dbstreaminterfaceconfig.h"
#include "cgiutils.h"

int db_fetch_streaminterfaceconfig_internal(const std::string &query, PGconn * dbconn, DBStreaminterfaceConfig::Data &data)
{
	PGresult *res;
	int numtuples;
	
	if (dbconn == NULL)
	{
		syslog(LOG_CRIT, "db_fetch_streaminterfaceconfig_internal connection invalid");
		return 1;
	}
	if (check_DB_connection_and_reconnect(&dbconn))
	{
		syslog(LOG_CRIT, "db_fetch_streaminterfaceconfig_internal cant reconnect DB connection");
		return 1;
	}

	res = PQexec(dbconn, query.c_str());
	
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "db_fetch_streaminterfaceconfig_internal: Answer not ok\n");
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
		data.typeofconfig = "";
	}
	else
	{
		data.typeofconfig = PQgetvalue(res, 0, 2);
	}

	if (PQgetisnull(res, 0, 3))
	{
		data.autostart = 1;
	}
	else
	{
		if (strcmp(PQgetvalue(res, 0, 3), "t") == 0)
		{
			data.autostart = 1;
		}
		else
		{
			data.autostart = 0;
		}
	}
	if (PQgetisnull(res, 0, 4))
	{
		data.msbuffer = 0; 
	}
	else
	{
		data.msbuffer = atoi(PQgetvalue(res, 0, 4));
	}
	if (PQgetisnull(res, 0, 5))
	{
		data.resendmode = ""; 
	}
	else
	{
		data.resendmode = PQgetvalue(res, 0, 5);
	}
	if (PQgetisnull(res, 0, 6))
	{
		data.mode = ""; 
	}
	else
	{
		data.mode = PQgetvalue(res, 0, 6);
	}
	if (PQgetisnull(res, 0, 7))
	{
		data.nosessionid = 0; 
	}
	else
	{
		if (strcmp(PQgetvalue(res, 0, 7), "t") == 0)
		{
			data.nosessionid = 1;
		}
		else
		{
			data.nosessionid = 0;
		}
	}
	if (PQgetisnull(res, 0, 8))
	{
		data.reordersize = 0; 
	}
	else
	{
		data.reordersize = atoi(PQgetvalue(res, 0, 8));
	}
	if (PQgetisnull(res, 0, 9))
	{
		data.backuptoprimary = 0; 
	}
	else
	{
		if (strcmp(PQgetvalue(res, 0, 9), "t") == 0)
		{
			data.backuptoprimary = 1;
		}
		else
		{
			data.backuptoprimary = 0;
		}
	}
	if (PQgetisnull(res, 0, 10))
	{
		data.multipath = 0; 
	}
	else
	{
		if (strcmp(PQgetvalue(res, 0, 10), "t") == 0)
		{
			data.multipath = 1;
		}
		else
		{
			data.multipath = 0;
		}
	}
	if (PQgetisnull(res, 0, 11))
	{
		data.mploadbalance = 0; 
	}
	else
	{
		if (strcmp(PQgetvalue(res, 0, 11), "t") == 0)
		{
			data.mploadbalance = 1;
		}
		else
		{
			data.mploadbalance = 0;
		}
	}
	if (PQgetisnull(res, 0, 12))
	{
		//data.mcinusecheck = 0; // true/false??? 
	}
	else
	{
		if (strcmp(PQgetvalue(res, 0, 12), "t") == 0)
		{
			data.mcinusecheck = 1;
		}
		else
		{
			data.mcinusecheck = 0;
		}
	}
	
	if (PQgetisnull(res, 0, 13))
	{
		//data.actasserver = 0; // true/false??? 
	}
	else
	{
		if (strcmp(PQgetvalue(res, 0, 13), "t") == 0)
		{
			data.actasserver = 1;
		}
		else
		{
			data.actasserver = 0;
		}
	}

	PQclear(res);
	return 0;
}

int db_fetch_streaminterfaceconfigs_internal(const std::string &query, PGconn * dbconn, std::vector<DBStreaminterfaceConfig::Data> &data)
{
	PGresult *res;
	int numtuples;

	if (dbconn == NULL)
	{
		syslog(LOG_CRIT, "db_fetch_streaminterfaceconfigs_internal connection invalid");
		return 1;
	}
	if (check_DB_connection_and_reconnect(&dbconn))
	{
		syslog(LOG_CRIT, "db_fetch_streaminterfaceconfigs_internal cant reconnect DB connection");
		return 1;
	}

	res = PQexec(dbconn, query.c_str());
	
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "db_fetch_streaminterfaceconfigs_internal: Answer not ok\n");
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
			data[i].typeofconfig = "";
		}
		else
		{
			data[i].typeofconfig = PQgetvalue(res, i, 2);
		}

		if (PQgetisnull(res, i, 3))
		{
			data[i].autostart = 0;
		}
		else
		{
			if (strcmp(PQgetvalue(res, i, 3), "t") == 0)
			{
				data[i].autostart = 1;
			}
			else
			{
				data[i].autostart = 0;
			}
		}
		if (PQgetisnull(res, i, 4))
		{
			data[i].msbuffer = 0; 
		}
		else
		{
			data[i].msbuffer = atoi(PQgetvalue(res, i, 4));
		}
		if (PQgetisnull(res, i, 5))
		{
			data[i].resendmode = ""; 
		}
		else
		{
			data[i].resendmode = PQgetvalue(res, i, 5);
		}
		if (PQgetisnull(res, i, 6))
		{
			data[i].mode = ""; 
		}
		else
		{
			data[i].mode = PQgetvalue(res, i, 6);
		}
		if (PQgetisnull(res, i, 7))
		{
			data[i].nosessionid = 0; 
		}
		else
		{
			if (strcmp(PQgetvalue(res, i, 7), "t") == 0)
			{
				data[i].nosessionid = 1;
			}
			else
			{
				data[i].nosessionid = 0;
			}
		}
		if (PQgetisnull(res, i, 8))
		{
			data[i].reordersize = 0; 
		}
		else
		{
			data[i].reordersize = atoi(PQgetvalue(res, i, 8));
		}
		if (PQgetisnull(res, i, 9))
		{
			data[i].backuptoprimary = 0; 
		}
		else
		{
			if (strcmp(PQgetvalue(res, i, 9), "t") == 0)
			{
				data[i].backuptoprimary = 1;
			}
			else
			{
				data[i].backuptoprimary = 0;
			}
		}
		if (PQgetisnull(res, i, 10))
		{
			data[i].multipath = 0; 
		}
		else
		{
			if (strcmp(PQgetvalue(res, i, 10), "t") == 0)
			{
				data[i].multipath = 1;
			}
			else
			{
				data[i].multipath = 0;
			}
		}
		if (PQgetisnull(res, i, 11))
		{
			data[i].mploadbalance = 0; 
		}
		else
		{
			if (strcmp(PQgetvalue(res, i, 11), "t") == 0)
			{
				data[i].mploadbalance = 1;
			}
			else
			{
				data[i].mploadbalance = 0;
			}
		}
		if (PQgetisnull(res, i, 12))
		{
			//data[i].mcinusecheck = 0; // true/false??? 
		}
		else
		{
			if (strcmp(PQgetvalue(res, i, 12), "t") == 0)
			{
				data[i].mcinusecheck = 1;
			}
			else
			{
				data[i].mcinusecheck = 0;
			}
		}
		if (PQgetisnull(res, i, 13))
		{
			//data[i].actasserver = 0; // true/false??? 
		}
		else
		{
			if (strcmp(PQgetvalue(res, i, 13), "t") == 0)
			{
				data[i].actasserver = 1;
			}
			else
			{
				data[i].actasserver = 0;
			}
		}
	}

	PQclear(res);
	return 0;
}

int db_update_streaminterfaceconfig_internal(const std::string &query, PGconn *dbconn)
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

int db_update_streaminterfaceconfig_internal_prepared(const std::string &query, std::vector<const char*> &paramvalues, PGconn *dbconn)
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

int DBStreaminterfaceConfig::db_fetch_by_id(PGconn *dbconn, uint64_t id, DBStreaminterfaceConfig::Data *data)
{
	std::string query = "SELECT id, streaminterface_id, typeofconfig, autostart, msbuffer, resendmode, mode, nosessionid, reordersize, backuptoprimary, multipath, mploadbalance, mcinusecheck, actasserver FROM streaminterfaceconfigs WHERE id = " + std::to_string(id);
	return db_fetch_streaminterfaceconfig_internal(query, dbconn, *data);
}

int DBStreaminterfaceConfig::db_fetch_by_streaminterface_id(PGconn *dbconn, uint64_t id, DBStreaminterfaceConfig::Data *data)
{
	std::string query = "SELECT id, streaminterface_id, typeofconfig, autostart, msbuffer, resendmode, mode, nosessionid, reordersize, backuptoprimary, multipath, mploadbalance, mcinusecheck, actasserver FROM streaminterfaceconfigs WHERE streaminterface_id = " + std::to_string(id);
	return db_fetch_streaminterfaceconfig_internal(query, dbconn, *data);
}

int DBStreaminterfaceConfig::db_fetch_all(PGconn *dbconn, std::vector<DBStreaminterfaceConfig::Data> *data)
{
	std::string query = "SELECT id, streaminterface_id, typeofconfig, autostart, msbuffer, resendmode, mode, nosessionid, reordersize, backuptoprimary, multipath, mploadbalance, mcinusecheck, actasserver FROM streaminterfaceconfigs";
	return db_fetch_streaminterfaceconfigs_internal(query, dbconn, *data);
}
/*
int DBStreaminterfaceConfig::db_update_by_id(PGconn *dbconn, DBStreaminterfaceConfig::Data *data)
{
	std::string query = "UPDATE streaminterfaceconfigs SET ";
	//query += "id = " + std::to_string(data->id) + ", ";
	//query += "streaminterface_id = " + std::to_string(data->streaminterface_id) + ", ";
	query += "typeofconfig = '" + data->typeofconfig + "', ";
	query += "autostart = '" + bool_to_str(data->autostart) + "', ";
	query += "msbuffer = " + std::to_string(data->msbuffer) + ", ";
	query += "resendmode = '" + data->resendmode +"', ";
	query += "mode = '" + data->mode +"', ";
	query += "nosessionid = '" + bool_to_str(data->nosessionid) + "', ";
	query += "reordersize = " + std::to_string(data->reordersize) + ", ";
	query += "backuptoprimary = '" + bool_to_str(data->backuptoprimary) + "', ";
	query += "multipath = '" + bool_to_str(data->multipath) + "', ";
	query += "mploadbalance = '" + bool_to_str(data->mploadbalance) + "', ";
	query += "mcinusecheck = '" + bool_to_str(data->mcinusecheck) + "', ";
	query += "actasserver = '" + bool_to_str(data->actasserver) + "' ";
	query += "WHERE id = " + std::to_string(data->id);

	return db_update_streaminterfaceconfig_internal(query, dbconn);
}
*/
int DBStreaminterfaceConfig::db_update_by_id(PGconn *dbconn, DBStreaminterfaceConfig::Data *data)
{
	std::string query = "UPDATE streaminterfaceconfigs SET typeofconfig = $1, autostart = $2, msbuffer = $3, resendmode = $4, mode = $5, nosessionid = $6, reordersize = $7, backuptoprimary = $8, multipath = $9, mploadbalance = $10, mcinusecheck = $11, actasserver = $12 WHERE id = " + std::to_string(data->id) + ";";

	std::string param1 = bool_to_str(data->autostart);
	std::string param2 = std::to_string(data->msbuffer);
	std::string param5 = bool_to_str(data->nosessionid);
	std::string param6 = std::to_string(data->reordersize);
	std::string param7 = bool_to_str(data->backuptoprimary);
	std::string param8 = bool_to_str(data->multipath);
	std::string param9 = bool_to_str(data->mploadbalance);
	std::string param10 = bool_to_str(data->mcinusecheck);
	std::string param11 = bool_to_str(data->actasserver);

	std::vector<const char*> paramvalues(12);
	paramvalues[0] = data->typeofconfig.c_str();
	paramvalues[1] = param1.c_str();
	paramvalues[2] = param2.c_str();
	paramvalues[3] = data->resendmode.c_str();
	paramvalues[4] = data->mode.c_str();
	paramvalues[5] = param5.c_str();
	paramvalues[6] = param6.c_str();
	paramvalues[7] = param7.c_str();
	paramvalues[8] = param8.c_str();
	paramvalues[9] = param9.c_str();
	paramvalues[10] = param10.c_str();
	paramvalues[11] = param11.c_str();

	return db_update_streaminterfaceconfig_internal_prepared(query, paramvalues, dbconn);
}
/*
int DBStreaminterfaceConfig::db_update_by_streaminterface_id(PGconn *dbconn, DBStreaminterfaceConfig::Data *data)
{
	std::string query = "UPDATE streaminterfaceconfigs SET ";
	//query += "id = " + std::to_string(data->id) + ", ";
	//query += "streaminterface_id = " + std::to_string(data->streaminterface_id) + ", ";
	query += "typeofconfig = '" + data->typeofconfig + "', ";
	query += "autostart = '" + bool_to_str(data->autostart) + "', ";
	query += "msbuffer = " + std::to_string(data->msbuffer) + ", ";
	query += "resendmode = '" + data->resendmode +"', ";
	query += "mode = '" + data->mode +"', ";
	query += "nosessionid = '" + bool_to_str(data->nosessionid) + "', ";
	query += "reordersize = " + std::to_string(data->reordersize) + ", ";
	query += "backuptoprimary = '" + bool_to_str(data->backuptoprimary) + "', ";
	query += "multipath = '" + bool_to_str(data->multipath) + "', ";
	query += "mploadbalance = '" + bool_to_str(data->mploadbalance) + "', ";
	query += "mcinusecheck = '" + bool_to_str(data->mcinusecheck) + "', ";
	query += "actasserver = '" + bool_to_str(data->actasserver) + "' ";
	query += "WHERE streaminterface_id = " + std::to_string(data->streaminterface_id);

	return db_update_streaminterfaceconfig_internal(query, dbconn);
}
*/
int DBStreaminterfaceConfig::db_update_by_streaminterface_id(PGconn *dbconn, DBStreaminterfaceConfig::Data *data)
{
	std::string query = "UPDATE streaminterfaceconfigs SET typeofconfig = $1, autostart = $2, msbuffer = $3, resendmode = $4, mode = $5, nosessionid = $6, reordersize = $7, backuptoprimary = $8, multipath = $9, mploadbalance = $10, mcinusecheck = $11, actasserver = $12 WHERE streaminterface_id = " + std::to_string(data->streaminterface_id) + ";";

	std::string param1 = bool_to_str(data->autostart);
	std::string param2 = std::to_string(data->msbuffer);
	std::string param5 = bool_to_str(data->nosessionid);
	std::string param6 = std::to_string(data->reordersize);
	std::string param7 = bool_to_str(data->backuptoprimary);
	std::string param8 = bool_to_str(data->multipath);
	std::string param9 = bool_to_str(data->mploadbalance);
	std::string param10 = bool_to_str(data->mcinusecheck);
	std::string param11 = bool_to_str(data->actasserver);

	std::vector<const char*> paramvalues(12);
	paramvalues[0] = data->typeofconfig.c_str();
	paramvalues[1] = param1.c_str();
	paramvalues[2] = param2.c_str();
	paramvalues[3] = data->resendmode.c_str();
	paramvalues[4] = data->mode.c_str();
	paramvalues[5] = param5.c_str();
	paramvalues[6] = param6.c_str();
	paramvalues[7] = param7.c_str();
	paramvalues[8] = param8.c_str();
	paramvalues[9] = param9.c_str();
	paramvalues[10] = param10.c_str();
	paramvalues[11] = param11.c_str();
	
	return db_update_streaminterfaceconfig_internal_prepared(query, paramvalues, dbconn);
}
