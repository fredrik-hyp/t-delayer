#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include "../../prot_rudp.h"
#include "../../print_struct.h"

#define STAT_LENGTH 2100

int run = 1;
static void term_func(int sig, siginfo_t *siginfo, void *context)
{
	fprintf(stderr, "%d Sending PID: %ld, UID: %ld\n",
			sig,
			(long)siginfo->si_pid,
			(long)siginfo->si_uid);
	run = 0;
}

void usage(char *process)
{
	fprintf(stderr, "Usage: %s <-m multicast_address> <-p #port> [-t #ttl] [-u] [-d] [-a] [-b] [-c] [-e] [-f] [-g]\n",
			process);
	fprintf(stderr, "-u use_pselect\n");
	fprintf(stderr, "-d print details\n");
	fprintf(stderr, "-a print RTP header\n");
	fprintf(stderr, "-b print MPEG header\n");
	fprintf(stderr, "-c print MPEG Adaption header\n");
	fprintf(stderr, "-e print MPEG IF Adaption if exist\n");
	fprintf(stderr, "-f print MPEG PCR if exist\n");
	fprintf(stderr, "-g print PCR\n");
}

int main(int argc, char **argv)
{
	if (argc < 3)
	{
		usage(argv[0]);
		return 1;
	}

	char multicast_address[16];
	int multicast_port;
	int multicast_ttl = 1;
	bool port = false;
	bool multicast = false;
	bool use_pselect = false;
	bool print_details = false;

	bool print_RTP_header = false;
	bool print_MPEG_header = false;
	bool print_MPEG_Adaption_header = false;
	bool print_MPEG_IF_Adaption_if_exist = false;
	bool print_MPEG_PCR_if_exist = false;
	bool print_PCR = false;

	int opt;

	// ------- PARSE ARGUMENTS -------
	while ((opt = getopt(argc, argv, "abcefgudt:p:m:")) != -1)
	{
		switch(opt)
		{
		case 'a':
			fprintf(stderr, "Arg: print RTP header\n");
			print_RTP_header = true;
			break;
		case 'b':
			fprintf(stderr, "Arg: print MPEG header\n");
			print_MPEG_header = true;
			break;
		case 'c':
			fprintf(stderr, "Arg: print MPEG adaption header\n");
			print_MPEG_Adaption_header = true;
			break;
		case 'e':
			fprintf(stderr, "Arg: print MPEG IF adaption if exist\n");
			print_MPEG_IF_Adaption_if_exist = true;
			break;
		case 'f':
			fprintf(stderr, "Arg: print MPEG PCR if exist\n");
			print_MPEG_PCR_if_exist = true;
			break;
		case 'g':
			fprintf(stderr, "Arg: print PCR\n");
			print_PCR = true;
			break;

		case 'u':
			fprintf(stderr, "Arg: Use pselect\n");
			use_pselect = true;
			break;

		case 'd':
			fprintf(stderr, "Arg: Print details\n");
			print_details = true;
			break;

		case 't':
			fprintf(stderr, "Arg: TTL: %s\n", optarg);
			multicast_ttl = (int)strtol(optarg, (char **)NULL, 10);
			break;

		case 'p':
			port = true;
			fprintf(stderr, "Arg: Port %s\n", optarg);
			multicast_port = (int)strtol(optarg, (char **)NULL, 10);
			break;

		case 'm':
			multicast = true;
			fprintf(stderr, "Arg: Multicast address %s\n", optarg);
			strncpy(multicast_address, optarg, 16);
			break;

		default:
			fprintf(stderr, "Arg: Default - not expecting %s\n", optarg);
			break;
		}
	}

	// ------- check required arguments -------
	if (multicast == false || port == false)
	{
		usage(argv[0]);
		return 1;
	}

	int fd_mc_input;
	struct sockaddr_in sockaddr_mc_input;
	socklen_t addrlen;
	struct ip_mreq mreq;
	ssize_t recv_bytes=0;
	uint8_t packet_buffer[PKTSIZE+RTP_MIN_HEADER_SIZE];
	int optval, retval;
	uint16_t old_sequence=0;
	uint64_t pre_lock, last_timestamp = 0, dT;
	struct timeval tv;

	int stat_buf[STAT_LENGTH];
	memset(stat_buf, 0, STAT_LENGTH*sizeof(int));

	// ------- setup Signal to catch ctrl+c -------
	struct sigaction sint;
	memset(&sint, 0, sizeof(struct sigaction));
	sint.sa_sigaction = &term_func;
	sint.sa_flags = 0;

	retval = sigaction(SIGINT, &sint, NULL);
	if (retval == -1)
	{
		fprintf(stderr, "Error to setup SIGINT signal handler (%s)",
				strerror(errno));
		return 1;
	}

	// ------- SETUP socket -------
	if ((fd_mc_input = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		fprintf(stderr, "multicast socket failure");
		return 1;
	}

	memset(&sockaddr_mc_input, 0, sizeof(struct sockaddr_in));
	sockaddr_mc_input.sin_family = AF_INET;
	sockaddr_mc_input.sin_addr.s_addr = inet_addr(multicast_address);
	sockaddr_mc_input.sin_port = htons(multicast_port);

	// ------- REUSE address -------
	optval = 1;
	if (setsockopt(fd_mc_input, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
	{
		fprintf(stderr, "setsockopt failed errno: %d %s\n",
				errno,
				strerror(errno));
		close(fd_mc_input);
		return 2;
	}

	// ------- BIND -------
	int bindret = bind(fd_mc_input, (struct sockaddr *) &sockaddr_mc_input, sizeof(struct sockaddr_in));
	if (bindret == -1)
	{
		fprintf(stderr, "bind to input address failed: %s\n",
				strerror(errno));
		close(fd_mc_input);
		return 3;
	}

	// ------- Multicast membership -------
	mreq.imr_multiaddr.s_addr = inet_addr(multicast_address);
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);

	if (setsockopt(fd_mc_input, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) == -1)
	{
		fprintf(stderr, "Probably unicast - setsockopt failed errno: %d %s\n",
				errno,
				strerror(errno));
	}

	addrlen = sizeof(struct sockaddr_in);
	while (run)
	{
		// ------- pselect -------
		int res_select = 0;
		while (run && use_pselect && res_select == 0)
		{
			fd_set fdset;
			sigset_t pselect_set;
			struct timespec tp_select;

			// Start blocking socket fix
			FD_ZERO(&fdset);
			FD_SET(fd_mc_input, &fdset);
			tp_select.tv_sec=1;
			tp_select.tv_nsec=0;
			sigemptyset(&pselect_set);
			sigaddset(&pselect_set, SIGTERM);
			sigaddset(&pselect_set, SIGINT);

			res_select = pselect(fd_mc_input+1,&fdset, NULL, NULL, &tp_select, &pselect_set);
		// if res_select == 0 timeout occurred
		}

		// ------- Recv -------
		recv_bytes = recvfrom(fd_mc_input, packet_buffer,
							  PKTSIZE + RTP_MIN_HEADER_SIZE, 0,
							  (struct sockaddr *) &sockaddr_mc_input, &addrlen);

		// ------- handle dT statistics -------
		gettimeofday(&tv, NULL);

		pre_lock = tv.tv_sec*1000000ULL + tv.tv_usec;
		dT = pre_lock - last_timestamp;
		if (dT > STAT_LENGTH)
		{
			fprintf(stderr, "mc_receiver dT %"PRIu64"\n", dT);
		}
		else
		{
			stat_buf[dT]++;
		}
		last_timestamp = pre_lock;

		// ------- CHECK RTP SEQUENCE GAP -------
		struct raw_rtp_packet *packet = (struct raw_rtp_packet *)(packet_buffer);
		uint16_t new_sequence = ntohs(packet->sequence_number);
		if (old_sequence+1 != new_sequence && new_sequence != 0)
		{
			fprintf(stderr, "old_sequence %u new_sequence %u\n",
					old_sequence,
					new_sequence);
		}
		old_sequence = new_sequence;


		// ------- Print different header -------
		if (print_RTP_header)
			print_rtp_header(packet_buffer);

		if (print_MPEG_header)
			print_all_mpeg_header(packet_buffer);

		if (print_MPEG_Adaption_header)
			print_all_mpeg_header_and_adaption_field(packet_buffer);

		if (print_MPEG_IF_Adaption_if_exist)
			print_if_adaption_field_exist_mpeg_header_and_adaption_field(packet_buffer);

		if (print_MPEG_PCR_if_exist)
			print_if_pcr_exist_mpeg_header_adaption_field_and_pcr(packet_buffer);

		if (print_PCR)
			print_pcr_if_exist(packet_buffer);

	} // end receiver


	// ------- Print summary -------
	int sum = 0, count = 1;
	if (print_details)
	{
		for (int i = 0; i < STAT_LENGTH; i++)
		{
			if (stat_buf[i] > 10)
				fprintf(stderr, "%d: %d\n", i, stat_buf[i]);
		}
	}
	else
	{
		for (int i = 0; i < STAT_LENGTH; i++)
		{
			sum += stat_buf[i];
			if (i > 99*count)
			{
				fprintf(stderr, "%d - %d: %d\n", 100*(count-1), i, sum);
				count++;
				sum = 0;
			}
		}
	}

	close(fd_mc_input);
	return 0;
}
