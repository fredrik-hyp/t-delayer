#ifndef __DBDELAYERS_H__
#define __DBDELAYERS_H__

#include "dbcommon.h"

typedef enum _delayer_action {
	JUMP,
	PLAY,
	PAUSE,
	PLAYTO
} e_delayer_action;

typedef struct _delayercoms {
	uint64_t id;
	uint64_t create_timestamp;
	uint64_t streaminterface_id;
	uint64_t pathnumber;
	char action[DB_MAX_MODE_LENGTH]; // See e_delayer_action
	e_delayer_action e_action;
	char jumptopos[DB_MAX_TIMESTAMP];
	uint64_t jumptopos_timestamp;
	char startplayat[DB_MAX_TIMESTAMP];
	uint64_t startplayat_timestamp;
	char playto[DB_MAX_TIMESTAMP];
	uint64_t playto_timestamp;
} s_delayercoms;

typedef struct _delayerstats {
	uint64_t id;
	uint64_t streaminterface_id;
	uint64_t pathnumber;
	char state[DB_MAX_MODE_LENGTH]; // play | pause
	uint64_t startplayat_timestamp;
	uint64_t startpos_timestamp;
	uint64_t lastpos_timestamp;
	uint64_t currpos_timestamp;
	uint64_t playto_timestamp;
} s_delayerstats;


// -- delayercoms - arguments to the delayer --
int db_fetch_delayercoms(PGconn **conn, s_delayercoms *delayercom,  int maxsize, int *retvalues, uint64_t streaminterface_id);

int db_delete_delayercoms(PGconn **conn, uint64_t streaminterface_id);
int db_delete_delayercomsbyid(PGconn **conn, uint64_t id);
int db_count_delayercoms(PGconn **conn, uint64_t streaminterfacenr);

int db_insert_delayercom(PGconn **conn, s_delayercoms *delayercom);

int db_insert_delayerstats(PGconn **conn, s_delayerstats *delayerstats);
int db_fetch_delayerstatsbystreaminterfaceid(PGconn **conn, s_delayerstats *delayerstats, int maxsize, int *retvalues, uint64_t streaminterface_id);
int db_delete_delayerstats(PGconn **conn, uint64_t streaminterface_id);

#endif
