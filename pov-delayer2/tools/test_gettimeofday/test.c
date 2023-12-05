#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>

int
delayer_gettimeofday(struct timeval *tv, struct timezone *tz)
{
	// timeval [tv_sec, tv_usec]
	static struct timeval initial_tv = { 0 };
	// timespec [tv_sec, tv_nsec]
	struct timespec now_ts;
	static bool initial_call = false;
	int ret;

	if (initial_call) {
		ret = clock_gettime(CLOCK_MONOTONIC_RAW, &now_ts);
		if (ret != 0)
			return ret;
		
		if (tv->tv_usec + (now_ts.tv_nsec / 1000) > 1000000) {
			tv->tv_sec = initial_tv.tv_sec + now_ts.tv_sec+1;
			tv->tv_usec = 1000000 - (initial_tv.tv_usec + now_ts.tv_nsec/1000);
		} else {
			tv->tv_sec = initial_tv.tv_sec + now_ts.tv_sec;
			tv->tv_usec = (initial_tv.tv_usec + now_ts.tv_nsec/1000);
		}
printf("clock_gettime %ld %ld\n", now_ts.tv_sec, now_ts.tv_nsec);
		
	} else {
		ret = gettimeofday(&initial_tv, NULL);
		if (ret != 0)
			return ret;
		ret = clock_gettime(CLOCK_MONOTONIC_RAW, &now_ts);
		if (ret != 0)
			return ret;

		initial_call = true;

		*tv = initial_tv;

printf("gettimeofday %ld %ld\n", initial_tv.tv_sec, initial_tv.tv_usec);
printf("clock_gettime %ld %ld\n", now_ts.tv_sec, now_ts.tv_nsec);
		// Compensate the intial_tv once
		initial_tv.tv_sec -= now_ts.tv_sec;
		if (initial_tv.tv_usec > (now_ts.tv_nsec/1000)) {
			initial_tv.tv_usec -= now_ts.tv_nsec/1000;
		} else {
			initial_tv.tv_sec--;
			initial_tv.tv_usec = 1000000 - now_ts.tv_nsec/1000;
		}
printf("comp gettimeofday %ld %ld\n", initial_tv.tv_sec, initial_tv.tv_usec);
	}
	return ret;
}

int
main()
{
	struct timeval tv;
	uint64_t now_time;
	for (int i = 0; i < 10000; i++) {
		delayer_gettimeofday(&tv, NULL);
		now_time = tv.tv_sec*1000 + tv.tv_usec/1000;	
		printf("delayer:\t\t%"PRIu64"\n", now_time);
		gettimeofday(&tv, NULL);
		now_time = tv.tv_sec*1000 + tv.tv_usec/1000;	
		printf("gettimeofday:\t\t%"PRIu64"\n", now_time);
		sleep(4);
	}
}


