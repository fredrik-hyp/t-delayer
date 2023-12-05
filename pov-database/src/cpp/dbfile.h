// __DBFILECPP_H__
#ifndef __DBFILECPP_H__
#define __DBFILECPP_H__

#include "dbcommon.h"
#include "md5.h"
#include <string>
#include <vector>

typedef enum _file_status
{
	CREATE,
	QUEUED,
	INTRANSIT,
	ONHOLD,
	DONE,
	CHKSFAIL
}file_status;

namespace DBFile
{
	typedef struct _FileData
	{
		uint64_t id;
		uint64_t streaminterface_id;
		std::string filename;
		uint64_t size;
		uint64_t priority;
		std::string queuedtime;
		std::string startsendtime;
		std::string stopsendtime;
		uint64_t transferredbytes;
		file_status e_status;
		std::string status;
		std::string comment;
		unsigned char md5sum[MD5_DIGEST_LENGTH];
	}Data;
	
	int db_fetch_by_id(PGconn *, uint64_t, DBFile::Data *);
	int db_fetch_by_id_epoch_time(PGconn *, uint64_t, DBFile::Data *);
	int db_fetch_by_streaminterface_id(PGconn *, uint64_t, std::vector<DBFile::Data> *);
	int db_fetch_all(PGconn *, std::vector<DBFile::Data> *);
	int db_fetch_most_recent_on_receiver(PGconn *, std::vector<DBFile::Data> *);
	int db_fetch_most_recent_on_transmitter(PGconn *, std::vector<DBFile::Data> *);
	int db_fetch_by_status(PGconn *, const std::string &, std::vector<DBFile::Data> *);
	int db_set_priority_by_id(PGconn *, uint64_t, int);
	int db_set_priority_for_folder(PGconn *, std::string, uint64_t, int);
	int db_set_status_by_id(PGconn *, uint64_t, const std::string &);
	int db_add_file(PGconn *, const DBFile::Data *);
	int db_delete_file_by_name_and_interface(PGconn *conn, const std::string &, uint64_t);
	int db_clear_all(PGconn *);
	int db_clear_interface(PGconn *, uint64_t);
	int db_clear_file(PGconn *, uint64_t);
	//int db_check_if_inactive(PGconn *, const std::string &, uint64_t);
}

#endif
