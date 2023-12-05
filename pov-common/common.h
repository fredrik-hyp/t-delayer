/*
 * common settings for transmitter and receiver
 */
#ifndef __COMMON_H__
#define __COMMON_H__
#define __STDC_FORMAT_MACROS
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <syslog.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdarg.h>
#include <dirent.h>
// _XOPEN_SOURCE must just affect sys includes
#ifndef SKIP_XOPEN_SOURCE
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE
#endif
#endif

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <ctype.h>


#include <sys/ioctl.h>
#include <netinet/ip.h>
#include <net/if.h>
#include <linux/ethtool.h>
#include <linux/sockios.h>

#include "prot_rudp.h"

#ifdef NODAEMON_DEBUG
	#define LOGMSG(level, fmt, ...) { \
		fprintf(stderr, fmt, __VA_ARGS__); \
		fprintf(stderr, "\n"); \
		syslog(level, fmt, __VA_ARGS__); \
		}
	#define OPTLOGMSG(opt, level, fmt, ...) { \
		fprintf(stderr, fmt, __VA_ARGS__); \
		fprintf(stderr, "\n"); \
		if (opt) { syslog(level, fmt, __VA_ARGS__); } \
		}
#else
	#define LOGMSG(level, fmt, ...) { \
		syslog(level, fmt, __VA_ARGS__); \
		}
	#define OPTLOGMSG(opt, level, fmt, ...) { \
		if (opt) { syslog(level, fmt, __VA_ARGS__); } \
		}
#endif

#define LOG_PTHREAD_ERROR(pthread_ret, name, line) { \
	if (pthread_ret != 0) { \
		LOGMSG(LOG_WARNING, "%s: pthread_ret@%d: %s (%d)", name, line, strerror(pthread_ret), pthread_ret); \
	} }

#define LOG_PTHREAD_ERROR_BREAK(pthread_ret, name, line) { \
	if (pthread_ret != 0) { \
		LOGMSG(LOG_WARNING, "%s: pthread_ret@%d: %s (%d)", name, line, strerror(pthread_ret), pthread_ret); \
		break; \
	} }

#define LOG_PTHREAD_ERROR_CONTINUE(pthread_ret, name, line) { \
	if (pthread_ret != 0) { \
		LOGMSG(LOG_WARNING, "%s: pthread_ret@%d: %s (%d)", name, line, strerror(pthread_ret), pthread_ret); \
		continue; \
	} }




#define SSL_SLEEP_CONNECT_RETRY 5000
#define EXIT_SUCCESS 0

#define SOCK_POVMANAGER "/tmp/.sock_povmanager"

#define RTT_ARRAY_SIZE 50
#define RTT_DEFAULT_MS 1000000
#define RTT_SAMPLE_RATE 5
#define RTT_HEADROOM 20000

#define PID_RECEIVER_FILENAME_PATTERN		"/var/run/rudp_receiver_%s.pid"
#define PID_TRANSMITTER_FILENAME_PATTERN	"/var/run/rudp_transmitter_%s.pid"
#define PID_FILE_RECEIVER_FILENAME_PATTERN	"/var/run/rudp_file_receiver_%s.pid"
#define PID_FILE_TRANSMITTER_FILENAME_PATTERN	"/var/run/rudp_file_transmitter_%s.pid"
#define PID_FORWARDER_FILENAME_PATTERN		"/var/run/udp_forwarder_%s.pid"
#define PID_DELAYER_FILENAME_PATTERN		"/var/run/delayerd/udp_delayer_%s.pid"
#define PID_RECORDER_FILENAME_PATTERN		"/var/run/udp_recorder_%s.pid"
#define PID_PLAYER_FILENAME_PATTERN		"/var/run/udp_player_%s.pid"
#define PID_SRTTRANSMITTER_FILENAME_PATTERN "/var/run/srt_sender_%s.pid"
#define PID_SRTRECEIVER_FILENAME_PATTERN "/var/run/srt_receiver_%s.pid"
#define MAX_PID_FILENAME			128


// MAX_INTERFACE_PATH = 1 maininterface + up to DB_MAX_SOURCE-1 redundant paths
#define MAX_INTERFACE_PATH			DB_MAX_SOURCE

#define PRODUCT_UNKNOWN 1
#define PRODUCT_BLUELABEL 2
#define PRODUCT_BLACKLABEL 3
	
#define LINUX_DIST_UNKNOWN 0
#define LINUX_DIST_DEBIAN_WHEEZY 1
#define LINUX_DIST_DEBIAN_JESSIE 2
#define LINUX_DIST_DEBIAN_STRETCH 3
#define LINUX_DIST_UBUNTU_TRUSTY 4

#define NO_OF_SDA_PARTITIONS 10

/* license structur for interfaceype */
typedef struct _povlicense{
	uint8_t interfacetypelength;
	char interfacetype[256];
	uint16_t numberofinterfaces;
	time_t validuntil;
	uint16_t extlength;
	char *ext;
} povlicense;


/* licensefile structure */
typedef struct _licensefilecontents{
	char serialnumber[128];
	uint8_t serialnumberlength;
	char macaddresses[256][24];
	uint8_t nrofmacaddresses;
	uint8_t numberofinterfacetypes;
	povlicense **licenses;
}licensefilecontents;


typedef struct __attribute__((__packed__)) _rtpheader
{
	uint8_t version:2;
	uint8_t p:1;
	uint8_t x:1;
	uint8_t cc:4;
	uint8_t m:1;
	uint8_t pt:7;
	uint16_t seqnr:16;
	uint32_t timestamp:32;
	uint32_t ssrc:32;
}rtpheader;

struct interface {
    int     index;
    int     flags;      /* IFF_UP etc. */
    long    speed;      /* Mbps; -1 is unknown */
    int     duplex;     /* DUPLEX_FULL, DUPLEX_HALF, or unknown */
    char    name[IF_NAMESIZE + 1];
};

/*
typedef struct _ackpacket{
	uint8_t counter[3];
}ackpacket;
*/

size_t trimwhitespace(char *out, size_t len, const char *str);
int hostnametoip(char *hostname, char *ip, bool log_errors);
void daemonize();
void parse_argument(int argc, char **argv, char *version, int *run_as_daemon);

int create_pid(const char *pidfilename);
int close_pid_file(int pidfile, const char *pidfilename);
int read_pid_file(const char *pidfilename);

int getInterfacePid(char *progname, char *interfacename);
int removePidFile(char *interfacename);

int getIPaddressForNic(char *nicname, char *ipaddress);
int getNicForIPaddress(char *ipaddress, char *nicname, int len_nicname);

int get_product();
int get_trap_sink(char *destination, int destlen, char *community, int comlen);

void addRTPHeader(uint8_t *packet, uint16_t size, int move);

int getLinuxDist();
int saveFile(char *filename, char *data, uint64_t len);

	


#endif
