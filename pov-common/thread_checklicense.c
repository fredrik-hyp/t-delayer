#include "common-db.h"
#include "thread_checklicense.h"
#include "interfaces_common.h"
#ifndef EC2VERSION
#include "checklicense.h"
#else
#include "checklicense_ec2.h"
#endif
#include "states.h"

#define MAX_NO_INTERFACES 1024
#define DEAD_TIME       5

int checkStreamInterfaceName(s_interface *streaminterface)
{
	// check if stream name is valid
	if(strncmp(streaminterface->name, "st", 2))
	{
		LOGMSG(LOG_CRIT, "%s: Illegal streaminterface detected, exiting.",
			streaminterface->name);
		return 0;
	}
	return 1;
}

int getStreamInterfaceNumber(s_interface *streaminterface)
{
	int streamnumber = atoi(&(streaminterface->name[2]));
	return streamnumber;
}

void zeroInterfaceInfo(interfacememinfo *imeminfos, int index)
{
	imeminfos[index].pid = 0;
	imeminfos[index].tv.tv_sec = 0;
	imeminfos[index].tv.tv_usec = 0;
	imeminfos[index].streaminterface_id = 0;
}

void check_and_repair_shared_mem(interfacememinfo *imeminfos, int index)
{
	// Low level for user PID are 300 for now
	// #define RESERVED_PIDS           300
	// http://lxr.free-electrons.com/source/kernel/pid.c

	struct timeval tv;
	gettimeofday(&tv, NULL);

	if (imeminfos[index].pid > 0 && 
		(imeminfos[index].tv.tv_sec < (tv.tv_sec-30) ||
		imeminfos[index].tv.tv_sec > (tv.tv_sec+30)))
	{
		LOGMSG(LOG_CRIT, "st%d: Found illegal stats - resetting it!",
			index+1);
		zeroInterfaceInfo(imeminfos, index);
	}
}

void setLicenseStopped(s_interface *streaminterface, PGconn **dbconn, pthread_mutex_t *mutex_dbconn)
{
	s_interfaceruninfo streaminterfaceruninfo;
	memset(&streaminterfaceruninfo, 0, sizeof(s_interfaceruninfo));

	streaminterfaceruninfo.streaminterface_id = streaminterface->id;
	streaminterfaceruninfo.pid = getpid();
	snprintf(streaminterfaceruninfo.startstopinfo, DB_MAX_STARTSTOPINFO, "licensestopped");

	pthread_mutex_lock(mutex_dbconn);
	dbupdatestreaminterfaceruninfo(dbconn, &streaminterfaceruninfo);
	pthread_mutex_unlock(mutex_dbconn);
}

uint16_t getLicensedInterface(uint64_t streaminterfaceid, uint8_t not_blacklabel)
{
	static uint16_t licensedinterface = 0;
	static uint16_t called = 60;
	static uint8_t blacklabel_first_run = 1;

	if (called % 60 == 0)
	{
		if (not_blacklabel)
		{
			licensedinterface = checklic(streaminterfaceid);
		}
		else if (blacklabel_first_run)
		{
			licensedinterface = checklic(streaminterfaceid);
			blacklabel_first_run = 0;
		}
		LOGMSG(LOG_NOTICE, "st%" PRIu64 ": Version %s", 
			streaminterfaceid, VERSION);
		called = 0;
	}
	called++;

	return licensedinterface;
}

int openSharedFile()
{
	int shmfd;
	shmfd = shm_open("/34980724050274", O_RDWR | O_CREAT, 0);
	if (shmfd < 1)
	{
		LOGMSG(LOG_CRIT, "Open IPC resource failed - %s",
				strerror(errno));
		return shmfd;
	}

	int size = sizeof(interfacememinfo)*MAX_NO_INTERFACES;
	struct stat st;
	memset(&st, 0, sizeof(struct stat));

	int stat_ret = fstat(shmfd, &st);
	if (stat_ret != 0)
	{
		LOGMSG(LOG_ERR, "Cant stat file %s (%d)",
			strerror(errno), errno); 
		close(shmfd);
		return 0;
	}
	if (st.st_size != size)
	{
		LOGMSG(LOG_ERR, "Wrong filesize %zd",
			st.st_size);
		close(shmfd);
		return 0;
	}
	return shmfd;
}

