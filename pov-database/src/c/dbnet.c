#include "dbnet.h"

int dbfetchnrofnetnics(PGconn **dbconn, int *res)
{
	return DBCount(dbconn, res, "netnics");
}

int dbfetchnrofnetvlans(PGconn **dbconn, int *res)
{
	return DBCount(dbconn, res, "netvlans");
}

void dbnetnicpopulate(PGresult *res, uint64_t row, dbnetnic *netnic)
{
	// == database id ==
	netnic->id = atoll(PQgetvalue(res, row, 0));

	// ==  physicalname ==
	snprintf(netnic->physicalname, DB_MAX_NAME_LENGTH, "%s", PQgetvalue(res, row, 1));

	// == name ==
	if (PQgetisnull(res, row, 2))
	{
		netnic->name[0]  = '\0';
	}
	else
	{
		sprintf(netnic->name, "%s", PQgetvalue(res, row, 2));
	}

	// == inuse ==
	if (strcmp(PQgetvalue(res, row, 3), "t")  )
	{
		netnic->inuse = 1;
	}
	else
	{
		netnic->inuse = 0;
	}

	// == description ==
	if(PQgetisnull(res, 0 , 4))
	{
		netnic->description[0] = '\0';
	}
	else
	{
		snprintf(netnic->description, 255, "%s", PQgetvalue(res, row, 4));	
	}

	// == dhcp ==
	if (strncmp(PQgetvalue(res,row,5), "t",1))
	{
		netnic->dhcp = 1;
	}
	else
	{
		netnic->dhcp = 0;
	}


	// address
	if(PQgetisnull(res, 0, 6))
	{
		netnic->address[0] = '\0';
	}
	else
	{
		snprintf(netnic->address, DB_MAX_HOSTNAME_LENGTH, "%s", PQgetvalue(res,row,6));
	}
	
	// netmask
	if(PQgetisnull(res, 0, 6))
	{
		netnic->netmask[0] = '\0';
	}
	else
	{
		snprintf(netnic->netmask, DB_MAX_HOSTNAME_LENGTH, "%s", PQgetvalue(res,row,7));
	}
	
}

void dbnetvlanpopulate(PGresult *res, uint64_t row, dbnetvlan *netvlan)
{
	// id
	netvlan->id = atoll(PQgetvalue(res, row, 0));
	
	// netnic id
	netvlan->netnic_id = atoll(PQgetvalue(res, row, 1));
	
	// netbridgenic id
	netvlan->netbridgenic_id = atoll(PQgetvalue(res, row, 2));
	
	// vlan id
	netvlan->vlanid = atoll(PQgetvalue(res, row, 3));
	
	// inuse
	if(strncmp(PQgetvalue(res, row, 4), "t",1))
	{
		netvlan->inuse = 1;
	}
	else
	{
		netvlan->inuse = 0;		
	}
		
	// description 
	snprintf(netvlan->description, DB_MAX_HOSTNAME_LENGTH, "%s", PQgetvalue(res, row, 5));

	// dhcp 
	if(strncmp(PQgetvalue(res, row, 6), "t",1))
	{
		netvlan->dhcp = 1;
	}
	else
	{
		netvlan->dhcp = 0;		
	}
	
	// address 
	snprintf(netvlan->address, DB_MAX_HOSTNAME_LENGTH, "%s", PQgetvalue(res, row, 7));
	
	// netmask
	
	snprintf(netvlan->netmask, DB_MAX_HOSTNAME_LENGTH, "%s", PQgetvalue(res, row, 8));
	
	
}

void dbnetroutepopulate(PGresult *res, uint64_t row, dbnetroute *netroute)
{
	// id
	netroute->id = atoll(PQgetvalue(res, row, 0));
	
	// netnic id
	netroute->netnic_id = atoll(PQgetvalue(res, row, 1));
	
	// netvlan id
	netroute->netvlan_id = atoll(PQgetvalue(res, row, 2));
	
	// network
	snprintf(netroute->network, DB_MAX_HOSTNAME_LENGTH, "%s", PQgetvalue(res, row, 3));

	// netmask
	snprintf(netroute->netmask, DB_MAX_HOSTNAME_LENGTH, "%s", PQgetvalue(res, row, 4));
	
	// gateway
	snprintf(netroute->gateway, DB_MAX_HOSTNAME_LENGTH, "%s", PQgetvalue(res, row, 5));
			
	// inuse 
	if(strncmp(PQgetvalue(res,row,6), "t", 1))
	{
		netroute->inuse = 1;
	}
	else
	{
		netroute->inuse = 0;
	}
	// description 
	snprintf(netroute->description, DB_MAX_HOSTNAME_LENGTH, "%s", PQgetvalue(res, row, 7));
}

