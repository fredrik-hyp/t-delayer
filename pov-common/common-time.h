#ifndef __COMMON_TIME_H__
#define __COMMON_TIME_H__

#include <stdint.h>
#include <sys/time.h>

int64_t calc_timeval_diff(struct timeval *now, struct timeval *past);
void add_timeout(struct timespec *result, struct timeval *now, int64_t timeout_usec);
#endif
