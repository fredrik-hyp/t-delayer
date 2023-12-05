#ifndef __DBPLCCS_H__
#define __DBPLCCS_H__

#include "dbcommon.h"

#define PLCC_CONNECT_SERVER_LENGTH 1024
#define PLCC_CERT_FILENAME_LENGTH 1024

typedef struct _plcc{
	uint64_t id;
	char connect_server[PLCC_CONNECT_SERVER_LENGTH];
	uint16_t connect_port;
	char cert_ca[PLCC_CERT_FILENAME_LENGTH]; 
	char cert_client[PLCC_CERT_FILENAME_LENGTH];
	char cert_client_key[PLCC_CERT_FILENAME_LENGTH];
} plcc;

int dbfetchallplccs(PGconn *conn, int no_of_plccs, plcc *plccs, int *numplccs_copied);

int dbfetch

#endif