void dbnetservicepopulate(PGresult *res, uint64_t row, dbnetservice *netservice)
{
	// == database id ==
	netservice->id = atoll(PQgetvalue(res, row, 0));
	
	// == type ==
	snprintf(netservice->type, DB_MAX_HOSTNAME_LENGTH, "%s", PQgetvalue(res,row, 1));
	
	// == address ==
	if(PQgetisnull(res, row, 2))
	{
		netservice->address[0] = '\0';
	}
	else
	{
		snprintf(netservice->address, DB_MAX_HOSTNAME_LENGTH, "%s", PQgetvalue(res, row, 2));
	}
	
	// == options ==
	if(PQgetisnull(res, row, 3))
	{
		netservice->options[0] = '\0';
	}
	else
	{
		snprintf(netservice->options, DB_MAX_HOSTNAME_LENGTH, "%s", PQgetvalue(res, row, 3));
	}
}

int dbfetchnetnics(PGconn **dbconn, dbnetnic *netnics, uint64_t size, uint64_t *reslen)
{
	char query[MEDIUM_QUERY_LEN];
	PGresult *res;
	int numtuples;
	uint64_t i;
	
	*reslen = 0;
	if (dbconn == NULL || netnics == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!", __FUNCTION__);
		return 1;
	}

	if (check_DB_connection_and_reconnect(dbconn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection.",
			__FUNCTION__);
		return 2;
	}

	if (size < 1)
	{
		syslog(LOG_WARNING, "%s size is less then 1",
			__FUNCTION__);
		return 3;
	}
	// Set everything in the struct to 0
	memset(netnics, 0, sizeof(dbnetnic)*size);
	memset(query, 0, MEDIUM_QUERY_LEN);

	int len;
	len = snprintf(query, MEDIUM_QUERY_LEN,
		"SELECT id, physicalname, " \
		" name, " \
		" inuse, "\
		" description, dhcp, address, netmask "\
		" FROM netnics "\
		" order by physicalname LIMIT %"PRIu64,
		size);

	if (len == MEDIUM_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is too small(%d/%d)!",
			 __FUNCTION__, len, MEDIUM_QUERY_LEN);
	}
	
#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n", query);
#endif

	res = PQexec(*dbconn, query);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "%s: Answer not ok\n", __FUNCTION__);
#endif
		PQclear(res);
		return 4;
	}

	numtuples = PQntuples(res);

	// Check number of rows returned 
	if(numtuples == 0)
	{
		PQclear(res);
		return 0;
	}
	else if (numtuples < 0)
	{
		PQclear(res);
		return 5;
	}

#ifdef NODAEMON_DEBUG_DB
	fprintf(stderr, "size: %zd\n\n", sizeof(netnic));
#endif

	for(i=0; i < numtuples && i < size ; i++)
	{
		dbnetnicpopulate(res, i, &(netnics[i]));
	}

	*reslen = numtuples;
	PQclear(res);
	return 0;
}

int dbfetchnetnicbyphysicalname(PGconn **dbconn, char *physicalname, dbnetnic *netnic)
{
	char query[MEDIUM_QUERY_LEN];
	PGresult *res;
	int numtuples, error=0;
	char *physicalname_str;
	
	if (dbconn == NULL || netnic == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!", __FUNCTION__);
		return 1;
	}

	if (check_DB_connection_and_reconnect(dbconn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection.",
			__FUNCTION__);
		return 2;
	}

	// Set everything in the struct to 0
	memset(netnic, 0, sizeof(dbnetnic));
	memset(query, 0, MEDIUM_QUERY_LEN);

	physicalname_str = pqescape(physicalname);
	if(physicalname_str == NULL)
	{
		syslog(LOG_ERR, "%s: Could not allocate memory for physicalname_str", __FUNCTION__);
		return 2;
	}
	
	int len;
	len = snprintf(query, MEDIUM_QUERY_LEN,
		"SELECT id, physicalname, " \
		" name, " \
		" inuse, "\
		" description, dhcp, address, netmask "\
		" FROM netnics "\
		" WHERE physicalname='%s'",
		physicalname_str);

	if (len == MEDIUM_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is too small(%d/%d)!",
			 __FUNCTION__, len, MEDIUM_QUERY_LEN);
	}
	
