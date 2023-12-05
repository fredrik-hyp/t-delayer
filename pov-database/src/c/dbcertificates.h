#ifndef __DBCERTIFICATES_H__
#define __DBCERTIFICATES_H__

#include "dbcommon.h"

typedef struct _dbrootcertificate{
    uint64_t id;
    char name[1024];
}dbrootcertificate;

typedef struct _dbclientcertificate{
    uint64_t id;
    char name[1024];
}dbclientcertificate;

int dbfetchallrootcertificates(PGconn **conn, dbrootcertificate ***rootcertificates, int *numcertificates_copied);
int dbfetchrootcertificatebyorganisationid(PGconn **conn, uint64_t organisationid, dbrootcertificate *rootcertificate);
int dbfetchclientcertificatesbyrootcertificateid(PGconn **conn, uint64_t rootcertificateid, dbclientcertificate ***clientcertificates, int *numcertificates_copied);
#endif