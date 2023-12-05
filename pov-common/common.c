#include "common.h"

#define CMD_ARRAY_SIZE 2092
size_t trimwhitespace(char *out, size_t len, const char *str)
{
	if (len == 0)
	{
		return 0;
	}

	const char *end;
	size_t out_size;

	// Trim leading space
	while (isspace(*str))
	{
		str++;
	}

	if (*str == 0) // All spaces?
	{
		*out = 0;
		return 1;
	}

	// Trim trailing space
	end = str + strlen(str) - 1;
	while (end > str && isspace(*end))
	{
		end--;
	}
	end++;

	// Set output size to minimum of trimmed string length and buffer size minus 1
	out_size = (end - str) < len-1 ? (end - str) : len-1;

	// Copy trimmed string and add null terminator
	memcpy(out, str, out_size);
	out[out_size] = 0;

	return out_size;
}

int hostnametoip(char *hostname, char *ip, bool log_errors)
{
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_in *h4;
	struct sockaddr_in6 *h6;
	struct in_addr ipaddrtest;
	int rv;

	// if valid ipv4 address, return directly
	if (inet_aton(hostname, &ipaddrtest)!= 0)
	{
		strcpy(ip, hostname);
		return 0;
	}

	memset(&hints, 0, sizeof(struct addrinfo));
//	hints.ai_family = AF_UNSPEC; // use AF_INET6 to force IPv6
	hints.ai_family = AF_INET; // use AF_INET6 to force IPv6
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(hostname, NULL, &hints, &servinfo)) != 0)
	{
		if (log_errors)
		{
			LOGMSG(LOG_WARNING, "hostnametoip: getaddrinfo: %s",
				gai_strerror(rv));
		}
		return 1;
	}

	for (p = servinfo; p != NULL; p = p->ai_next)
	{
		const char *ret;
		switch (p->ai_family)
		{
		case AF_INET:
			h4 = (struct sockaddr_in *) p->ai_addr;
			ret = inet_ntop(p->ai_family, &(h4->sin_addr), ip, 255);
			break;
		case AF_INET6:
			h6 = (struct sockaddr_in6 *) p->ai_addr;
			ret = inet_ntop(p->ai_family, &(h6->sin6_addr), ip, 255);
			break;
		}
		if (ret != NULL)
		{
			// Got an adress return
			freeaddrinfo(servinfo); // all done with this structure
#ifdef NODAEMON_DEBUG
			LOGMSG(LOG_WARNING, "ip: %s", ip);
#endif
			return 0;
		}
		// An error
		LOGMSG(LOG_WARNING, "inet_ntop: error converting hostname to ip %s",
			strerror(errno));
		// Loop and see if there are more information
	}

	// Didnt found any ipaddress

	freeaddrinfo(servinfo); // all done with this structure
	return 2;
}

void daemonize()
{
	pid_t pid;
	// Clone ourselves to make a child
	pid = fork(); 

	// If the pid is less than zero,
	// something went wrong when forking
	if (pid < 0)
	{
		LOGMSG(LOG_WARNING, "fork failed - %s %d",
			strerror(errno), errno);
		exit(EXIT_FAILURE);
	}

	// If the pid we got back was greater
	// than zero, then the clone was
	// successful and we are the parent.
	if (pid > 0)
	{
		exit(EXIT_SUCCESS);
	}
#if 0
	// If execution reaches this point we are the child
	pid = fork(); // double fork
	if (pid > 0)
	{
		exit(EXIT_SUCCESS);
	}
#endif
	
	// Set the umask to zero
	umask(0);
	
	// Sends a message to the syslog daemon
	LOGMSG(LOG_NOTICE, "Successfully started daemon %d",
		pid); 

	// this is optional and only needs to be done when your daemon exits
	//closelog();

	// Close the standard file descriptors
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
	if (open("/dev/null",O_RDONLY) == -1)
	{
		LOGMSG(LOG_WARNING, "failed to reopen stdin while daemonising (errno=%d)",
			errno);
	}
	if (open("/dev/null",O_WRONLY) == -1)
	{
		LOGMSG(LOG_WARNING, "failed to reopen stdout while daemonising (errno=%d)",
			errno);
	}
	if (open("/dev/null",O_RDWR) == -1)
	{
		LOGMSG(LOG_WARNING, "failed to reopen stderr while daemonising (errno=%d)",
			errno);
	}

	pid_t sid;

	// Try to create our own process group
	sid = setsid();
	if (sid < 0)
	{
		LOGMSG(LOG_ERR, "Could not create process group (errno=%d)",
			errno);
		exit(EXIT_FAILURE);
	}
	
	// Change the current working directory
	if ((chdir("/")) < 0)
	{
		LOGMSG(LOG_ERR, "Could not change working directory to / (errno=%d)",
			errno);
		exit(EXIT_FAILURE);
	}
}