#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n", query);
#endif

	res = PQexec(*dbconn, query);
	free(physicalname_str);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "%s: Answer not ok\n", __FUNCTION__);
#endif
		PQclear(res);
		return 4;
	}

	numtuples = PQntuples(res);

	// Check number of rows returned 
	if(numtuples == 0)
	{
		PQclear(res);
		return 0;
	}
	else if (numtuples != 1)
	{
		PQclear(res);
		return 5;
	}

#ifdef NODAEMON_DEBUG_DB
	fprintf(stderr, "size: %zd\n\n", sizeof(netnic));
#endif

	dbnetnicpopulate(res, 0, netnic);

	PQclear(res);
	return error;
}

int dbupdatenetnic(PGconn **dbconn, dbnetnic *netnic)
{
	PGresult *res;
	char insert_query[BIG_QUERY_LEN];
	int query_len;
	char *physicalname,
		*name,
		*description,
		*address,
		*netmask;
	char dhcp[12], inuse[12];
		
	if (dbconn == NULL || netnic == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!", __FUNCTION__);
		return 1;
	}

	if (check_DB_connection_and_reconnect(dbconn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection.", __FUNCTION__);
		return 2;
	}

	memset(insert_query, 0, BIG_QUERY_LEN);

	if(netnic->dhcp)
	{
		sprintf(dhcp, "'t'");
	}
	else
	{
		sprintf(dhcp, "'f'");
	}
	
	if(netnic->inuse)
	{
		sprintf(inuse, "'t'");
	}
	else
	{
		sprintf(inuse, "'t'");
	}
	
	physicalname = pqescape(netnic->physicalname);
	if (physicalname == NULL)
	{
		return 4;
	}

	name = pqescape(netnic->name);
	if (name == NULL)
	{
		free(physicalname);
		return 5;
	}

	description = pqescape(netnic->description);
	if (description == NULL)
	{
		free(physicalname);
		free(name);
		return 6;
	}

	address = pqescape(netnic->address);
	if (address == NULL)
	{
		free(physicalname);
		free(name);
		free(description);
		return 7;
	}	

	netmask = pqescape(netnic->netmask);
	if (netmask == NULL)
	{
		free(physicalname);
		free(name);
		free(description);
		free(address);
		return 8;
	}
		
	query_len = snprintf(insert_query, BIG_QUERY_LEN,
		"UPDATE netnics SET physicalname='%s', name='%s', inuse=%s, "\
		" description='%s', dhcp=%s, address='%s', netmask='%s' "\
		" WHERE id=%"PRIu64,
		physicalname,
		name,
		inuse,
		description,
		dhcp,
		address,
		netmask,
		netnic->id);

	if (query_len == BIG_QUERY_LEN || query_len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is to small(%d/%d)!",
			__FUNCTION__, query_len, BIG_QUERY_LEN);
	}

	free(physicalname);
	free(name);
	free(description);
	free(address);
	free(netmask);

#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n", insert_query);
#endif
	res = PQexec(*dbconn, insert_query);
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "%s: Answer not ok...\n", __FUNCTION__);
#endif
		PQclear(res);
		return 9;
	}

	if (atoi(PQcmdTuples(res)) < 1)
	{
		PQclear(res);
		return 10;
	}
	PQclear(res);
	return 0;
}

