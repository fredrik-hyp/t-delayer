#ifndef __DBORGANSATIONS_H__
#define __DBORGANSATIONS_H__

#include "dbcommon.h"

typedef struct _organisation{
    uint64_t id;
    char name[1024];
}dborganisation;

int dbfetchallorganisations(PGconn **conn, dborganisation ***organistions, int *numorganisations_copied);
#endif
