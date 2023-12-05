#include "common-db.h"
#include "dbdelayers.h"
#include "dbfile.h"
#include <stdio.h>
#include <ncurses.h>
#include <unistd.h>


bool run = true;

static void term_func(int sig, siginfo_t *siginfo, void *context)
{
        run = false;
printf("term_funnc\n");
}

const char *micro_timestamp_to_str(uint64_t time, char *buf, int size)
{
	if (time == 0)
	{
		buf[0] = '\0';
		return buf;
	}
	time_t epoch = time/1000000;
	struct tm *epoch_tm;

	epoch_tm = localtime(&epoch);
	strftime(buf, size, "%y-%m-%d %H:%M:%S", epoch_tm);
	return buf;
}

const char *timestamp_to_str(uint64_t time, char *buf, int size)
{
	if (time == 0)
	{
		buf[0] = '\0';
		return buf;
	}
	time_t epoch = time;
	struct tm *epoch_tm;

	epoch_tm = localtime(&epoch);
	strftime(buf, size, "%y-%m-%d %H:%M:%S", epoch_tm);
	return buf;
}

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		printf("Usage: %s <streaminterface_id>\n", argv[0]);
		return 1;
	}

	// === Setup signal handler ===
	struct sigaction sterm;
	memset(&sterm, 0, sizeof(struct sigaction));
	sterm.sa_sigaction = &term_func;
	sterm.sa_flags = 0;// SA_SIGINFO;

	int retval = sigaction(SIGINT, &sterm, NULL);
	if (retval == -1)
	{
		printf("Failed to setup signal handler\n");
		return 2;
	}

	uint64_t streaminterface_id;
	streaminterface_id = atoi(argv[1]);
	
	PGconn *dbconn;
	// === Connect to database and fetch this interface config ===
	dbconn = DBConnect();
	if (dbconn == NULL)
	{
		printf("Failed to connect to database\n");
		exit(0);
	}

	s_interfacelog streaminterfacelog[10];
	s_delayerstats delayerstats[MAX_INTERFACE_PATH];
	s_interfacelivestatistic livestatistic[DB_MAX_NR_OF_STREAM_PATHS];
	int db_ret;
	uint16_t fetchedrow=0, old_fetchedrow=0;

	char buf1[80], buf2[80], buf3[80], buf4[80], buf5[80];
	
	initscr();
	noecho();
	curs_set(FALSE);
	nodelay(stdscr, TRUE);

	int row_pos;
	int max_y, max_x, old_max_y=0, old_max_x=0, max_count;
	getmaxyx(stdscr, max_y, max_x);

	uint64_t bitratein, bitrateout, bit_count;
	bool scale_100mbps = false, current_scale_100mbps;

	db_ret = dbfetchstreaminterfacelivestatisticmpbystreaminterfaceid(&dbconn,
			livestatistic,
			streaminterface_id,
			&fetchedrow);

	mvprintw(0, 0, "Streaminterface_id: %d mode: %s source: %s multipath: %d mploadbalance: %d ttl: %d",
		streaminterface_id, livestatistic[0].mode, livestatistic[0].source,
		livestatistic[0].multipath, livestatistic[0].mploadbalance, livestatistic[0].ttl);

	for (int i = 0, c=0; i < fetchedrow; i++)
	{
		// Check initial scaling
		if (livestatistic[i].bitratein > 10000000 || livestatistic[i].bitrateout > 10000000)
		{
			scale_100mbps = true;
		}

		if (livestatistic[i].pathnumber == 100)
		{
			continue;
		}
		mvprintw(1, 35*c, "destination[%d]: %s",
			livestatistic[i].pathnumber, livestatistic[i].destination);
		c++;
	}
	current_scale_100mbps = scale_100mbps;

	bool delayer = (strncmp("delayer", livestatistic[0].mode, 7) == 0);
	// filetransmitter or filereceiver
	bool rudp_file = (strncmp("file", livestatistic[0].mode, 4) == 0);
	while (run)
	{
		// Fix scale if the terminal windows size are changed
		getmaxyx(stdscr, max_y, max_x);
		if (max_y != old_max_y || max_x != old_max_x || scale_100mbps != current_scale_100mbps)
		{
			old_max_y = max_y;
			old_max_x = max_x;
			current_scale_100mbps = scale_100mbps;

			// Clear row
			move(4, 0);
			clrtoeol();
			move(5, 0);
			clrtoeol();

			// print new multiplier
			if (current_scale_100mbps)
			{
				mvprintw(4, 0, "100x");
				mvprintw(5, 0, "10x");
			}
			else
			{
				mvprintw(4, 0, "10x");
				mvprintw(5, 0, "1x");
			}

			// print scale
			for (max_count = 0; 20*max_count < max_x; max_count++)
			{
				mvprintw(4, 7+20*max_count, "%d", max_count);
			}
			for (max_count = 0; 2*max_count < max_x; max_count++)
			{
				mvprintw(5, 7+2*max_count, "%d ", max_count%10);
			}
		}

		db_ret = dbfetchstreaminterfacelivestatisticmpbystreaminterfaceid(&dbconn,
                                livestatistic,
				streaminterface_id,
				&fetchedrow);
		if (db_ret == 0)
		{
			scale_100mbps = false;
			old_fetchedrow = fetchedrow;

			for (int i = 0; i < fetchedrow; i++)
			{
				row_pos = 2*i;
				if (current_scale_100mbps)
				{
					// Divide with 5000 000 to get the halfs.
					bitratein = livestatistic[i].bitratein/5000000;
					bitrateout = livestatistic[i].bitrateout/5000000;
				}
				else
				{
					// Divide with 500 000 to get the halfs.
					bitratein = livestatistic[i].bitratein/500000;
					bitrateout = livestatistic[i].bitrateout/500000;
				}

				if (livestatistic[i].bitratein > 10000000 || livestatistic[i].bitrateout > 10000000)
				{
					scale_100mbps = true;
				}

				// clear bar
				move(6+row_pos, 0);
				clrtoeol();
				move(7+row_pos,0);
				clrtoeol();

				// repaint the bar
				mvprintw(6+row_pos, 0, "I: %"PRIu64, livestatistic[i].pathnumber);
				mvprintw(7+row_pos, 0, "O:");
				for (bit_count = 0; bit_count < bitratein; bit_count++)
				{
					mvprintw(6+row_pos, 7 + bit_count, "*");
				}
				for (bit_count = 0; bit_count < bitrateout; bit_count++)
				{
					mvprintw(7+row_pos, 7 + bit_count, "*");
				}
			}
			move(6+2*fetchedrow, 0);
			clrtoeol();
			move(7+2*fetchedrow, 0);
			clrtoeol();

			mvprintw(7+2*fetchedrow, 0, "udpdrops: %"PRIu64" acc_rtp_drops: %"PRIu64,
				livestatistic[0].udpdrops, livestatistic[0].acc_rtp_drops);

			move(8+2*fetchedrow, 0);
			clrtoeol();
		}
		else
		{
			// timestamp
			//mvprintw(20, 0, "fetch streaminterface livestatistic failed %d", db_ret);
			for (int i = 0; i < old_fetchedrow; i++)
			{
				row_pos = 2*i;
				move(6+row_pos, 0);
				clrtoeol();
				move(7+row_pos, 0);
				clrtoeol();
			}
			fetchedrow = old_fetchedrow;

		}

		int fetched_delayerstats_paths = 0;
		if (delayer)
		{
			db_ret = db_fetch_delayerstatsbystreaminterfaceid(&dbconn,
				delayerstats,
				MAX_INTERFACE_PATH,
				&fetched_delayerstats_paths,
				streaminterface_id);
			if (db_ret != 0)
			{
				// timestamp
				mvprintw(21, 0, "fetch delayerstats failed %d", db_ret);
			}
			else
			{

				for (int i = 0; i < fetched_delayerstats_paths; i++)
				{
					timestamp_to_str(delayerstats[i].startplayat_timestamp, buf1, 80);
					timestamp_to_str(delayerstats[i].playto_timestamp, buf2, 80);

					timestamp_to_str(delayerstats[i].startpos_timestamp, buf3, 80);
					timestamp_to_str(delayerstats[i].currpos_timestamp, buf4, 80);
					timestamp_to_str(delayerstats[i].lastpos_timestamp, buf5, 80);

					row_pos = 2*fetchedrow+2*i;
					// clear bar
					move(9+row_pos, 0);
					clrtoeol();
					move(10+row_pos, 0);
					clrtoeol();

					mvprintw(9+row_pos, 0, "path[%d] state: %s startplayat: %s playto: %s",
						delayerstats[i].pathnumber,
						delayerstats[i].state, buf1, buf2);
					mvprintw(10+row_pos, 0, "startpos: %s currpos: %s lastpos: %s",
						buf3, buf4, buf5);
				}
			}
		} else if (rudp_file) {
			s_file file;
			int fetched_files;
			db_ret = db_fetch_file_intransit(&dbconn,
				&file,
				1,
				&fetched_files,
				streaminterface_id);
			if (db_ret != 0)
			{
				// timestamp
				mvprintw(2, 0, "fetch file_intransit failed %d", db_ret);
			}
			else if (file.size > 0)
			{
				mvprintw(2, 0, "%s %"PRIu64" / %"PRIu64" (MB)",
					file.filename,
					file.transferredbytes/1048576,
					file.size/1048576);
			}
			else
			{
				move(2, 0);
				clrtoeol();
				mvprintw(2, 0, "No file transferring");
			}
		}

		row_pos = 12+2*fetchedrow + 2*(fetched_delayerstats_paths-1);
		uint16_t fetched_interfacelogs_rows;

		db_ret = dbfetch_interfacelog(&dbconn,
				streaminterface_id,
				streaminterfacelog, 10,
				&fetched_interfacelogs_rows);
		for (int i = 0; i < fetched_interfacelogs_rows && i < 10; i++)
		{
			move(row_pos+i, 0);
			clrtoeol();

			mvprintw(row_pos+i, 0, "%s: %s %u",
				streaminterfacelog[i].timestamp,
				streaminterfacelog[i].text, fetchedrow);
		}

		refresh();
		sleep(1);
		if (getch() == 'q')
		{
			run = false;
		}
	}
	endwin();
	return 0;
}
