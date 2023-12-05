#include "dbcommon.h"
#include "dbrootcertificate.h"

int db_fetch_rootcertificate_internal(const std::string &query, PGconn * dbconn, DBRootCertificate::Data &data)
{
	PGresult *res;
	int numtuples;

	if (dbconn == NULL)
	{
		syslog(LOG_CRIT, "db_fetch_rootcertificate_internal connection invalid");
		return 1;
	}
	if (check_DB_connection_and_reconnect(&dbconn))
	{
		syslog(LOG_CRIT, "db_fetch_rootcertificate_internal cant reconnect DB connection");
		return 1;
	}

	res = PQexec(dbconn, query.c_str());
	
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "db_fetch_rootcertificate_internal: Answer not ok\n");
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
		data.created = "";
	}
	else
	{
		data.created = PQgetvalue(res, 0, 1);
	}

	if (PQgetisnull(res, 0, 2))
	{
		data.expiration_days = 0;
	}
	else
	{
		data.expiration_days = atoi(PQgetvalue(res, 0, 2));
	}

	if (PQgetisnull(res, 0, 3))
	{
		data.organisation_id = 0;
	}
	else
	{
		data.organisation_id = atoi(PQgetvalue(res, 0, 3));
	}
	if (PQgetisnull(res, 0, 4))
	{
		data.name = ""; 
	}
	else
	{
		data.name = PQgetvalue(res, 0, 4);
	}
	if (PQgetisnull(res, 0, 5))
	{
		data.public_part = ""; 
	}
	else
	{
		data.public_part = PQgetvalue(res, 0, 5);
	}
	if (PQgetisnull(res, 0, 6))
	{
		data.private_part = ""; 
	}
	else
	{
		data.private_part = PQgetvalue(res, 0, 6);
	}
	if (PQgetisnull(res, 0, 7))
	{
		data.notbefore = ""; 
	}
	else
	{
		data.notbefore = PQgetvalue(res, 0, 7);
	}
	if (PQgetisnull(res, 0, 8))
	{
		data.notafter = ""; 
	}
	else
	{
		data.notafter = PQgetvalue(res, 0, 8);
	}

	PQclear(res);
	return 0;
}

int db_fetch_rootcertificate_internal_prepared(const std::string &query, std::vector<const char*> &paramvalues, PGconn * dbconn, DBRootCertificate::Data &data)
{
	PGresult *res;
	int numtuples;

	if (dbconn == NULL)
	{
		syslog(LOG_CRIT, "db_fetch_rootcertificate_internal connection invalid");
		return 1;
	}
	if (check_DB_connection_and_reconnect(&dbconn))
	{
		syslog(LOG_CRIT, "db_fetch_rootcertificate_internal cant reconnect DB connection");
		return 1;
	}
	
	int noOfValues = paramvalues.size();

	PQprepare(dbconn, "", query.c_str(), noOfValues, NULL);

	int paramLengths[noOfValues];
	for(int i = 0; i < noOfValues; i++)
	{
		paramLengths[i] = strlen(paramvalues[i]);
	}
	
	res = PQexecPrepared(dbconn, "", noOfValues, &paramvalues[0], paramLengths, NULL, 1);
		
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "db_fetch_rootcertificate_internal_prepared: Answer not ok\n");
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
		data.created = "";
	}
	else
	{
		data.created = PQgetvalue(res, 0, 1);
	}

	if (PQgetisnull(res, 0, 2))
	{
		data.expiration_days = 0;
	}
	else
	{
		data.expiration_days = atoi(PQgetvalue(res, 0, 2));
	}

	if (PQgetisnull(res, 0, 3))
	{
		data.organisation_id = 0;
	}
	else
	{
		data.organisation_id = atoi(PQgetvalue(res, 0, 3));
	}
	if (PQgetisnull(res, 0, 4))
	{
		data.name = ""; 
	}
	else
	{
		data.name = PQgetvalue(res, 0, 4);
	}
	if (PQgetisnull(res, 0, 5))
	{
		data.public_part = ""; 
	}
	else
	{
		data.public_part = PQgetvalue(res, 0, 5);
	}
	if (PQgetisnull(res, 0, 6))
	{
		data.private_part = ""; 
	}
	else
	{
		data.private_part = PQgetvalue(res, 0, 6);
	}
	if (PQgetisnull(res, 0, 7))
	{
		data.notbefore = ""; 
	}
	else
	{
		data.notbefore = PQgetvalue(res, 0, 7);
	}
	if (PQgetisnull(res, 0, 8))
	{
		data.notafter = ""; 
	}
	else
	{
		data.notafter = PQgetvalue(res, 0, 8);
	}

	PQclear(res);
	return 0;
}

