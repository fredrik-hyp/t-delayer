// __DBORGANISATIONCPP_H__
#ifndef __DBORGANISATIONCPP_H__
#define __DBORGANISATIONCPP_H__

#include "dbcommon.h"
#include <string>
#include <vector>

namespace DBOrganisation
{
	typedef struct _OrganisationData
	{
		uint64_t id;
		std::string name;
		std::string address1;
		std::string address2;
		std::string zipcode;
		std::string city;
		std::string country;
		std::string contactperson;
		std::string telephone;
		std::string email;
		std::string comments;
	}Data;

	int db_fetch_by_id(PGconn *, uint64_t, DBOrganisation::Data *);
	int db_fetch_all(PGconn *, std::vector<DBOrganisation::Data> *);
	int db_update_by_id(PGconn *, DBOrganisation::Data *);
	int db_add(PGconn *, DBOrganisation::Data *);
	int db_delete_by_id(PGconn *, uint64_t);
}

#endif
