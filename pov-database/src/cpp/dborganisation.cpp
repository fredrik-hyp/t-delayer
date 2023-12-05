#include "dbcommon.h"
#include "dborganisation.h"

int db_fetch_organisation_internal(const std::string &query, PGconn * dbconn, DBOrganisation::Data &data)
{
	PGresult *res;
	int numtuples;

	if (dbconn == NULL)
	{
		syslog(LOG_CRIT, "db_fetch_organisation_internal connection invalid");
		return 1;
	}
	if (check_DB_connection_and_reconnect(&dbconn))
	{
		syslog(LOG_CRIT, "db_fetch_organisation_internal cant reconnect DB connection");
		return 1;
	}

	res = PQexec(dbconn, query.c_str());
	
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "db_fetch_organisation_internal: Answer not ok\n");
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
		data.name = "";
	}
	else
	{
		data.name = PQgetvalue(res, 0, 1);
	}

	if (PQgetisnull(res, 0, 2))
	{
		data.address1 = "";
	}
	else
	{
		data.address1 = PQgetvalue(res, 0, 2);
	}

	if (PQgetisnull(res, 0, 3))
	{
		data.address2 = "";
	}
	else
	{
		data.address2 = PQgetvalue(res, 0, 3);
	}
	if (PQgetisnull(res, 0, 4))
	{
		data.zipcode = ""; 
	}
	else
	{
		data.zipcode = PQgetvalue(res, 0, 4);
	}
	if (PQgetisnull(res, 0, 5))
	{
		data.city = ""; 
	}
	else
	{
		data.city = PQgetvalue(res, 0, 5);
	}
	if (PQgetisnull(res, 0, 6))
	{
		data.country = ""; 
	}
	else
	{
		data.country = PQgetvalue(res, 0, 6);
	}
	if (PQgetisnull(res, 0, 7))
	{
		data.contactperson = ""; 
	}
	else
	{
		data.contactperson = PQgetvalue(res, 0, 7);
	}
	if (PQgetisnull(res, 0, 8))
	{
		data.telephone = ""; 
	}
	else
	{
		data.telephone = PQgetvalue(res, 0, 8);
	}
	if (PQgetisnull(res, 0, 9))
	{
		data.email = ""; 
	}
	else
	{
		data.email = PQgetvalue(res, 0, 9);
	}
	if (PQgetisnull(res, 0, 10))
	{
		data.comments = ""; 
	}
	else
	{
		data.comments = PQgetvalue(res, 0, 10);
	}

	PQclear(res);
	return 0;
}

int db_fetch_organisation_internal(const std::string &query, PGconn * dbconn, std::vector<DBOrganisation::Data> &data)
{
	PGresult *res;
	int numtuples;

	if (dbconn == NULL)
	{
		syslog(LOG_CRIT, "db_fetch_organisations_internal connection invalid");
		return 1;
	}
	if (check_DB_connection_and_reconnect(&dbconn))
	{
		syslog(LOG_CRIT, "db_fetch_organisations_internal cant reconnect DB connection");
		return 1;
	}

	res = PQexec(dbconn, query.c_str());
	
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "db_fetch_organisations_internal: Answer not ok\n");
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
			data[i].name = "";
		}
		else
		{
			data[i].name = PQgetvalue(res, i, 1);
		}

		if (PQgetisnull(res, i, 2))
		{
			data[i].address1 = "";
		}
		else
		{
			data[i].address1 = PQgetvalue(res, i, 2);
		}

		if (PQgetisnull(res, i, 3))
		{
			data[i].address2 = "";
		}
		else
		{
			data[i].address2 = PQgetvalue(res, i, 3);
		}
		if (PQgetisnull(res, i, 4))
		{
			data[i].zipcode = ""; 
		}
		else
		{
			data[i].zipcode = PQgetvalue(res, i, 4);
		}
		if (PQgetisnull(res, i, 5))
		{
			data[i].city = ""; 
		}
		else
		{
			data[i].city = PQgetvalue(res, i, 5);
		}
		if (PQgetisnull(res, i, 6))
		{
			data[i].country = ""; 
		}
		else
		{
			data[i].country = PQgetvalue(res, i, 6);
		}
		if (PQgetisnull(res, i, 7))
		{
			data[i].contactperson = ""; 
		}
		else
		{
			data[i].contactperson = PQgetvalue(res, i, 7);
		}
		if (PQgetisnull(res, i, 8))
		{
			data[i].telephone = ""; 
		}
		else
		{
			data[i].telephone = PQgetvalue(res, i, 8);
		}
		if (PQgetisnull(res, i, 9))
		{
			data[i].email = ""; 
		}
		else
		{
			data[i].email = PQgetvalue(res, i, 9);
		}
		if (PQgetisnull(res, i, 10))
		{
			data[i].comments = ""; 
		}
		else
		{
			data[i].comments = PQgetvalue(res, i, 10);
		}
	}

	PQclear(res);
	return 0;
}