int db_fetch_rootcertificates_internal(const std::string &query, PGconn * dbconn, std::vector<DBRootCertificate::Data> &data)
{
	PGresult *res;
	int numtuples;

	if (dbconn == NULL)
	{
		syslog(LOG_CRIT, "db_fetch_rootcertificates_internal connection invalid");
		return 1;
	}
	if (check_DB_connection_and_reconnect(&dbconn))
	{
		syslog(LOG_CRIT, "db_fetch_rootcertificates_internal cant reconnect DB connection");
		return 1;
	}

	res = PQexec(dbconn, query.c_str());
	
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "db_fetch_rootcertificates_internal: Answer not ok\n");
		PQclear(res);
	}

	numtuples = PQntuples(res);
	data.resize(numtuples);
	
	if(numtuples == 0)
	{
		return 1;
	}
	
	data.resize(numtuples);	
	
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
			data[i].created = "";
		}
		else
		{
			data[i].created = PQgetvalue(res, i, 1);
		}

		if (PQgetisnull(res, i, 2))
		{
			data[i].expiration_days = 0;
		}
		else
		{
			data[i].expiration_days = atoi(PQgetvalue(res, i, 2));
		}

		if (PQgetisnull(res, i, 3))
		{
			data[i].organisation_id = 0;
		}
		else
		{
			data[i].organisation_id = atoi(PQgetvalue(res, i, 3));
		}
		if (PQgetisnull(res, i, 4))
		{
			data[i].name = ""; 
		}
		else
		{
			data[i].name = PQgetvalue(res, i, 4);
		}
		if (PQgetisnull(res, i, 5))
		{
			data[i].public_part = ""; 
		}
		else
		{
			data[i].public_part = PQgetvalue(res, i, 5);
		}
		if (PQgetisnull(res, i, 6))
		{
			data[i].private_part = ""; 
		}
		else
		{
			data[i].private_part = PQgetvalue(res, i, 6);
		}
		if (PQgetisnull(res, i, 7))
		{
			data[i].notbefore = ""; 
		}
		else
		{
			data[i].notbefore = PQgetvalue(res, i, 7);
		}
		if (PQgetisnull(res, i, 8))
		{
			data[i].notafter = ""; 
		}
		else
		{
			data[i].notafter = PQgetvalue(res, i, 8);
		}
	}

	PQclear(res);
	return 0;
}

int db_update_rootcertificate_internal(const std::string &query, PGconn *dbconn)
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

int db_update_rootcertificate_internal_prepared(const std::string &query, std::vector<const char*> &paramvalues, PGconn *dbconn)
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


int DBRootCertificate::db_fetch_by_id(PGconn *dbconn, uint64_t id, DBRootCertificate::Data *data)
{
	std::string query = "SELECT id, created, expiration_days, organisation_id, name, public_part, private_part, notbefore, notafter FROM rootcertificates WHERE id = " + std::to_string(id);
	return db_fetch_rootcertificate_internal(query, dbconn, *data);
}

int DBRootCertificate::db_fetch_by_organisation_name(PGconn *dbconn, const std::string &name, DBRootCertificate::Data *data)
{
	std::string query = "SELECT id, created, expiration_days, organisation_id, name, public_part, private_part, notbefore, notafter FROM rootcertificates WHERE name = $1;";
	std::vector<const char*> paramvalues(1);
	paramvalues[0] = name.c_str();
	return db_fetch_rootcertificate_internal_prepared(query, paramvalues, dbconn, *data);
}

int DBRootCertificate::db_fetch_by_organisation_id(PGconn *dbconn, uint64_t id, DBRootCertificate::Data *data)
{
	std::string query = "SELECT id, created, expiration_days, organisation_id, name, public_part, private_part, notbefore, notafter FROM rootcertificates WHERE organisation_id = " + std::to_string(id);
	return db_fetch_rootcertificate_internal(query, dbconn, *data);
}


