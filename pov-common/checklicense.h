#ifndef __CHECKLICENSE_H__
#define __CHECKLICENSE_H__

#include "common.h"
#include <openssl/sha.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <net/if.h> 
#include <netinet/in.h>
#include <dirent.h>

uint16_t checklic(uint64_t streaminterfaceid);
#endif
