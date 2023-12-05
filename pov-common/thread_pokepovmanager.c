#include "thread_pokepovmanager.h"
#include "common.h"
#include "common-db.h"
#include "states.h"

void pokePovManager(uint64_t streaminterfaceid)
{
	int sockfd, servlen,result;
	struct sockaddr_un  serv_addr;
	char buffer[82];
	fd_set writefds;

	memset(&serv_addr,0,sizeof(struct sockaddr_un));
	serv_addr.sun_family = AF_UNIX;

	strcpy(serv_addr.sun_path, SOCK_POVMANAGER);
	servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);
	if ((sockfd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK,0)) < 0)
	{
		LOGMSG(LOG_WARNING, "st%" PRIu64 ": pokePovManager: Cant create socket: %d",
			streaminterfaceid, errno);
		return;
	}

#if 0
// DEBUGISH
	LOGMSG(LOG_NOTICE, "st%" PRIu64 ": Connecting %d %" PRIu64,
		streaminterfaceid, sockfd, streaminterfaceid);
#endif
	if (connect(sockfd, (struct sockaddr *) &serv_addr, servlen) < 0)
	{
		if (errno == EWOULDBLOCK)
		{
			struct timeval tv;
			tv.tv_sec = 0;
			tv.tv_usec = 50000; // 5/100 sec
			FD_ZERO(&writefds);
			FD_SET(sockfd, &writefds);
			result = select(sockfd+1, NULL, &writefds, NULL, &tv);
			if (result < 1)
			{
				close(sockfd);
				return;
			}
		}
		else
		{
			close(sockfd);
			return;
		}
	}
	
   	sprintf(buffer,"%" PRIu64, streaminterfaceid);
//	fprintf(stderr, "sending: %s\n", buffer);
//	LOGMSG(LOG_NOTICE, "sending: %s", buffer);
	result = write(sockfd,buffer,strlen(buffer));

#if 0
// Kind of debugcode
	if (result == -1)
		LOGMSG(LOG_NOTICE, "st%" PRIu64 ": result after sending %d - %s",
			streaminterfaceid, result, strerror(errno));
	else
		LOGMSG(LOG_NOTICE, "st%" PRIu64 ": result after sending %d",
			streaminterfaceid, result);
#endif
	close(sockfd);

}

void *thread_pokepovmanager(void *arg)
{
	s_interface *streaminterface = (s_interface *)arg;
	struct timeval tv;
	uint64_t last_pokepovmanager;

	last_pokepovmanager =  0;
	
	while (pstate != STATE_SHUTDOWN)
	{
		gettimeofday(&tv,NULL);
		if (last_pokepovmanager >= tv.tv_sec)
		{
			usleep(100000);
			continue;
		}

		pokePovManager(streaminterface->id);

		last_pokepovmanager = tv.tv_sec;
	}

	// Poke povmanager one last time
	// Got aprox 5 Sec to switch database to stopped state
	pokePovManager(streaminterface->id);

	LOGMSG(LOG_WARNING, "%s: EXIT thread_pokepovmanager", streaminterface->name);

	return NULL;
}