void print_usage(char **argv)
{
	fprintf(stderr, "USAGE: %s <streaminterface> [nodaemon]\n", argv[0]);
	fprintf(stderr, "       streaminterface\t\tconfigured interface on this server\n");
	fprintf(stderr, "       nodaemon\t\t\tdont daemonize application\n");
	fprintf(stderr, "       -v\t\t\tprint version\n");
}

void parse_argument(int argc, char **argv, char *version, int *run_as_daemon)
{
	switch(argc)
	{
	case 2:
		if (strcmp(argv[1], "-v") == 0)
		{
			fprintf(stderr, "Version: %s\n", version);
			exit(0);
		}
		break;
	case 3:
		if(strcmp(argv[2], "nodaemon") == 0)
		{
			*run_as_daemon=0;
		}
		else
		{
			print_usage(argv);
			exit(1);
		}
		break;
	default:
		print_usage(argv);
		exit(1);
	}
}

// Create pid file named pidfilename and write current process pid inside.
int create_pid(const char *pidfilename)
{
	int fd;
	int flags;
	char buf[128];

	fd = open(pidfilename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	if (fd == -1)
	{
		LOGMSG(LOG_WARNING, "Cant open PID file %s - %s(%d)",
			pidfilename, strerror(errno), errno);
		return -1;
	}
	flags = fcntl(fd, F_GETFD);
	if (flags == -1)
	{
		LOGMSG(LOG_WARNING, "Cant get flags for PID file %s - %s(%d)",
			pidfilename, strerror(errno), errno);
		close(fd);
		return -1;
	}
	flags |= FD_CLOEXEC;
	if (fcntl(fd, F_SETFD, flags) == -1)
	{
		LOGMSG(LOG_WARNING, "Cant set flags for PID file %s - %s(%d)",
			pidfilename, strerror(errno), errno);
		close(fd);
		return -1;
	}
	if (ftruncate(fd, 0) == -1)
	{
		LOGMSG(LOG_WARNING, "Cant truncate PID file %s - %s(%d)",
			pidfilename, strerror(errno), errno);
		close(fd);
		return -1;
	}
	snprintf(buf, 128, "%ld\n", (long) getpid());
	if (write(fd, buf, strlen(buf)) != strlen(buf))
	{
		LOGMSG(LOG_WARNING, "Cant write to PID file %s - %s(%d)",
			pidfilename, strerror(errno), errno);
		close(fd);
		return -1;
	}
	return fd;
}

// Close file and unlink file
int close_pid_file(int pidfile, const char *pidfilename)
{
	int cret = close(pidfile);
	if (cret == -1)
	{
		LOGMSG(LOG_WARNING, "close(%d) failed - %s(%d)",
			pidfile, strerror(errno), errno);
		return -1;
	}

	int uret = unlink(pidfilename);
	if (uret == -1)
	{
		LOGMSG(LOG_WARNING, "unlink file %s failed - %s(%d)",
			pidfilename, strerror(errno), errno);
		return -1;
	}

	return 0;
}

int read_pid_file(const char *pidfilename)
{

	int fd;

	fd = open(pidfilename, O_RDONLY);
	if (fd == -1)
	{
		LOGMSG(LOG_WARNING, "Cant open PID file %s - %s(%d)",
			pidfilename, strerror(errno), errno);
		return -1;
	}

	ssize_t read_size;
	char buf[128];
	read_size = read(fd, buf, 128);
	close(fd);
	if (read_size == -1)
	{
		LOGMSG(LOG_WARNING, "Cant read PID file %s - %s(%d)",
			pidfilename, strerror(errno), errno);
		return -1;
	}
	
	long pid;
	pid = strtol(buf, NULL, 10);
	if (pid == LONG_MIN || pid == LONG_MAX)
	{
		LOGMSG(LOG_WARNING, "Cant convert PID to int %s - %s(%d)",
			pidfilename, strerror(errno), errno);
		return -1;
	}

	return (int)pid;
}

int checkPidFile(char *pidfilename)
{
	if (access(pidfilename, F_OK) == -1)
	{
		return 0;
	}

	int pid;
	pid = read_pid_file(pidfilename);
	if (pid > 0)
	{
		int retkill;
		retkill = kill(pid, 0);
		// Is process alive?
		if (retkill == 0)
		{
			return pid;
		}

		// No its not alive :/
		// We have the pid-file but it doesnt seems that the process is running
		// unlink the file
		int retunlink;
		retunlink = unlink(pidfilename);
		if (retunlink == -1)
		{
			LOGMSG(LOG_WARNING, "Cant unlink pid file %s - %s(%d)",
					pidfilename, strerror(errno), errno);
		}
	}
	return 0;
}


// Get the pid of the interface(name) if its running
int getInterfacePid(char *progname, char *interfacename)
{
	char pidfilename[MAX_PID_FILENAME];
	int szret;
	int pid;

	// Test if its an receiver or transmitter or forwarder or delayer
	// ======= RECEIVER =========
	szret = snprintf(pidfilename, MAX_PID_FILENAME, PID_RECEIVER_FILENAME_PATTERN, interfacename);
	if (szret < 0 || szret >= MAX_PID_FILENAME)
	{
		LOGMSG(LOG_WARNING, "#1 getInterfacePid truncated string or error %d",
			szret);
	}
	pid = checkPidFile(pidfilename);
	if (pid)
	{
		return pid;
	}

	// ======= TRANSMITTER =========
	szret = snprintf(pidfilename, MAX_PID_FILENAME, PID_TRANSMITTER_FILENAME_PATTERN, interfacename);
	if (szret < 0 || szret >= MAX_PID_FILENAME)
	{
		LOGMSG(LOG_WARNING, "#2 getInterfacePid truncated string or error %d",
			szret);
	}
	pid = checkPidFile(pidfilename);
	if (pid)
	{
		return pid;
	}

	// ======= FORWARDER =========
	szret = snprintf(pidfilename, MAX_PID_FILENAME, PID_FORWARDER_FILENAME_PATTERN, interfacename);
	if (szret < 0 || szret >= MAX_PID_FILENAME)
	{
		LOGMSG(LOG_WARNING, "#3 getInterfacePid truncated string or error %d",
			szret);
	}
	pid = checkPidFile(pidfilename);
	if (pid)
	{
		return pid;
	}

	// ======= DELAYER =========
	szret = snprintf(pidfilename, MAX_PID_FILENAME, PID_DELAYER_FILENAME_PATTERN, interfacename);
	if (szret < 0 || szret >= MAX_PID_FILENAME)
	{
		LOGMSG(LOG_WARNING, "#4 getInterfacePid truncated string or error %d",
			szret);
	}
	pid = checkPidFile(pidfilename);
	if (pid)
	{
		return pid;
	}

	// ======= RECORDER =========
	szret = snprintf(pidfilename, MAX_PID_FILENAME, PID_RECORDER_FILENAME_PATTERN, interfacename);
	if (szret < 0 || szret >= MAX_PID_FILENAME)
	{
		LOGMSG(LOG_WARNING, "#5 getInterfacePid truncated string or error %d",
			szret);
	}
	pid = checkPidFile(pidfilename);
	if (pid)
	{
		return pid;
	}

	// ======= PLAYER =========
	szret = snprintf(pidfilename, MAX_PID_FILENAME, PID_PLAYER_FILENAME_PATTERN, interfacename);
	if (szret < 0 || szret >= MAX_PID_FILENAME)
	{
		LOGMSG(LOG_WARNING, "#6 getInterfacePid truncated string or error %d",
			szret);
	}
	pid = checkPidFile(pidfilename);
	if (pid)
	{
		return pid;
	}

	// ======= RUDP FILE RECEIVER =========
	szret = snprintf(pidfilename, MAX_PID_FILENAME, PID_FILE_RECEIVER_FILENAME_PATTERN, interfacename);
	if (szret < 0 || szret >= MAX_PID_FILENAME)
	{
		LOGMSG(LOG_WARNING, "#7 getInterfacePid truncated string or error %d",
			szret);
	}
	pid = checkPidFile(pidfilename);
	if (pid)
	{
		return pid;
	}

	// ======= RUDP FILE TRANSMITTER =========
	szret = snprintf(pidfilename, MAX_PID_FILENAME, PID_FILE_TRANSMITTER_FILENAME_PATTERN, interfacename);
	if (szret < 0 || szret >= MAX_PID_FILENAME)
	{
		LOGMSG(LOG_WARNING, "#8 getInterfacePid truncated string or error %d",
			szret);
	}
	pid = checkPidFile(pidfilename);
	if (pid)
	{
		return pid;
	}

	// Fallback use the old way with ps aux
	FILE *fp;
	char cmd[CMD_ARRAY_SIZE];
	char str[CMD_ARRAY_SIZE];

	// sprintf(cmd, "/bin/ps auxwww|/bin/grep \"er %s\"|grep -v %s", interfacename, progname);
	szret = snprintf(cmd, CMD_ARRAY_SIZE, "/bin/ps auxwww|/bin/grep \"er %s\"|grep -v %s|grep -v \"er %s[0-9]\"",
			interfacename, progname, interfacename);
	if (szret < 0 || szret >= CMD_ARRAY_SIZE)
	{
		LOGMSG(LOG_WARNING, "#3 getInterfacePid truncated string or error %d",
			szret);
	}

	// TODO Reuse result from snprintf to calculate position to start copy and length of the array left
	int strsize = strlen(cmd);
	strncpy(cmd + strsize, "|/bin/grep -v grep|awk '{printf(\"%s\\n\",$2)}'", CMD_ARRAY_SIZE-strsize);

	fp = popen(cmd, "r");
	if (fp == NULL)
	{
		LOGMSG(LOG_WARNING, "error popen - %s",
			strerror(errno));
		return 0;
	}

	while (fgets(str, CMD_ARRAY_SIZE-1, fp) != NULL)
	{
		pclose(fp);
		return atoi(str);
	}
	pclose(fp);
	return 0;
}

int removePidFile(char *interfacename)
{
	char pidfilename[MAX_PID_FILENAME];
	int szret;

	// Test if its an receiver or transmitter or forwarder
	// ======= RECEIVER =========
	szret = snprintf(pidfilename, MAX_PID_FILENAME, PID_RECEIVER_FILENAME_PATTERN, interfacename);
	if (szret < 0 || szret >= MAX_PID_FILENAME)
	{
		LOGMSG(LOG_WARNING, "#1 removePidFile truncated string or error %d",
			szret);
	}
	if (access(pidfilename, F_OK) != -1)
	{
		int retunlink = unlink(pidfilename);
		if (retunlink == -1)
		{
			LOGMSG(LOG_WARNING, "Cant unlink pid file %s - %s(%d)",
				pidfilename, strerror(errno), errno);
		}
		return 0;
	}
	
	// ======= TRANSMITTER =========
	szret = snprintf(pidfilename, MAX_PID_FILENAME, PID_TRANSMITTER_FILENAME_PATTERN, interfacename);
	if (szret < 0 || szret >= MAX_PID_FILENAME)
	{
		LOGMSG(LOG_WARNING, "#2 removePidFile truncated string or error %d", szret);
	}
	if (access(pidfilename, F_OK) != -1)
	{
		int retunlink = unlink(pidfilename);
		if (retunlink == -1)
		{
			LOGMSG(LOG_WARNING, "Cant unlink pid file %s - %s(%d)",
				pidfilename, strerror(errno), errno);
		}
		return 0;
	}

	// ======= FORWARDER =========
	szret = snprintf(pidfilename, MAX_PID_FILENAME, PID_FORWARDER_FILENAME_PATTERN, interfacename);
	if (szret < 0 || szret >= MAX_PID_FILENAME)
	{
		LOGMSG(LOG_WARNING, "#3 removePidFile truncated string or error %d",
			szret);
	}
	if (access(pidfilename, F_OK) != -1)
	{
		int retunlink = unlink(pidfilename);
		if (retunlink == -1)
		{
			LOGMSG(LOG_WARNING, "Cant unlink pid file %s - %s(%d)",
				pidfilename, strerror(errno), errno);
		}
		return 0;
	}

	// ======= DELAYER =========
	szret = snprintf(pidfilename, MAX_PID_FILENAME, PID_DELAYER_FILENAME_PATTERN, interfacename);
	if (szret < 0 || szret >= MAX_PID_FILENAME)
	{
		LOGMSG(LOG_WARNING, "#4 removePidFile truncated string or error %d",
			szret);
	}
	if (access(pidfilename, F_OK) != -1)
	{
		int retunlink = unlink(pidfilename);
		if (retunlink == -1)
		{
			LOGMSG(LOG_WARNING, "Cant unlink pid file %s - %s(%d)",
				pidfilename, strerror(errno), errno);
		}
		return 0;
	}

	// ======= RECORDER =========
	szret = snprintf(pidfilename, MAX_PID_FILENAME, PID_RECORDER_FILENAME_PATTERN, interfacename);
	if (szret < 0 || szret >= MAX_PID_FILENAME)
	{
		LOGMSG(LOG_WARNING, "#5 removePidFile truncated string or error %d",
			szret);
	}
	if (access(pidfilename, F_OK) != -1)
	{
		int retunlink = unlink(pidfilename);
		if (retunlink == -1)
		{
			LOGMSG(LOG_WARNING, "Cant unlink pid file %s - %s(%d)",
				pidfilename, strerror(errno), errno);
		}
		return 0;
	}

	// ======= PLAYER =========
	szret = snprintf(pidfilename, MAX_PID_FILENAME, PID_PLAYER_FILENAME_PATTERN, interfacename);
	if (szret < 0 || szret >= MAX_PID_FILENAME)
	{
		LOGMSG(LOG_WARNING, "#6 removePidFile truncated string or error %d", szret);
	}
	if (access(pidfilename, F_OK) != -1)
	{
		int retunlink = unlink(pidfilename);
		if (retunlink == -1)
		{
			LOGMSG(LOG_WARNING, "Cant unlink pid file %s - %s(%d)",
				pidfilename, strerror(errno), errno);
		}
		return 0;
	}

	// ======= RUDP FILE RECEIVER =========
	szret = snprintf(pidfilename, MAX_PID_FILENAME, PID_FILE_RECEIVER_FILENAME_PATTERN, interfacename);
	if (szret < 0 || szret >= MAX_PID_FILENAME)
	{
		LOGMSG(LOG_WARNING, "#7 removePidFile truncated string or error %d",
			szret);
	}
	if (access(pidfilename, F_OK) != -1)
	{
		int retunlink = unlink(pidfilename);
		if (retunlink == -1)
		{
			LOGMSG(LOG_WARNING, "Cant unlink pid file %s - %s(%d)",
				pidfilename, strerror(errno), errno);
		}
		return 0;
	}
	
	// ======= RUDP FILE TRANSMITTER =========
	szret = snprintf(pidfilename, MAX_PID_FILENAME, PID_FILE_TRANSMITTER_FILENAME_PATTERN, interfacename);
	if (szret < 0 || szret >= MAX_PID_FILENAME)
	{
		LOGMSG(LOG_WARNING, "#8 removePidFile truncated string or error %d",
			szret);
	}
	if (access(pidfilename, F_OK) != -1)
	{
		int retunlink = unlink(pidfilename);
		if (retunlink == -1)
		{
			LOGMSG(LOG_WARNING, "Cant unlink pid file %s - %s(%d)",
				pidfilename, strerror(errno), errno);
		}
		return 0;
	}

	return 1;
}

static int get_interface_common(const int fd, struct ifreq *const ifr, struct interface *const info)
{
    struct ethtool_cmd  cmd;
    int                 result;

    /* Interface flags. */
    if (ioctl(fd, SIOCGIFFLAGS, ifr) == -1)
        info->flags = 0;
    else
        info->flags = ifr->ifr_flags;

    ifr->ifr_data = (void *)&cmd;
    cmd.cmd = ETHTOOL_GSET; /* "Get settings" */
    if (ioctl(fd, SIOCETHTOOL, ifr) == -1) {
        /* Unknown */
        info->speed = -1L;
        info->duplex = DUPLEX_UNKNOWN;
    } else {
        info->speed = ethtool_cmd_speed(&cmd);
        info->duplex = cmd.duplex;
    }

    do {
        result = close(fd);
    } while (result == -1 && errno == EINTR);
    if (result == -1)
        return errno;

    return 0;
}

int get_interface_by_index(const int index, struct interface *const info)
{
    int             socketfd, result;
    struct ifreq    ifr;

    if (index < 1 || !info)
        return errno = EINVAL;

    socketfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (socketfd == -1)
        return errno;

    ifr.ifr_ifindex = index;
    if (ioctl(socketfd, SIOCGIFNAME, &ifr) == -1) {
        do {
            result = close(socketfd);
        } while (result == -1 && errno == EINTR);
        return errno = ENOENT;
    }

    info->index = index;
    strncpy(info->name, ifr.ifr_name, IF_NAMESIZE);
    info->name[IF_NAMESIZE] = '\0';

    return get_interface_common(socketfd, &ifr, info);
}

int get_interface_by_name(const char *const name, struct interface *const info)
{
    int             socketfd, result;
    struct ifreq    ifr;

    if (!name || !*name || !info)
        return errno = EINVAL;

    socketfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (socketfd == -1)
        return errno;

    strncpy(ifr.ifr_name, name, IF_NAMESIZE);
    if (ioctl(socketfd, SIOCGIFINDEX, &ifr) == -1) {
        do {
            result = close(socketfd);
        } while (result == -1 && errno == EINTR);
        return errno = ENOENT;
    }

    info->index = ifr.ifr_ifindex;
    strncpy(info->name, name, IF_NAMESIZE);
    info->name[IF_NAMESIZE] = '\0';

    return get_interface_common(socketfd, &ifr, info);
}


int getIPaddressForNic(char *nicname, char *ipaddress)
{
	FILE *fd;
	char output[CMD_ARRAY_SIZE];
	char cmd[CMD_ARRAY_SIZE];
	int szret;
	
	memset(output, 0, CMD_ARRAY_SIZE);
	szret = snprintf(cmd, CMD_ARRAY_SIZE, "/sbin/ifconfig %s| /bin/grep 'inet addr:' | /usr/bin/cut -d: -f2| /usr/bin/awk '{ print $1}'", nicname);
	if (szret < 0 || szret >= MAX_PID_FILENAME)
	{
		LOGMSG(LOG_WARNING, "#1 getIPaddressForNic truncated string or error %d",
			szret);
	}

	fd = popen(cmd,"r");
	fgets(output, CMD_ARRAY_SIZE, fd);
	trimwhitespace(ipaddress, 16, output);
	pclose(fd);
	
	return 0;
}

int getNicForIPaddress(char *ipaddress, char *nicname, int len_nicname)
{
	char cmpipaddr[64];
	
	DIR *dir;
	struct dirent *dp;
	char *file_name;
	
	dir = opendir("/sys/class/net");
	while ((dp=readdir(dir)) != NULL) {
	        file_name = dp->d_name;
			if(!strcmp(file_name, ".") || !strcmp(file_name, ".."))
			{
				continue;
			}
			
			if(!getIPaddressForNic(file_name,cmpipaddr))
			{
				if(!strncmp(ipaddress, cmpipaddr,64))	
				{
					strncpy(nicname, file_name, len_nicname);
					closedir(dir);
					return 0;
				}
			}
	}
	closedir(dir);
	
	return 1;
}

int get_product()
{
	FILE *fd;
	char content[256];
	long serialnumber = 0;
	char *res = NULL;

	memset(content, 0, 256);
	fd = fopen("/etc/serialnumber", "r");
	if(fd == NULL)
	{
		return PRODUCT_UNKNOWN;
	}

	res = fgets(content,256, fd);
	fclose(fd);

	if (res == NULL)
	{
		return PRODUCT_UNKNOWN;
	}

	serialnumber = atol(content);
	if (serialnumber >= 10000)
	{
		return PRODUCT_BLACKLABEL;
	}
	else if(serialnumber >= 500)
	{
		return PRODUCT_BLUELABEL;
	}

	return PRODUCT_UNKNOWN;
}

int get_trap_sink(char *destination, int destlen, char *community, int comlen)
{
	FILE *fd;

	destination[0] = '\0';
	community[0] = '\0';

	memset(destination, 0, destlen);
	memset(community, 0, comlen);
	if ( access( "/etc/snmp/snmpd.conf", F_OK ) == -1 )
	{
		return 1;
	}

	fd = popen("/bin/grep ^trap2sink /etc/snmp/snmpd.conf|/usr/bin/awk '{printf(\"\%s\\n\%s\\n\", $2 ,$3)}'", "r");
	if (fd == NULL)
	{
		return 2;
	}

	if (fgets(destination, destlen, fd) == NULL)
	{
		pclose(fd);
		return 3;
	}
	destination[destlen-1] = '\0';

	if (fgets(community, comlen, fd) == NULL)
	{
		pclose(fd);
		return 4;
	}

	destination[destlen-1] = '\0';
	community[comlen-1] = '\0';

	destination[strlen(destination)-1] ='\0';
	community[strlen(community)-1] ='\0';
	pclose(fd);

	return 0;
}

/* adds rtp header to buffer it does not exist
 * if move == 0 it assumes enouch space is available in the beginning of the packet,
   otherwhise it will move the data RTP header length forward
*/
void addRTPHeader(uint8_t *packet, uint16_t size, int move)
{
	static uint16_t rtpcounter = 0;
	int srcIndex;
	int dstIndex;
	
	if(move)
	{
		srcIndex = size - 1;
		dstIndex = size + RTP_MIN_HEADER_SIZE - 1;
		for ( ; srcIndex >= 0; dstIndex--, srcIndex--)
		{
			packet[dstIndex]=packet[srcIndex];
		}

	}
	
	memset(packet, 0, RTP_MIN_HEADER_SIZE);
	packet[0] = 128;
	packet[1] = 33; // payload type = mpeg-ts
	packet[2] = (rtpcounter & 0xff00) >> 8; 
	packet[3] = (rtpcounter & 0x00ff);
	rtpcounter++;
}

int getLinuxDist()
{
	FILE *fd;
	char buf[4096], *str = NULL;
	char *str2 = NULL;
	char distribid[4096];
	char *str_ptr=NULL, *str2_ptr=NULL;
	int debversion= 0;
	float ubuntuversion_f = 0;
	int ubuntuversion = 0;
	int ret;
		
	memset(buf, 0, sizeof(char)*4096);
	//memset(str, 0, sizeof(char)*4096);
	//memset(str2, 0, sizeof(char)*4096);
	memset(distribid, 0, sizeof(char)*4096);
	
	if(access("/etc/lsb-release", F_OK) != -1)
	{
		fd = fopen("/etc/lsb-release", "r");
		if(fd == NULL)
		{
			syslog(LOG_WARNING, "Could not open /etc/lsb-release.");
			return LINUX_DIST_UNKNOWN;
		}
		
		ret = fread(buf, sizeof(char), 4096, fd);
		buf[4095] = 0;
		fclose(fd);
		
		str = strtok_r(buf, "\n", &str_ptr);
		while( str != NULL)
		{
			if(!strncmp("DISTRIB_ID", str, 10))
			{
				str2 = strtok_r(str, "=", &str2_ptr);
				if(str2 == NULL)
				{
					syslog(LOG_WARNING, "Error running multiple strtok_r at %d", __LINE__);
					str = strtok_r(NULL, "\n", &str_ptr);
					continue;
				}
				
				str2 = strtok_r(NULL, "=", &str2_ptr);
				if(str2 == NULL)
				{
					syslog(LOG_WARNING, "Could not parse distrib id.");
					str = strtok_r(NULL, "\n", &str_ptr);
					continue;
				}
				
				snprintf(distribid, 4096, "%s", str2);
			}
			else if(!strncmp("DISTRIB_RELEASE", str, 15))
			{
				str2 = strtok_r(str, "=", &str2_ptr);
				if(str2 == NULL)
				{
					syslog(LOG_WARNING, "Error running multiple strtok_r at %d", __LINE__);
					str = strtok_r(NULL, "\n", &str_ptr);
					continue;
				}
				
				str2 = strtok_r(NULL, "=", &str2_ptr);
				if(str2 == NULL)
				{
					syslog(LOG_WARNING, "Could not parse Ubuntu version.");
					str = strtok_r(NULL, "\n", &str_ptr);
					continue;
				}
				ubuntuversion_f =  atof(str2);
				ubuntuversion = ubuntuversion_f;
			}
	
			str = strtok_r(NULL, "\n", &str_ptr);
		}
		
		if(strncmp(distribid, "Ubuntu", 6))
		{
			switch(ubuntuversion)
			{
				case 14: return LINUX_DIST_UBUNTU_TRUSTY;
				default: syslog(LOG_WARNING, "Unhandled ubuntu version.");
			}
		}
		
		
	}
	else if(access("/etc/debian_version", F_OK) != -1)
	{
		fd = fopen("/etc/debian_version", "r");
		if(fd == NULL)
		{
			syslog(LOG_WARNING, "Could not open /etc/debian_version.");
			return LINUX_DIST_UNKNOWN;
		}
		
		ret = fread(buf,sizeof(char), 4096, fd);
		fclose(fd);

		buf[4095] = 0;
		if(ret == 0)
		{
			syslog(LOG_WARNING, "Unable to read /etc/debian_version.");
			return LINUX_DIST_UNKNOWN;
		}
		
		str = strtok(buf, ".");
		debversion = atoi(str);
		switch(debversion)
		{
			case 7: return LINUX_DIST_DEBIAN_WHEEZY;
			case 8: return LINUX_DIST_DEBIAN_JESSIE;
			case 9: return LINUX_DIST_DEBIAN_STRETCH;
			default: syslog(LOG_WARNING, "Unhandled debian version.");
			
		}
	}
	return LINUX_DIST_UNKNOWN;
}


int saveFile(char *filename, char *data, uint64_t len)
{
	FILE *fd = NULL;
	int ret;
	
	fd = fopen(filename, "w");
	if(fd == NULL)
	{
		return 1;
	}
	
	ret = fwrite(data, sizeof(char), len, fd);
	fclose(fd);

	if(ret != len)
	{
		printf("ret: %d:%"PRIu64"\n", ret,len);
		return 2;
	}
	
	return 0;
}