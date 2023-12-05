#ifndef __DBNET_H__
#define __DBNET_H__
#include "dbcommon.h"

#define DBNETSERVICE_TYPE_DNS "dns"
#define DBNETSERVICE_TYPE_DEFGW "defgw"
 
typedef struct __dbnetnic
{
	uint64_t id;
	char physicalname[DB_MAX_HOSTNAME_LENGTH];
	char name[DB_MAX_HOSTNAME_LENGTH];
	int inuse;
	char description[DB_MAX_HOSTNAME_LENGTH];
	int dhcp;
	char address[DB_MAX_HOSTNAME_LENGTH];
	char netmask[DB_MAX_HOSTNAME_LENGTH];
}dbnetnic;

typedef struct __dbnetvlan
{
	uint64_t id;
	uint64_t netnic_id;
	uint64_t netbridgenic_id;
	uint16_t vlanid;
	int inuse;
	char description[DB_MAX_HOSTNAME_LENGTH];
	int dhcp;
	char address[DB_MAX_HOSTNAME_LENGTH];
	char netmask[DB_MAX_HOSTNAME_LENGTH];	
}dbnetvlan;

typedef struct __dbnetroute
{
	uint64_t id;
	uint64_t netnic_id;
	uint64_t netvlan_id;
	char network[DB_MAX_HOSTNAME_LENGTH];
	char netmask[DB_MAX_HOSTNAME_LENGTH];
	char gateway[DB_MAX_HOSTNAME_LENGTH];
	int inuse;
	char description[DB_MAX_HOSTNAME_LENGTH];
}dbnetroute;

typedef struct __dbnetservice
{
	uint64_t id;
	char type[DB_MAX_HOSTNAME_LENGTH];
	char address[DB_MAX_HOSTNAME_LENGTH];
	char options[DB_MAX_HOSTNAME_LENGTH];
}dbnetservice;

int dbfetchnetnicbyphysicalname(PGconn **dbconn, char *physicalname, dbnetnic *netnic);
int dbfetchnetnics(PGconn **dbconn, dbnetnic *netnics, uint64_t size, uint64_t *reslen);

int dbupdatenetnic(PGconn **dbconn, dbnetnic *netnic);
int dbaddnetnic(PGconn **dbconn, dbnetnic *netnic);
int dbdeleteallnetnics(PGconn **dbconn);

int dbfetchnetvlansbynetnicid(PGconn **dbconn, uint64_t netnic_id, dbnetvlan *netvlans, uint64_t size, uint64_t *reslen);
int dbfetchnetvlanbynetnicidandvlanid(PGconn **dbconn, uint64_t netnic_id, uint16_t vlanid, dbnetvlan *netvlan);
int dbaddnetvlan(PGconn **dbconn, dbnetvlan *netvlan);

int dbfetchnetroutesbynetnicid(PGconn **dbconn, uint64_t netnic_id, dbnetroute *netroutes, uint64_t size, uint64_t *reslen);
int dbfetchnetroutesbynetnicidandvlanid(PGconn **dbconn, uint64_t netnic_id, uint64_t netvlan_id, dbnetroute *netroutes, uint64_t size, uint64_t *reslen);
int dbaddnetroute(PGconn **dbconn, dbnetroute *netroute);

int dbfetchnetservicesbytype(PGconn **dbconn, char *type, dbnetservice *netservices, uint64_t size, uint64_t *reslen);
int dbaddnetservice(PGconn **dbconn, dbnetservice *netservice);
int dbdeletenetservicesbytype(PGconn **dbconn, char *type);
#endif


