#ifndef __THREAD_CHECKLICENSE_H__
#define __THREAD_CHECKLICENSE_H__

#include <sys/mman.h>
#include <sys/time.h>

void *thread_checklicense(void *arg);

typedef struct _checklicensearg
{
	s_interface *streaminterface;
	PGconn			**dbconn;
	pthread_mutex_t		*mutex_dbconn;
} s_checklicensearg;


#endif