int DBRootCertificate::db_fetch_all(PGconn *dbconn, std::vector<DBRootCertificate::Data> *data)
{
	std::string query = "SELECT id, created, expiration_days, organisation_id, name, public_part, private_part, notbefore, notafter FROM rootcertificates";
	return db_fetch_rootcertificates_internal(query, dbconn, *data);
}
/*
int DBRootCertificate::db_update_by_id(PGconn *dbconn, DBRootCertificate::Data *data)
{
	std::string query = "UPDATE rootcertificates SET ";
	if(data->notbefore.length() > 0)
	{
		query += "notbefore = '" + data->notbefore + "', ";
	}
	if(data->notafter.length() > 0)
	{
		query += "notafter = '" + data->notafter + "', ";
	}
	query += "expiration_days = " + std::to_string(data->expiration_days) + ", ";
	query += "organisation_id = " + std::to_string(data->organisation_id) + ", ";
	query += "name = '" + data->name + "', ";
	query += "public_part = '" + data->public_part + "', ";
	query += "private_part = '" + data->private_part + "', ";
	query += "WHERE id = " + std::to_string(data->id);

	return db_update_rootcertificate_internal(query, dbconn);
}
*/
int DBRootCertificate::db_update_by_id(PGconn *dbconn, DBRootCertificate::Data *data)
{
	std::string query = "UPDATE rootcertificates SET expiration_days = $1, organisation_id = $2, name = $3, public_part = $4, private_part = $5 WHERE id = " + std::to_string(data->id) +";";
	
	std::string param0 = std::to_string(data->expiration_days);
	std::string param1 = std::to_string(data->organisation_id);

	std::vector<const char*> paramvalues(5);
	paramvalues[0] = param0.c_str();
	paramvalues[1] = param1.c_str();
	paramvalues[2] = data->name.c_str();
	paramvalues[3] = data->public_part.c_str();
	paramvalues[4] = data->private_part.c_str();

	return db_update_rootcertificate_internal_prepared(query, paramvalues, dbconn);
}
/*
int DBRootCertificate::db_add(PGconn * dbconn, DBRootCertificate::Data *data)
{
	std::string query = "INSERT INTO rootcertificates (expiration_days, organisation_id, name, public_part, private_part) VALUES (";
	if(data->notbefore.length() > 0)
	{
		query += "'" + data->notbefore +"', ";
	}
	if(data->notafter.length() > 0)
	{
		query += "'" + data->notafter + "', ";
	}
	query += std::to_string(data->expiration_days) + ", ";
	query += std::to_string(data->organisation_id) + ", ";
	query += "'" + data->name + "', ";
	query += "'" + data->public_part + "', ";
	query += "'" + data->private_part + "' ";
	query += ")";
	return db_update_rootcertificate_internal(query, dbconn);
}
*/
int DBRootCertificate::db_add(PGconn * dbconn, DBRootCertificate::Data *data)
{

	std::string query = "INSERT INTO rootcertificates (expiration_days, organisation_id, name, public_part, private_part) VALUES($1, $2, $3, $4, $5);";

	std::string param0 = std::to_string(data->expiration_days);
	std::string param1 = std::to_string(data->organisation_id);
	
	std::vector<const char*> paramvalues(5);
	paramvalues[0] = param0.c_str();
	paramvalues[1] = param1.c_str();
	paramvalues[2] = data->name.c_str();
	paramvalues[3] = data->public_part.c_str();
	paramvalues[4] = data->private_part.c_str();
	//paramvalues[5] = data->notbefore.c_str();
	//paramvalues[6] = data->notafter.c_str();

	return db_update_rootcertificate_internal_prepared(query, paramvalues, dbconn);
}

int DBRootCertificate::db_delete_by_id(PGconn *dbconn, uint64_t id)
{
	std::string query = "DELETE FROM rootcertificates WHERE id = " + std::to_string(id);
	return db_update_rootcertificate_internal(query, dbconn);
}

int DBRootCertificate::db_delete_by_organisation_id(PGconn *dbconn, uint64_t id)
{
	std::string query = "DELETE FROM rootcertificates WHERE organisation_id = " + std::to_string(id);
	return db_update_rootcertificate_internal(query, dbconn);
}