int dbaddnetnic(PGconn **dbconn, dbnetnic *netnic)
{
	PGresult *res;
	char insert_query[BIG_QUERY_LEN];
	int query_len;
	char *physicalname,
		*name,
		*description,
		*address,
		*netmask;
	char dhcp[12], inuse[12];
		
	if (dbconn == NULL || netnic == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!", __FUNCTION__);
		return 1;
	}

	if (check_DB_connection_and_reconnect(dbconn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection.", __FUNCTION__);
		return 2;
	}

	memset(insert_query, 0, BIG_QUERY_LEN);

	if(netnic->dhcp)
	{
		sprintf(dhcp, "'t'");
	}
	else
	{
		sprintf(dhcp, "'f'");
	}
	
	if(netnic->inuse)
	{
		sprintf(inuse, "'t'");
	}
	else
	{
		sprintf(inuse, "'t'");
	}
	
	physicalname = pqescape(netnic->physicalname);
	if (physicalname == NULL)
	{
		return 4;
	}

	name = pqescape(netnic->name);
	if (name == NULL)
	{
		free(physicalname);
		return 5;
	}

	description = pqescape(netnic->description);
	if (description == NULL)
	{
		free(physicalname);
		free(name);
		return 6;
	}

	address = pqescape(netnic->address);
	if (address == NULL)
	{
		free(physicalname);
		free(name);
		free(description);
		return 7;
	}	

	netmask = pqescape(netnic->netmask);
	if (netmask == NULL)
	{
		free(physicalname);
		free(name);
		free(description);
		free(address);
		return 8;
	}
		
	query_len = snprintf(insert_query, BIG_QUERY_LEN,
		"INSERT INTO netnics (physicalname, name, inuse, "\
		" description, dhcp, address, netmask) "\
		" VALUES ('%s', '%s', %s, '%s', %s, '%s', '%s')",
		physicalname,
		name,
		inuse,
		description,
		dhcp,
		address,
		netmask);

	if (query_len == BIG_QUERY_LEN || query_len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is to small(%d/%d)!",
			__FUNCTION__, query_len, BIG_QUERY_LEN);
	}

	free(physicalname);
	free(name);
	free(description);
	free(address);
	free(netmask);

#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n", insert_query);
#endif
	res = PQexec(*dbconn, insert_query);
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "%s: Answer not ok\n", __FUNCTION__);
#endif
		PQclear(res);
		return 9;
	}

	if (atoi(PQcmdTuples(res)) < 1)
	{
		PQclear(res);
		return 10;
	}
	PQclear(res);
	return 0;
}

int dbfetchnetvlansbynetnicid(PGconn **dbconn, uint64_t netnic_id, dbnetvlan *netvlans, uint64_t size, uint64_t *reslen)
{
	char query[MEDIUM_QUERY_LEN];
	PGresult *res;
	int numtuples, i=0;
	
	*reslen = 0;
	if (dbconn == NULL || netvlans == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!", __FUNCTION__);
		return 1;
	}

	if (check_DB_connection_and_reconnect(dbconn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection.",
			__FUNCTION__);
		return 2;
	}

	memset(query, 0, MEDIUM_QUERY_LEN);

	
	int len;
	len = snprintf(query, MEDIUM_QUERY_LEN,
		"SELECT id, netnic_id, " \
		" netbridgenic_id, " \
		" vlanid, inuse, "\
		" description, dhcp, address, netmask "\
		" FROM netvlans "\
		" WHERE netnic_id=%"PRIu64,
		netnic_id);

	if (len == MEDIUM_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is too small(%d/%d)!",
			 __FUNCTION__, len, MEDIUM_QUERY_LEN);
	}
	
#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n", query);
#endif

	res = PQexec(*dbconn, query);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "%s: Answer not ok\n", __FUNCTION__);
#endif
		PQclear(res);
		return 4;
	}

	numtuples = PQntuples(res);

	// Check number of rows returned 
	if(numtuples == 0)
	{
		PQclear(res);
		return 0;
	}
	else if (numtuples != 1)
	{
		PQclear(res);
		return 5;
	}

#ifdef NODAEMON_DEBUG_DB
	fprintf(stderr, "size: %zd\n\n", sizeof(netnic));
#endif

	for(i=0; i < numtuples && i < size; i++)
	{
		dbnetvlanpopulate(res, i, &(netvlans[i]));
	}
	*reslen = numtuples;
	PQclear(res);
	return 0;	
}

