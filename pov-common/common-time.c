#include "common-time.h"
int64_t calc_timeval_diff(struct timeval *now, struct timeval *past)
{
	if (now->tv_sec > past->tv_sec)
	{
		// Ex 2.90 - 1.80
		if (now->tv_usec >= past->tv_usec)
		{
			int64_t usec = now->tv_usec - past->tv_usec;
			int64_t sec = now->tv_sec - past->tv_sec;
			return sec*1000000 + usec;
		}
		// Ex 2.10 - 1.90
		else
		{
			int64_t usec = 1000000 + now->tv_usec - past->tv_usec;
			int64_t sec = (now->tv_sec - 1) - past->tv_sec;
			return sec*1000000 + usec;
		}
	}
	else if (now->tv_sec == past->tv_sec)
	{
		return now->tv_usec - past->tv_usec;
	}
	else
	{
		if (now->tv_usec >= past->tv_usec)
		{
			int64_t usec = now->tv_usec - (past->tv_usec+1000000);
			int64_t sec = now->tv_sec - (past->tv_sec-1);
			return sec*1000000+usec;
		}
		else
		{
			int64_t usec = now->tv_usec - past->tv_usec;
			int64_t sec = now->tv_sec - past->tv_sec;
			return sec*1000000+usec;
		}
	}
	return 0;
}

void add_timeout(struct timespec *result, struct timeval *now, int64_t timeout_usec)
{
	if (now->tv_usec + timeout_usec > 1000000)
	{
		int nsec = (now->tv_usec+timeout_usec)/1000000;
		result->tv_sec = now->tv_sec+nsec;
		result->tv_nsec = (now->tv_usec + timeout_usec - nsec*1000000)*1000;
		return;
	}
	// else
	result->tv_sec = now->tv_sec;
	result->tv_nsec = (now->tv_usec+timeout_usec)*1000;
	return;
}
