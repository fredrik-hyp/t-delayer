// __DBCLIENTCERTIFICATECPP_H__
#ifndef __DBCLIENTCERTIFICATECPP_H__
#define __DBCLIENTCERTIFICATECPP_H__

#include "dbcommon.h"
#include <string>
#include <vector>

namespace DBClientCertificate
{
	typedef struct _ClientCertificateData
	{
		uint64_t id;
		std::string created;
		int expiration_days;
		int rootcertificate_id;
		std::string name;
		std::string public_part;
		std::string private_part;
		bool server;
		std::string notbefore;
		std::string notafter;
	}Data;

	int db_fetch_by_id(PGconn *, uint64_t, DBClientCertificate::Data *);
	int db_fetch_by_rootcertificate_id(PGconn *, uint64_t, DBClientCertificate::Data *);
	int db_fetch_all(PGconn *, std::vector<DBClientCertificate::Data> *);
	int db_update_by_id(PGconn *, DBClientCertificate::Data *, std::string &);
	int db_add(PGconn *, DBClientCertificate::Data *);
	int db_delete_by_id(PGconn *, uint64_t);
	int db_delete_by_rootcertificate_id(PGconn *, uint64_t);
}

#endif