int dbaddnetvlan(PGconn **dbconn, dbnetvlan *netvlan)
{
	PGresult *res;
	char insert_query[BIG_QUERY_LEN];
	int query_len;
	char inuse[12],
		*description,
		dhcp[12],
		*address, 
		*netmask,
		netnicid_str[256],
		netbridgenicid_str[256];
		
	if (dbconn == NULL || netvlan == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!", __FUNCTION__);
		return 1;
	}

	if (check_DB_connection_and_reconnect(dbconn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection.", __FUNCTION__);
		return 2;
	}

	memset(insert_query, 0, BIG_QUERY_LEN);

	if(netvlan->netnic_id == 0)
	{
		sprintf(netnicid_str, "null");
	}
	else
	{
		snprintf(netnicid_str,255, "%"PRIu64, netvlan->netnic_id);
	}
	
	if(netvlan->netbridgenic_id == 0)
	{
		sprintf(netbridgenicid_str, "null");
	}
	else
	{
		snprintf(netbridgenicid_str, 255, "%"PRIu64, netvlan->netbridgenic_id);
	}


	if(netvlan->dhcp)
	{
		sprintf(dhcp, "'t'");
	}
	else
	{
		sprintf(dhcp, "'f'");
	}
	
	if(netvlan->inuse)
	{
		sprintf(inuse, "'t'");
	}
	else
	{
		sprintf(inuse, "'t'");
	}
	

	description = pqescape(netvlan->description);
	if(description == NULL)
	{
		return 3;
	}
	
	address = pqescape(netvlan->address);
	if (address == NULL)
	{
		free(description);
		return 4;
	}

	netmask = pqescape(netvlan->description);
	if (netmask == NULL)
	{
		free(description);
		free(address);
		return 5;
	}
	

	query_len = snprintf(insert_query, BIG_QUERY_LEN,
		"INSERT INTO netvlans (netnic_id, netbridgenic_id, vlanid, "\
		" inuse, description, dhcp, address, netmask) "\
		" VALUES (%s, %s, %d, %s, '%s', %s, '%s', '%s')",
		netnicid_str,
		netbridgenicid_str,
		netvlan->vlanid,
		inuse,
		description,
		dhcp,
		address,
		netmask);

	if (query_len == BIG_QUERY_LEN || query_len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is to small(%d/%d)!",
			__FUNCTION__, query_len, BIG_QUERY_LEN);
	}

	free(description);
	free(address);
	free(netmask);

#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n", insert_query);
#endif
	res = PQexec(*dbconn, insert_query);
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "%s: Answer not ok\n", __FUNCTION__);
#endif
		PQclear(res);
		return 9;
	}

	if (atoi(PQcmdTuples(res)) < 1)
	{
		PQclear(res);
		return 10;
	}
	PQclear(res);
	return 0;
}

int dbfetchnetroutesbynetnicid(PGconn **dbconn, uint64_t netnic_id, dbnetroute *netroutes, uint64_t size, uint64_t *reslen)
{
	return dbfetchnetroutesbynetnicidandvlanid(dbconn, netnic_id, 0, netroutes, size, reslen);
}

int dbfetchnetroutesbynetnicidandvlanid(PGconn **dbconn, uint64_t netnic_id, uint64_t netvlan_id, dbnetroute *netroutes, uint64_t size, uint64_t *reslen)
{
	PGresult *res;
	char query[MEDIUM_QUERY_LEN];
	int i, numtuples, query_len;
	
	*reslen = 0;
	if (dbconn == NULL || netroutes == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!", __FUNCTION__);
		return 1;
	}

	if (check_DB_connection_and_reconnect(dbconn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection.", __FUNCTION__);
		return 2;
	}
	
	if(netvlan_id == 0)
	{
		query_len = snprintf(query, MEDIUM_QUERY_LEN, "SELECT id, netnic_id, netvlan_id, network, netmask, gateway, inuse, description WHERE\
						netnic_id=%"PRIu64 " AND netvlan_id IS NULL", netnic_id);
	}
	else
	{
		query_len = snprintf(query, MEDIUM_QUERY_LEN, "SELECT id, netnic_id, netvlan_id, network, netmask, gateway, inuse, description WHERE\
						netnic_id=%"PRIu64 " AND netvlan_id=%"PRIu64, netnic_id, netvlan_id);		
	}
	
	if (query_len == BIG_QUERY_LEN || query_len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is to small(%d/%d)!",
			__FUNCTION__, query_len, BIG_QUERY_LEN);
	}


#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n", insert_query);
#endif
	res = PQexec(*dbconn, query);
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "%s: Answer not ok\n", __FUNCTION__);
#endif
		PQclear(res);
		return 3;
	}

	numtuples = atoi(PQcmdTuples(res));
	if (numtuples < 1)
	{
		PQclear(res);
		return 4;
	}
	
	for(i = 0; i < numtuples && i < size ; i++)
	{
		dbnetroutepopulate(res, i, netroutes);
	}
	*reslen = numtuples;
	PQclear(res);
	return 0;
}

