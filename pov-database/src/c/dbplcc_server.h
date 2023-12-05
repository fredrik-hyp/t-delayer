#ifndef __DBPLCC_SERVER_H__
#define __DBPLCC_SERVER_H__

#include "dbcommon.h"

#define PLCC_LISTEN_ADDRESS_LENGTH 1024
#define PLCC_CERT_FILENAME_LENGTH 2048

typedef struct _serverconfig{
	uint64_t id;
	char listen_address[1024];
	uint16_t listen_port;
	char cert_ca[2048];
	char cert_ca_key[2048];
} serverconfig;

int dbplccfetchalldevices(PGconn *dbconn);
int dbplccfetchdevicebyid(PGconn *dbconn);
int dbplccfetchdevicebyserialnumber(PGconn *dbconn);
int dbplccupdatedevice(PGconn *dbconn);

int dbplccfetchallserverconfigs(PGconn *dbconn, int max, serverconfig *serverconfigs, int *read);

int dbplccfetchallconnections(PGconn *dbconn);
int dbplccfetchconnectionbyid(PGconn *dbconn);
int dbplccfetchconnectionbydeviceid(PGconn *dbconn);
int dbplccupdateconnection(PGconn *dbconn);

int dbplccfetchdeviceactionbydeviceid(PGconn *dbconn);
int dbplccupdatedeviceaction(PGconn *dbconn);
#endif