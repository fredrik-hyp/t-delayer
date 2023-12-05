#ifndef __INTERFACES_COMMON_H__
#define __INTERFACES_COMMON_H__

#include <sys/types.h>
#include <unistd.h>
#include <inttypes.h>

typedef struct _interfacememinfo{
	pid_t pid;
	uint64_t streaminterface_id;
	struct timeval tv;
	}interfacememinfo;

#endif