int dbaddnetroute(PGconn **dbconn, dbnetroute *netroute)
{
	PGresult *res;
	char insert_query[BIG_QUERY_LEN];
	int query_len;
	char netvlanid_str[DB_MAX_HOSTNAME_LENGTH],
		*network,
		*netmask, 
		*gateway,
		inuse_str[12],
		*description;
		
	if (dbconn == NULL || netroute == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!", __FUNCTION__);
		return 1;
	}

	if (check_DB_connection_and_reconnect(dbconn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection.", __FUNCTION__);
		return 2;
	}

	memset(insert_query, 0, BIG_QUERY_LEN);
	memset(netvlanid_str,0, DB_MAX_HOSTNAME_LENGTH);
	
	if(netroute->netvlan_id == 0)
	{
		sprintf(netvlanid_str, "null");
	}
	else
	{
		snprintf(netvlanid_str, 255, "%"PRIu64, netroute->netvlan_id);
	}

	network = pqescape(netroute->network);
	if (network == NULL)
	{
		return 3;
	}
	
	netmask = pqescape(netroute->network);
	if (netmask == NULL)
	{
		free(network);
		return 4;
	}

	gateway = pqescape(netroute->gateway);
	if (gateway == NULL)
	{
		free(network);
		free(netmask);
		return 6;
	}

	description = pqescape(netroute->description);
	if(description == NULL)
	{
		free(network);
		free(netmask);
		free(gateway);
		return 7;
	}
	
	if(netroute->inuse)
	{
		sprintf(inuse_str, "'t'");
	}
	else
	{
		sprintf(inuse_str, "'f'");
	}
	
	query_len = snprintf(insert_query, BIG_QUERY_LEN,
		"INSERT INTO netroutes (netnic_id, netvlan_id, network, "\
		" netmask, gateway, inuse, description) "\
		" VALUES (%"PRIu64", %s, %s, %s, %s, %s, %s)",
		netroute->netnic_id,
		netvlanid_str,
		network,
		netmask,
		gateway,
		inuse_str,
		description);

	if (query_len == BIG_QUERY_LEN || query_len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is to small(%d/%d)!",
			__FUNCTION__, query_len, BIG_QUERY_LEN);
	}

	free(network);
	free(netmask);
	free(gateway);
	free(description);

#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n", insert_query);
#endif
	res = PQexec(*dbconn, insert_query);
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "%s: Answer not ok\n", __FUNCTION__);
#endif
		PQclear(res);
		return 9;
	}

	if (atoi(PQcmdTuples(res)) < 1)
	{
		PQclear(res);
		return 10;
	}
	PQclear(res);
	return 0;
}

int dbdeleteallnetnics(PGconn **dbconn)
{
	char query[SMALL_QUERY_LEN];
	PGresult *res;

	if (dbconn == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!", __FUNCTION__);
		return 1;
	}
	if (check_DB_connection_and_reconnect(dbconn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection", __FUNCTION__);
		return 1;
	}
	
	sprintf(query, "DELETE FROM netnics");
	res = PQexec(*dbconn, query);
   	if(!strcmp(PQcmdTuples(res), ""))
    {
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "Answer not ok %d\n", __LINE__);
#endif
		PQclear(res);
		return -1;
	}	
	
    PQclear(res);
	return 0;   
}

