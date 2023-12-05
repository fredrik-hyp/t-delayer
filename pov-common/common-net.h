#ifndef __COMMON_NET_H__
#define __COMMON_NET_H__

#include <stdio.h>
#include <stdint.h>
#include "dbcommon.h"
#include "dbnet.h"

int maskbitcount(uint8_t mask);
int str2cidr(char *str, int *cidr);
int cidr2str(int cidr, char *res, int len);
int saveDNStoDB(PGconn **dbconn, char *dns);
int constructInterfacesFileFromDB(PGconn **dbconn, char *resultstr, uint64_t len);

#endif