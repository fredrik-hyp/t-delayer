#ifndef __DBFILE_H__
#define __DBFILE_H__

#include "dbcommon.h"

typedef enum _file_status {
	CREATE,
	QUEUED,
	INTRANSIT,
	ONHOLD,
	DONE,
	CHKSFAIL
} e_file_status;

typedef struct _file {
        uint64_t id;
        uint64_t streaminterface_id;
        char filename[DB_MAX_NAME_LENGTH];
        uint64_t size;
	uint8_t size_not_changed_counter; // Internal only!
	uint64_t priority;
	uint64_t queuedtime;
	uint64_t startsendtime;
	uint64_t stopsendtime;
	uint64_t transferredbytes;
	e_file_status e_status;
	// Create, Queued, Intransit, Done
	char status[DB_MAX_MODE_LENGTH];
	// Other
	char comment[DB_MAX_NAME_LENGTH];
} s_file;


// Query everything streaminterface_id
int db_fetch_file(PGconn **conn, s_file *file, int maxsize, int *retvalues, uint64_t streaminterface_id);
// Query streaminterface_id AND status='Create'
int db_fetch_file_create(PGconn **conn, s_file *file, int maxsize, int *retvalues, uint64_t streaminterface_id);
// Query streaminterface_id AND status='Queued'
int db_fetch_file_queued(PGconn **conn, s_file *file, int maxsize, int *retvalues, uint64_t streaminterface_id);
// Query streaminterface_id AND status='Intransit'
int db_fetch_file_intransit(PGconn **conn, s_file *file, int maxsize, int *retvalues, uint64_t streaminterface_id);
// Query streaminterface_id AND status='Onhold'
int db_fetch_file_onhold(PGconn **conn, s_file *file, int maxsize, int *retvalues, uint64_t streaminterface_id);
// Query streaminterface_id AND status='Queued' order by queuedtime, priority
int db_fetch_file_to_transfer(PGconn **conn, s_file *file, int maxsize, int *retvalues, uint64_t streaminterface_id);
// Query streaminterface_id AND status!='Done'
int db_fetch_file_not_done(PGconn **conn, s_file *file, int maxsize, int *retvalues, uint64_t streaminterface_id);

int db_fetch_file_by_name(PGconn **conn, s_file *file, int maxsize, int *retvalues, char *filename, uint64_t streaminterface_id);

int db_delete_filebyinterface(PGconn **conn, uint64_t streaminterface_id);
int db_delete_filebynameandinterface(PGconn **conn, char *filename, uint64_t streaminterface_id);
int db_delete_filebyid(PGconn **conn, uint64_t id);
int db_delete_intransit(PGconn **conn, uint64_t streaminterface_id);


// Fetch database id for the filename in combination with Onhold
int db_fetch_file_id(PGconn **conn, s_file *file);
// Fetch transferredbytes for for the file_id
int db_fetch_file_transferredbytes(PGconn **conn, s_file *file);
// insert basic are a subset of s_file struct
int db_insert_basic_file(PGconn **conn, s_file *file);
// Will insert all elements in s_file struct
int db_insert_file(PGconn **conn, s_file *file);

// only update the comment
int db_update_comment(PGconn **conn, s_file *file);
// Only update size
int db_update_file_size(PGconn **conn, s_file *file);
// Only update transferredbytes
int db_update_file_transferredbytes(PGconn **conn, s_file *file);


// Update status to QUEUED and set queuedtime to now()
int db_update_file_to_queued(PGconn **conn, s_file *file);
// Update status to INTRANSIT and set startsendtime to now()
int db_update_file_to_intransit(PGconn **conn, s_file *file);
// Update status to ONHOLD and set stopsendtime to now()
int db_update_file_to_onhold(PGconn **conn, s_file *file);
// Update status to DONE and set stopsendtime to now()
int db_update_file_to_done(PGconn **conn, s_file *file);
// Update status to CHKSFAIL and set stopsendtime to now()
int db_update_file_to_chksfail(PGconn **conn, s_file *file);
// Update all field in s_file struct
int db_update_file(PGconn **conn, s_file *file);


#endif // __DBFILE_H__