int dbfetchnetservicesbytype(PGconn **dbconn, char *type, dbnetservice *netservices, uint64_t size, uint64_t *reslen)
{
	char query[MEDIUM_QUERY_LEN];
	int query_len;
	uint64_t numtuples;
	PGresult *res;
	char *type_str=NULL;
	uint64_t i;
	
	*reslen = 0;
	if (dbconn == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!", __FUNCTION__);
		return 1;
	}
	if (check_DB_connection_and_reconnect(dbconn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection", __FUNCTION__);
		return 1;
	}
	
	type_str = pqescape(type);
	if(type_str == NULL)
	{
		syslog(LOG_CRIT, "%s type is null", __FUNCTION__);
		return 2;
	}
	
	query_len = snprintf(query, BIG_QUERY_LEN,
		"SELECT id,type, address, options FROM netservices WHERE type='%s' ORDER BY address ",
		type);

	if (query_len == MEDIUM_QUERY_LEN || query_len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is to small(%d/%d)!",
			__FUNCTION__, query_len, MEDIUM_QUERY_LEN);
	}

	free(type_str);
	type_str = NULL;
	
	res = PQexec(*dbconn, query);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "%s: Answer not ok\n", __FUNCTION__);
#endif
		PQclear(res);
		return 4;
	}

	numtuples = PQntuples(res);

	// Check number of rows returned 
	if(numtuples == 0)
	{
		PQclear(res);
		return 0;
	}


	for(i = 0 ; i < numtuples && i < size; i++)
	{		
		dbnetservicepopulate(res, i, &(netservices[i]));
	}
	
	*reslen = numtuples;
	PQclear(res);
	return 0;
	
}

int dbaddnetservice(PGconn **dbconn, dbnetservice *netservice)
{
	char insert_query[BIG_QUERY_LEN];
	int query_len;
	PGresult *res;
	char *type,
	     *address,
		 *options;
		 
	if (dbconn == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!", __FUNCTION__);
		return 1;
	}
	if (check_DB_connection_and_reconnect(dbconn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection", __FUNCTION__);
		return 1;
	}
	
	type = pqescape(netservice->type);
	if(type == NULL)
	{
		syslog(LOG_CRIT, "%s type is null", __FUNCTION__);
		return 2;
	}

	address = pqescape(netservice->address);
	if(address == NULL)
	{
		free(type);
		syslog(LOG_CRIT, "%s address is null", __FUNCTION__);
		return 2;
	}

 	options = pqescape(netservice->options);
	if(options == NULL)
	{
		free(type);
		free(address);
		syslog(LOG_CRIT, "%s options is null", __FUNCTION__);
		return 2;
	}

	query_len = snprintf(insert_query, BIG_QUERY_LEN,
		"INSERT INTO netservices (type, address, options) "\
		" VALUES ('%s', '%s', '%s')",
		type,
		address,
		options);

	if (query_len == BIG_QUERY_LEN || query_len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is to small(%d/%d)!",
			__FUNCTION__, query_len, BIG_QUERY_LEN);
	}

	free(type);
	free(address);
	free(options);

#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n", insert_query);
#endif
	res = PQexec(*dbconn, insert_query);
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "%s: Answer not ok\n", __FUNCTION__);
#endif
		PQclear(res);
		return 9;
	}

	if (atoi(PQcmdTuples(res)) < 1)
	{
		PQclear(res);
		return 10;
	}
	PQclear(res);
	return 0;
}

int dbdeletenetservicesbytype(PGconn **dbconn, char *type)
{
	char query[SMALL_QUERY_LEN];
	PGresult *res;
	char *type_str;
	
	if (dbconn == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!", __FUNCTION__);
		return 1;
	}
	if (check_DB_connection_and_reconnect(dbconn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection", __FUNCTION__);
		return 1;
	}
	
	type_str = pqescape(type);
	if(type_str == NULL)
	{
		syslog(LOG_CRIT, "%s: type is null", __FUNCTION__);
		return 2;
	}


	
	snprintf(query, SMALL_QUERY_LEN, "DELETE FROM netservices WHERE type='%s'", type);
#ifdef NODAEMON_DEBUG_DB_QUERY
		fprintf(stderr, "%s\n", query);
#endif
	
	free(type_str);
	res = PQexec(*dbconn, query);
   	if(atoi(PQcmdTuples(res))  < 0)
    {
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "Answer not ok %d\n", __LINE__);
#endif
		PQclear(res);
		return -1;
	}	
	
    PQclear(res);
	return 0;   
}

