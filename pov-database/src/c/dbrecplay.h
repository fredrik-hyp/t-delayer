#ifndef __DBRECPLAY_H__
#define __DBRECPLAY_H__

#include "dbcommon.h"

typedef enum _recorder_state {
	INIT,
	RECORDING,
	DONE,
	PAUSED
} e_recorder_state;

// e_state [INIT | RECORDING | DONE]
typedef struct _recordings
{
	uint64_t id;
	uint64_t streaminterface_id;
	char name[DB_MAX_NAME_LENGTH];
	char state[DB_MAX_STATE];
	e_recorder_state e_state;
	uint64_t recordingstart_timestamp;
	uint64_t recordingend_timestamp;
	char sourceaddress[DB_MAX_HOSTNAME_LENGTH];
	uint16_t sourceport;
} s_recordings;


// e_state [RECORDING | PAUSE]
typedef struct _recorderstats
{
	uint64_t id;
	uint64_t streaminterface_id;
	char state[DB_MAX_STATE];
	e_recorder_state e_state;
	uint64_t startrecordat_timestamp;
	uint64_t startpos_timestamp;
	uint64_t lastpos_timestamp;
	uint64_t currpos_timestamp;
	uint64_t recordto_timestamp;
} s_recorderstats;

typedef enum _player_action {
	JUMP,
	PLAY,
	PAUSE,
	PLAYTO
} e_player_action;

// e_action [JUMP | PLAY | PAUSE | PLAYTO}
typedef struct _playercoms
{
	uint64_t id;
	uint64_t streaminterface_id;
	uint64_t pathnumber;
	uint64_t create_timestamp;
	char action[DB_MAX_STATE];
	e_player_action e_action;
	char jumptopos[DB_MAX_TIMESTAMP];
	uint64_t jumptopos_timestamp;
	char startplayat[DB_MAX_TIMESTAMP];
	uint64_t startplayat_timestamp;
	char playto[DB_MAX_TIMESTAMP];
	uint64_t playto_timestamp;
} s_playercoms;

// e_state are a subset of e_player_action [PLAY | PAUSE]
typedef struct _playerstats
{
	uint64_t id;
	uint64_t streaminterface_id;
	uint64_t pathnumber;
	uint64_t recording_id;
	char state[DB_MAX_STATE];
	e_player_action e_state;
	uint64_t startplayat_timestamp;
	uint64_t startpos_timestamp;
	uint64_t lastpos_timestamp;
	uint64_t currpos_timestamp;
	uint64_t playto_timestamp;
} s_playerstats;


// -- Recordings --
int db_fetch_recordings(PGconn **conn, s_recordings *recording, int maxsize, int *retvalues, uint64_t streaminterface_id, e_recorder_state req_state);
int db_fetch_recording(PGconn **conn, s_recordings *recording, uint64_t recorderid);
int db_fetch_recordingbyname(PGconn **conn, s_recordings *recording, char *name);
int db_update_recordings(PGconn **conn, s_recordings *recording);
int db_insert_recordings(PGconn **conn, s_recordings *recording);
int db_delete_recording(PGconn **conn, uint64_t recordingid);

// -- recorder stats --
int db_insert_recorderstats(PGconn **conn, s_recorderstats *recorderstats);
int db_fetch_recorderstatsbystreaminterfaceid(PGconn **conn, s_recorderstats *recorderstats, int maxsize, int *retvalues, uint64_t streaminterface_id);
int db_delete_recorderstats(PGconn **conn, uint64_t streaminterface_id);


// -- Playercom --
int db_fetch_playercoms(PGconn **conn, s_playercoms *playercom, int maxsize, int *retvalues, uint64_t streaminterface_id);
int db_delete_playercoms(PGconn **conn, uint64_t streaminterface_id);
int db_delete_playercomsbyid(PGconn **conn, uint64_t id);
int db_count_playercoms(PGconn **conn, uint64_t streaminterface_id);
int db_insert_playercom(PGconn **conn, s_playercoms *delayercom);

// -- Playerstats --
int db_insert_playerstats(PGconn **conn, s_playerstats *playerstats);
int db_fetch_playerstatsbystreaminterfaceid(PGconn **conn, s_playerstats *playerstats, int maxsize, int *retvalues, uint64_t streaminterface_id);
int db_delete_playerstats(PGconn **conn, uint64_t streaminterface_id);

#endif // __DBRECPLAY_H__