void closeSharedFile(int shmfd)
{
	close(shmfd);
}

void *mountSharedFile(int shmfd)
{
	void *mount;
	int size = sizeof(interfacememinfo)*MAX_NO_INTERFACES;
	int mprot = PROT_WRITE | PROT_READ;
	int mflags = MAP_SHARED;
#ifdef NODAEMON_DEBUG
	fprintf(stderr, "size: %d\n", size);
#endif

	mount = mmap(NULL, size, mprot, mflags, shmfd, (off_t)0);
	if (mount == MAP_FAILED)
	{
		LOGMSG(LOG_CRIT, "Cant mount IPC resource - %s",
				strerror(errno));
		close(shmfd);
		return NULL;
	}
	return mount;
}

int unmountSharedFile(void *mount, int shmfd)
{
	int munmapRet = 0;
	int size = sizeof(interfacememinfo)*MAX_NO_INTERFACES;
	munmapRet = munmap(mount, size);
	if (munmapRet == -1)
	{
		LOGMSG(LOG_CRIT, "Cant unmount IPC resource - %s",
				strerror(errno));
		close(shmfd);
		return 0;
	}
	return 1;
}

int countActiveInterfaces(interfacememinfo *imeminfos)
{
	int i;
	int activeCount = 0;
	struct timeval tv;

	gettimeofday(&tv, NULL);
	for (i = 0; i < MAX_NO_INTERFACES; i++)
	{
		check_and_repair_shared_mem(imeminfos, i);
		if (imeminfos[i].pid != 0 &&
			imeminfos[i].tv.tv_sec + DEAD_TIME > tv.tv_sec)
		{
			activeCount++;
		}
	}
	return activeCount;
}

int updateInterfaceInfo(s_interface *streaminterface, interfacememinfo *imeminfos, int index)
{
	pid_t pid = getpid();

	struct timeval tv;
	gettimeofday(&tv, NULL);

//#ifdef NODAEMON_DEBUG
#if 0
	fprintf(stderr, "index: %d\n", index);
	fprintf(stderr, "imeminfos[index].pid: %d\n", imeminfos[index].pid);
	fprintf(stderr, "imeminfos[index].tv.tv_sec: %lu\n", imeminfos[index].tv.tv_sec);
	fprintf(stderr, "imeminfos[index].tv.tv_usec: %lu\n", imeminfos[index].tv.tv_usec);
	fprintf(stderr, "imeminfos[index].streaminterface_id: %" PRIu64 "\n", imeminfos[index].streaminterface_id);
#endif
	
	check_and_repair_shared_mem(imeminfos, index);
	if (imeminfos[index].pid != pid &&
		imeminfos[index].tv.tv_sec + DEAD_TIME > tv.tv_sec)
	{
		LOGMSG(LOG_WARNING, "%s: Another instance is running with our id, exiting. mypid=%d",
			streaminterface->name, getpid());
		LOGMSG(LOG_WARNING, "%" PRIu64 " > %" PRIu64 " ID: %" PRIu64,
			imeminfos[index].tv.tv_sec + DEAD_TIME,
			tv.tv_sec, imeminfos[index].streaminterface_id);
		return 0;
	}

	imeminfos[index].pid = pid;
	imeminfos[index].tv = tv;
	imeminfos[index].streaminterface_id = streaminterface->id;
	return 1;
}

