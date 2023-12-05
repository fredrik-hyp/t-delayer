#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include "buffer.h"

#define BUFSIZE 256

void usage(const char *name)
{
	fprintf(stderr, "Usage: %s <-I filename.bin> [-a print index|framenumber] [-O outfile.ts] [-F framenumber]\n", name);
}

int main(int argc, char **argv)
{
	FILE *p_infile=NULL, *p_outfile=NULL;
	size_t ret;
	int noOfRead = 0;
	bufferelement *current_bufelement;
	int noOfWrapped = 0;
	int first=0;
	int lastCounter = 0;
	int discontinuity = 0;
	bool infile_rudp=false;
	char infile[BUFSIZE];
	bool outfile_ts=false;
	char outfile[BUFSIZE];
	bool framenumber_rudp=false;
	int framenumber=-1;

	bool all_index_frame=false;
	uint64_t last_time_stamp=0, first_time_stamp=0;

	bool show_index=false;
	uint16_t index_no=0;


	// === Parse argument list ===
	if (argc < 2)
	{
		// Not right number of arguments print usage
		usage(argv[0]);
		return 1;
	}
	int opt;
	while ((opt = getopt(argc, argv, "aI:i:O:F:")) != -1)
	{
		switch(opt)
		{
		case 'I':
			fprintf(stderr, "input file %s\n", optarg);
			infile_rudp = true;
			strncpy(infile, optarg, BUFSIZE);
			break;
		case 'i':
			fprintf(stderr, "Index %s\n", optarg);
			show_index = true;
			index_no = atoi(optarg);
			break;
		case 'a':
			fprintf(stderr, "All index | framenumber\n");
			all_index_frame = true;
			break;
		case 'O':
			fprintf(stderr, "output file %s\n", optarg);
			outfile_ts= true;
			strncpy(outfile, optarg, BUFSIZE);
			break;
		case 'F':
			fprintf(stderr, "framenumber %s\n", optarg);
			framenumber_rudp = true;
			framenumber = atoi(optarg);
			break;
		default:
			fprintf(stderr, "DEFAULT!!\n");
			break;
		}
	}
	if (infile_rudp == false)
	{
		usage(argv[0]);
		return 1;
	}

	// === Create some memory ===
	current_bufelement = calloc(1,sizeof(bufferelement));

	// === Open file ===
	fprintf(stderr, "open file: %s\n", infile);
	p_infile = fopen(infile, "r");
	if (p_infile == NULL)
	{
		fprintf(stderr, "fopen(%s, \"r\") failed: %d %s\n",
			   infile, errno, strerror(errno));
		return 1;
	}

	if (outfile_ts)
	{
		p_outfile = fopen(outfile, "w");
		if (p_outfile == NULL)
		{
			fprintf(stderr, "fopen(%s, \"r\") failed: %d %s\n",
				   outfile, errno, strerror(errno));
			return 1;
		}
	}


	// === Start reading ===
	while (feof(p_infile) == 0)
	{
		ret = fread(current_bufelement->packet_buffer, PACKET_BUFFER_SIZE, 1, p_infile);

		if (ret == 0)
		{
			if (feof(p_infile) != 0)
			{
				break;
			}
			else if (ferror(p_infile) != 0)
			{
				fprintf(stderr, "ferror(p_infile) %d %s",
						errno, strerror(errno));
				clearerr(p_infile);
				break;
			}
			break;
		}
		noOfRead++;

		parseHeader(current_bufelement->packet_buffer, &(current_bufelement->header));

		if (all_index_frame)
		{
			fprintf(stderr, "[%d] hdr.counter=%u hdr.sendtime %"PRIu64" dT %"PRIu64"\n",
				noOfRead, current_bufelement->header.counter,
				current_bufelement->header.sendtime,
				current_bufelement->header.sendtime - last_time_stamp);
			last_time_stamp = current_bufelement->header.sendtime;
		}
		if (outfile_ts)
		{
			ret = fwrite((current_bufelement->packet_buffer+RUDP_HEADER_SIZE), PKTSIZE + RTP_MIN_HEADER_SIZE, 1, p_outfile);
		}

		// ==== First noOfRead == 1 ====
		if (noOfRead > 1)
		{
			if (lastCounter == MAX_PACKET_COUNTER && current_bufelement->header.counter == 1)
			{
				// Do nothing - its ok
				noOfWrapped++;
			}
			else if (lastCounter+1 != current_bufelement->header.counter)
			{
				fprintf(stderr, "[%d] %u != [%d] %u\n", noOfRead-1, lastCounter, noOfRead, current_bufelement->header.counter);
				discontinuity++;
			}
		}
		else
		{
			first = current_bufelement->header.counter;
			first_time_stamp = current_bufelement->header.sendtime;
		}
		lastCounter = current_bufelement->header.counter;

		if (show_index && noOfRead == index_no)
		{
			fprintf(stderr, "\n[%d] hdr.counter=%u hdr.sendtime %"PRIu64"\n\n",
				noOfRead, current_bufelement->header.counter,
				current_bufelement->header.sendtime);
		}
		if (framenumber == lastCounter)
		{
			fprintf(stderr, "header.typeofpacket %d\n", current_bufelement->header.typeofpacket);
			fprintf(stderr, "header.reserved %d\n", current_bufelement->header.reserved);
			fprintf(stderr, "header.sessionid %u\n", current_bufelement->header.sessionid);
			fprintf(stderr, "header.payloadlength %d\n", current_bufelement->header.payloadlength);
			fprintf(stderr, "header.channelnumber %d\n", current_bufelement->header.channelnumber);
			fprintf(stderr, "header.counter %d\n", current_bufelement->header.counter);
			fprintf(stderr, "header.sendtime %lu\n", current_bufelement->header.sendtime);

			fprintf(stderr, "rudp_sendtime %"PRIu64"\n", current_bufelement->rudp_sendtime);
			fprintf(stderr, "rudp_sendtime %d\n", current_bufelement->rtp);

		}
	}

	fprintf(stderr, "dT in file: %"PRIu64"\n", current_bufelement->header.sendtime-first_time_stamp);
	fprintf(stderr, "Counters in file [%d - %d] ", first, lastCounter);
	if (noOfWrapped > 0)
		fprintf(stderr, "wrapped %d", noOfWrapped);
	fprintf(stderr, "\n");

	fprintf(stderr, "Number of RUDP packets: %d\n",noOfRead);
	fprintf(stderr, "rudp counter discontinuity: %d\n", discontinuity);

	// === Free memory ===
	free(current_bufelement);

	fprintf(stderr, "close file\n");
	fclose(p_infile);
	if (outfile_ts)
	{
		fclose(p_outfile);
	}
	return 0;
}
