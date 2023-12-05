// __DBROOTCERTIFICATECPP_H__
#ifndef __DBROOTCERTIFICATECPP_H__
#define __DBROOTCERTIFICATECPP_H__

#include "dbcommon.h"
#include <string>
#include <vector>

namespace DBRootCertificate
{
	typedef struct _RootCertificateData
	{
		uint64_t id;
		std::string created;		
		int expiration_days;
		int organisation_id;
		std::string name;
		std::string public_part;
		std::string private_part;		
		std::string notbefore;
		std::string notafter;
	}Data;

	int db_fetch_by_id(PGconn *, uint64_t, DBRootCertificate::Data *);
	int db_fetch_by_organisation_name(PGconn*, const std::string &name, DBRootCertificate::Data *);
	int db_fetch_by_organisation_id(PGconn *, uint64_t, DBRootCertificate::Data *);
	int db_fetch_all(PGconn *, std::vector<DBRootCertificate::Data> *);
	int db_update_by_id(PGconn *, DBRootCertificate::Data *);
	int db_add(PGconn *, DBRootCertificate::Data *);
	int db_delete_by_id(PGconn *, uint64_t);
	int db_delete_by_organisation_id(PGconn *, uint64_t);
}

#endif