void *thread_checklicense(void *arg)
{
	s_checklicensearg *checkarg = (s_checklicensearg *)arg;
	s_interface *streaminterface = checkarg->streaminterface;
	struct timeval tv;
	uint64_t last_checklicense;
	int streamnumber_index = 0;
	int shmfd = 0;
	interfacememinfo *imeminfos;
	uint8_t not_blacklabel = (get_product() != PRODUCT_BLACKLABEL);
#ifdef NOLICENSECHECK
	int cnt = 0;
#endif

	if (checkStreamInterfaceName(streaminterface) == 0)
	{
		LOGMSG(LOG_WARNING, "%s: checkStreamInterfaceName failed - shutdown",
			streaminterface->name);
		pstate = STATE_SHUTDOWN;
		return NULL;
	}

	// Get streamnumber from streaminterface and turn it into an index
	streamnumber_index = getStreamInterfaceNumber(streaminterface)-1;
	if (streamnumber_index < 0 || streamnumber_index > MAX_NO_INTERFACES)
	{
		LOGMSG(LOG_WARNING, "%s: getStreamInterfaceNumber failed (%d) - shutdown",
			streaminterface->name, streamnumber_index);
		pstate = STATE_SHUTDOWN;
		return NULL;
	}

	// Open shared file and map the file into imeminfos pointer
	shmfd = openSharedFile();
	if (shmfd < 1)
	{
		LOGMSG(LOG_WARNING, "%s: openSharedFile failed - shutdown",
			streaminterface->name);
		pstate = STATE_SHUTDOWN;
		return NULL;
	}

	imeminfos = (interfacememinfo *)mountSharedFile(shmfd);
	if (imeminfos == NULL)
	{
		LOGMSG(LOG_WARNING, "%s: mountSharedFile failed - shutdown",
			streaminterface->name);
		pstate = STATE_SHUTDOWN;
		return NULL;
	}

	gettimeofday(&tv,NULL);
	last_checklicense = tv.tv_sec-10;

	while (pstate != STATE_SHUTDOWN)
	{
		gettimeofday(&tv,NULL);
		if (last_checklicense >= tv.tv_sec)
		{
			usleep(100000);
			continue;
		}

		// possible to compile with no license checks, when we are in trusted environments
#ifdef NOLICENSECHECK
		if(cnt == 0)
		{
			LOGMSG(LOG_NOTICE, "st%" PRIu64 ": Version %s", 
				streaminterface->id, VERSION);			
		}
		cnt++;
		cnt = cnt%60;
#endif
			
#ifndef NOLICENSECHECK
		
		// === CHECK NUMBER OF RUNNING INTERFACE IS WITHIN LIMITS ===
		if (countActiveInterfaces(imeminfos) > getLicensedInterface(streaminterface->id, not_blacklabel))
		{
			LOGMSG(LOG_NOTICE, "%s: Too many active(%d/%d) interface!",
				streaminterface->name,
				countActiveInterfaces(imeminfos),
				getLicensedInterface(streaminterface->id, not_blacklabel));
			setLicenseStopped(streaminterface, checkarg->dbconn, checkarg->mutex_dbconn);

			pstate = STATE_SHUTDOWN;
			continue;
		}
#endif
		// === UPDATE SHARED FILE WITH INFO ===
		if (updateInterfaceInfo(streaminterface, imeminfos, streamnumber_index) == 0)
		{
			pstate = STATE_SHUTDOWN;
			continue;
		}
		// In case of checklicense activity are slow - lets get current time.
		gettimeofday(&tv,NULL);
		last_checklicense = tv.tv_sec;
	}

	LOGMSG(LOG_WARNING, "%s: EXIT thread_checklicense", streaminterface->name);

	// Unset structure for this client
	zeroInterfaceInfo(imeminfos, streamnumber_index);

	// unmount and close file
	if (unmountSharedFile((void *) imeminfos, shmfd) == 0)
	{
		pstate = STATE_SHUTDOWN;
		return NULL;
	}
	closeSharedFile(shmfd);
	return NULL;
}