int dbfetchnetvlanbynetnicidandvlanid(PGconn **dbconn, uint64_t netnic_id, uint16_t vlanid, dbnetvlan *netvlan)
{
	char query[MEDIUM_QUERY_LEN];
	PGresult *res;
	int numtuples, error=0;
	
	if (dbconn == NULL || netvlan == NULL)
	{
		syslog(LOG_CRIT, "%s argument is NULL!", __FUNCTION__);
		return 1;
	}

	if (check_DB_connection_and_reconnect(dbconn))
	{
		syslog(LOG_CRIT, "%s cant reconnect DB connection.",
			__FUNCTION__);
		return 2;
	}

	// Set everything in the struct to 0
	memset(netvlan, 0, sizeof(dbnetvlan));
	memset(query, 0, MEDIUM_QUERY_LEN);
	
	int len;
	len = snprintf(query, MEDIUM_QUERY_LEN,
		"SELECT id, netnic_id, " \
		" netbridgenic_id, " \
		" vlanid, inuse, "\
		" description, dhcp, address, netmask "\
		" FROM netvlans "\
		" WHERE netnic_id=%"PRIu64" AND vlanid=%d",
		netnic_id, vlanid);

	if (len == MEDIUM_QUERY_LEN || len < 0)
	{
		syslog(LOG_WARNING, "%s snprintf buf is too small(%d/%d)!",
			 __FUNCTION__, len, MEDIUM_QUERY_LEN);
	}
	
#ifdef NODAEMON_DEBUG_DB_QUERY
	fprintf(stderr, "%s\n", query);
#endif

	res = PQexec(*dbconn, query);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
#ifdef NODAEMON_DEBUG_DB
		fprintf(stderr, "%s: Answer not ok\n", __FUNCTION__);
#endif
		PQclear(res);
		return 4;
	}

	numtuples = PQntuples(res);

	// Check number of rows returned 
	if(numtuples == 0)
	{
		PQclear(res);
		return 0;
	}
	else if (numtuples != 1)
	{
		PQclear(res);
		return 5;
	}

#ifdef NODAEMON_DEBUG_DB
	fprintf(stderr, "size: %zd\n\n", sizeof(dbnetvlan));
#endif

	// == database id ==
	netvlan->id = atoll(PQgetvalue(res, 0, 0));

	// == netnicid ==
	if(PQgetisnull(res,0,1))
	{
		netvlan->netnic_id = 0;
	}
	else
	{
		netvlan->netnic_id = atoll(PQgetvalue(res, 0, 1));
	}	

	// == netbridgenicid ==
	if (PQgetisnull(res, 0, 2))
	{
		netvlan->netbridgenic_id = 0;
	}
	else
	{
		netvlan->netbridgenic_id = atoll(PQgetvalue(res, 0, 1));
	}

	// == vlanid ==
	if (PQgetisnull(res, 0, 3))
	{
		netvlan->vlanid = 0;
	}
	else
	{
		netvlan->vlanid = atoi(PQgetvalue(res, 0, 3));
	}

	// == inuse ==
	if(PQgetisnull(res,0,4))
	{
		netvlan->inuse=0;
	}
	else if(!strcmp(PQgetvalue(res, 0, 4), "t"))
	{
		netvlan->inuse = 1;
	}
	else
	{
		netvlan->inuse = 0;
	}
		
	// == description ==
	if (PQgetisnull(res,0,5))
	{
		netvlan->description[0] = '\0';
	}
	else
	{
		snprintf(netvlan->description, DB_MAX_HOSTNAME_LENGTH, "%s", PQgetvalue(res,0,5));
	}

	// == dhcp ==
	if(PQgetisnull(res,0,6))
	{
		netvlan->dhcp=0;
	}
	else if(!strcmp(PQgetvalue(res, 0, 6), "t"))
	{
		netvlan->dhcp = 1;
	}
	else
	{
		netvlan->dhcp = 0;
	}

	// == address ==
	if (PQgetisnull(res, 0, 7))
	{
		netvlan->address[0] = '\0';
	}
	else
	{
		snprintf(netvlan->address, DB_MAX_HOSTNAME_LENGTH, "%s", PQgetvalue(res, 0,7));
	}

	// == netmask ==
	if (PQgetisnull(res, 0, 8))
	{
		netvlan->netmask[0] = '\0';
	}
	else
	{
		snprintf(netvlan->netmask, DB_MAX_HOSTNAME_LENGTH, "%s", PQgetvalue(res, 0,8));
	}

	PQclear(res);
	return error;	
}