int db_update_organisation_internal(const std::string &query, PGconn *dbconn)
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

int db_update_organisation_internal_prepared(const std::string &query, std::vector<const char*> &paramvalues, PGconn *dbconn)
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

int DBOrganisation::db_fetch_by_id(PGconn *dbconn, uint64_t id, DBOrganisation::Data *data)
{
	std::string query = "SELECT id, name, address1, address2, zipcode, city, country, contactperson, telephone, email, comments FROM organisations WHERE id = " + std::to_string(id);
	return db_fetch_organisation_internal(query, dbconn, *data);
}

int DBOrganisation::db_fetch_all(PGconn *dbconn, std::vector<DBOrganisation::Data> *data)
{
	std::string query = "SELECT id, name, address1, address2, zipcode, city, country, contactperson, telephone, email, comments FROM organisations";
	return db_fetch_organisation_internal(query, dbconn, *data);
}
/*
int DBOrganisation::db_update_by_id(PGconn *dbconn, DBOrganisation::Data *data)
{
	std::string query = "UPDATE organisations SET ";
	query += "name = '" + data->name + "', ";
	query += "address1 = '" + data->address1 + "', ";
	query += "address2 = '" + data->address2 + "', ";
	query += "zipcode = '" + data->zipcode + "', ";
	query += "city = '" + data->city + "', ";
	query += "country = '" + data->country + "', ";
	query += "contactperson = '" + data->contactperson + "', ";
	query += "telephone = '" + data->telephone + "', ";
	query += "email = '" + data->email + "', ";
	query += "comments = '" + data->comments + "' ";
	query += "WHERE id = " + std::to_string(data->id);

	return db_update_organisation_internal(query, dbconn);
}
*/
int DBOrganisation::db_update_by_id(PGconn *dbconn, DBOrganisation::Data *data)
{
	std::string query = "UPDATE organisations SET name = $1, address1 = $2, address2 = $3, zipcode = $4, city = $5, country = $6, contactperson = $7, telephone = $8, email = $9, comments = $10 WHERE id = " + std::to_string(data->id) + ";";

	std::vector<const char*> paramvalues(10);
	paramvalues[0] = data->name.c_str();
	paramvalues[1] = data->address1.c_str();
	paramvalues[2] = data->address2.c_str();
	paramvalues[3] = data->zipcode.c_str();
	paramvalues[4] = data->city.c_str();
	paramvalues[5] = data->country.c_str();
	paramvalues[6] = data->contactperson.c_str();
	paramvalues[7] = data->telephone.c_str();
	paramvalues[8] = data->email.c_str();
	paramvalues[9] = data->comments.c_str();

	return db_update_organisation_internal_prepared(query, paramvalues, dbconn);
}
/*
int DBOrganisation::db_add(PGconn * dbconn, DBOrganisation::Data *data)
{
	std::string query = "INSERT INTO organisations (name, address1, address2, zipcode, city, country, contactperson, telephone, email, comments) VALUES (";
	query += "'" + data->name + "', ";
	query += "'" + data->address1 + "', ";
	query += "'" + data->address2 + "', ";
	query += "'" + data->zipcode + "', ";
	query += "'" + data->city + "', ";
	query += "'" + data->country + "', ";
	query += "'" + data->contactperson + "', ";
	query += "'" + data->telephone + "', ";
	query += "'" + data->email + "', ";
	query += "'" + data->comments + "'";
	query += ")";

	return db_update_organisation_internal(query, dbconn);
}
*/
int DBOrganisation::db_add(PGconn * dbconn, DBOrganisation::Data *data)
{
	std::string query = "INSERT INTO organisations (name, address1, address2, zipcode, city, country, contactperson, telephone, email, comments) VALUES ($1, $2, $3, $4, $5, $6, $7 ,$8 ,$9, $10)";
	
	std::vector<const char*> paramvalues(10);
	paramvalues[0] = data->name.c_str();
	paramvalues[1] = data->address1.c_str();
	paramvalues[2] = data->address2.c_str();
	paramvalues[3] = data->zipcode.c_str();
	paramvalues[4] = data->city.c_str();
	paramvalues[5] = data->country.c_str();
	paramvalues[6] = data->contactperson.c_str();
	paramvalues[7] = data->telephone.c_str();
	paramvalues[8] = data->email.c_str();
	paramvalues[9] = data->comments.c_str();

	return db_update_organisation_internal_prepared(query, paramvalues, dbconn);
}

int DBOrganisation::db_delete_by_id(PGconn *dbconn, uint64_t id)
{
	std::string query = "DELETE FROM organisations WHERE id = " + std::to_string(id);
	return db_update_organisation_internal(query, dbconn);
}

