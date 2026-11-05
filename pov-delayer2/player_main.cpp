#include <algorithm>
#include <cerrno>
#include <cinttypes>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <map>
#include <set>
#include <string>
#include <sys/time.h>
#include <vector>

extern "C" {
#include "buffer.h"
#include "common.h"
#include "common-db.h"
#include "dbrecplay.h"
#include "dbstreaminterfaces.h"
#include "dbsysteminfo.h"
#include "prot_rudp.h"
}

static volatile sig_atomic_t shutdown_requested = 0;
static volatile sig_atomic_t reload_requested = 0;

static const uint64_t NO_TIME_LIMIT = UINT64_MAX;

enum player_run_state {
	PLAYER_RUN_PAUSE,
	PLAYER_RUN_PLAY
};

struct chunk_info {
	uint32_t id;
	uint64_t start_time;
	uint64_t end_time;
	std::string path;
};

struct playback_position {
	uint32_t file_id;
	long offset;
	uint64_t packet_time;
};

struct playback_state {
	player_run_state state;
	bool have_position;
	playback_position pos;
	uint64_t record_base_time;
	uint64_t wall_base_time;
	uint64_t startplayat;
	uint64_t startpos;
	uint64_t currpos;
	uint64_t lastpos;
	uint64_t playto;
};

struct open_chunk {
	FILE *fp;
	uint32_t file_id;
	std::string path;
};

static void term_signal_handler(int, siginfo_t *, void *)
{
	shutdown_requested = 1;
}

static void reload_signal_handler(int, siginfo_t *, void *)
{
	reload_requested = 1;
}

static uint64_t now_usec()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000000ULL + tv.tv_usec;
}

static void print_usage(const char *progname)
{
	fprintf(stderr,
		"USAGE: %s <streaminterface> [recording-directory] [nodaemon]\n"
		"       %s -v\n",
		progname, progname);
}

static int parse_arguments(int argc, char **argv, std::string *interfacename,
	std::string *recording_dir, int *run_as_daemon)
{
	if (argc == 2 && strcmp(argv[1], "-v") == 0)
	{
		fprintf(stderr, "Version: %s\n", VERSION);
		return 1;
	}

	if (argc < 2 || argc > 4)
	{
		print_usage(argv[0]);
		return -1;
	}

	*interfacename = argv[1];
	*run_as_daemon = 1;

	for (int i = 2; i < argc; i++)
	{
		if (strcmp(argv[i], "nodaemon") == 0)
		{
			*run_as_daemon = 0;
		}
		else if (recording_dir->empty())
		{
			*recording_dir = argv[i];
		}
		else
		{
			print_usage(argv[0]);
			return -1;
		}
	}

	return 0;
}

static int setup_signal_handlers()
{
	struct sigaction sterm;
	memset(&sterm, 0, sizeof(sterm));
	sterm.sa_sigaction = &term_signal_handler;
	sterm.sa_flags = SA_SIGINFO;
	if (sigaction(SIGTERM, &sterm, NULL) == -1)
	{
		return 1;
	}
	if (sigaction(SIGINT, &sterm, NULL) == -1)
	{
		return 2;
	}

	struct sigaction susr;
	memset(&susr, 0, sizeof(susr));
	susr.sa_sigaction = &reload_signal_handler;
	susr.sa_flags = SA_SIGINFO;
	if (sigaction(SIGUSR1, &susr, NULL) == -1)
	{
		return 3;
	}

	return 0;
}

static bool has_suffix(const std::string &value, const char *suffix)
{
	const size_t suffix_len = strlen(suffix);
	if (value.size() < suffix_len)
	{
		return false;
	}
	return value.compare(value.size() - suffix_len, suffix_len, suffix) == 0;
}

static bool parse_chunk_filename(const std::string &filename,
	const std::string &prefix, uint32_t *id)
{
	const std::string marker = prefix + "_chunk_";
	if (filename.compare(0, marker.size(), marker) != 0 ||
		!has_suffix(filename, ".bin"))
	{
		return false;
	}

	const size_t start = marker.size();
	const size_t end = filename.size() - 4;
	if (start >= end)
	{
		return false;
	}

	char *tail = NULL;
	errno = 0;
	unsigned long parsed = strtoul(filename.substr(start, end - start).c_str(),
		&tail, 10);
	if (errno != 0 || tail == NULL || *tail != '\0' || parsed > UINT32_MAX)
	{
		return false;
	}

	*id = (uint32_t)parsed;
	return true;
}

static std::string chunk_path(const std::string &directory,
	const std::string &prefix, uint32_t id)
{
	char filename[VIDEO_PATH_LEN + DB_MAX_NAME_LENGTH + 64];
	snprintf(filename, sizeof(filename), "%s/%s_chunk_%u.bin",
		directory.c_str(), prefix.c_str(), id);
	return std::string(filename);
}

static int discover_chunk_prefix(const std::string &directory,
	std::string *prefix)
{
	DIR *dir = opendir(directory.c_str());
	if (dir == NULL)
	{
		return 1;
	}

	std::set<std::string> prefixes;
	struct dirent *dent;
	while ((dent = readdir(dir)) != NULL)
	{
		std::string name = dent->d_name;
		if (!has_suffix(name, ".bin"))
		{
			continue;
		}
		size_t pos = name.find("_chunk_");
		if (pos == std::string::npos || pos == 0)
		{
			continue;
		}
		prefixes.insert(name.substr(0, pos));
	}
	closedir(dir);

	if (prefixes.size() != 1)
	{
		return prefixes.empty() ? 2 : 3;
	}

	*prefix = *prefixes.begin();
	return 0;
}

static int read_record_at(FILE *fp, long offset, uint8_t *packet,
	rudpheader *header)
{
	if (fseek(fp, offset, SEEK_SET) == -1)
	{
		return 1;
	}

	size_t read_count = fread(packet, PACKET_BUFFER_SIZE, 1, fp);
	if (read_count != 1)
	{
		return 2;
	}

	parseHeader(packet, header);
	return 0;
}

static int taste_chunk_file(const std::string &path, chunk_info *chunk)
{
	struct stat stat_buf;
	if (stat(path.c_str(), &stat_buf) == -1)
	{
		return 1;
	}
	if (stat_buf.st_size < PACKET_BUFFER_SIZE)
	{
		return 2;
	}

	long packet_count = stat_buf.st_size / PACKET_BUFFER_SIZE;
	if (packet_count < 1)
	{
		return 3;
	}

	FILE *fp = fopen(path.c_str(), "rb");
	if (fp == NULL)
	{
		return 4;
	}

	uint8_t packet[PACKET_BUFFER_SIZE];
	rudpheader header;
	int ret = read_record_at(fp, 0, packet, &header);
	if (ret)
	{
		fclose(fp);
		return 5;
	}
	chunk->start_time = header.sendtime;

	ret = read_record_at(fp, (packet_count - 1) * PACKET_BUFFER_SIZE,
		packet, &header);
	fclose(fp);
	if (ret)
	{
		return 6;
	}

	chunk->end_time = header.sendtime;
	if (chunk->end_time > chunk->start_time + CHUNK_TIME_DIFF + 1000000ULL)
	{
		chunk->end_time = chunk->start_time + CHUNK_TIME_DIFF;
	}

	return 0;
}

static int load_chunks(const std::string &directory, const std::string &prefix,
	std::map<uint32_t, chunk_info> *chunks)
{
	DIR *dir = opendir(directory.c_str());
	if (dir == NULL)
	{
		return 1;
	}

	std::map<uint32_t, chunk_info> next_chunks;
	struct dirent *dent;
	while ((dent = readdir(dir)) != NULL)
	{
		uint32_t id = 0;
		if (!parse_chunk_filename(dent->d_name, prefix, &id))
		{
			continue;
		}

		chunk_info chunk;
		chunk.id = id;
		chunk.start_time = 0;
		chunk.end_time = 0;
		chunk.path = chunk_path(directory, prefix, id);
		if (taste_chunk_file(chunk.path, &chunk) == 0)
		{
			next_chunks[id] = chunk;
		}
	}
	closedir(dir);

	if (next_chunks.empty())
	{
		return 2;
	}

	chunks->swap(next_chunks);
	return 0;
}

static int find_position_in_chunk(const chunk_info &chunk,
	uint64_t searched_time, playback_position *pos)
{
	FILE *fp = fopen(chunk.path.c_str(), "rb");
	if (fp == NULL)
	{
		return 1;
	}

	uint8_t packet[PACKET_BUFFER_SIZE];
	rudpheader header;
	long offset = 0;
	while (!shutdown_requested)
	{
		size_t read_count = fread(packet, PACKET_BUFFER_SIZE, 1, fp);
		if (read_count != 1)
		{
			fclose(fp);
			return 2;
		}

		parseHeader(packet, &header);
		if (header.sendtime >= searched_time)
		{
			pos->file_id = chunk.id;
			pos->offset = offset;
			pos->packet_time = header.sendtime;
			fclose(fp);
			return 0;
		}
		offset += PACKET_BUFFER_SIZE;
	}

	fclose(fp);
	return 3;
}

static int find_position(const std::map<uint32_t, chunk_info> &chunks,
	uint64_t searched_time, playback_position *pos)
{
	if (chunks.empty())
	{
		return 1;
	}

	if (searched_time == 0)
	{
		return find_position_in_chunk(chunks.begin()->second,
			chunks.begin()->second.start_time, pos);
	}

	for (std::map<uint32_t, chunk_info>::const_iterator it = chunks.begin();
		it != chunks.end(); ++it)
	{
		if (searched_time <= it->second.end_time)
		{
			uint64_t packet_time = searched_time;
			if (searched_time < it->second.start_time)
			{
				packet_time = it->second.start_time;
			}
			return find_position_in_chunk(it->second, packet_time, pos);
		}
	}

	return 2;
}

static int open_chunk_at(const std::string &directory, const std::string &prefix,
	const playback_position &pos, open_chunk *open)
{
	if (open->fp != NULL && open->file_id == pos.file_id)
	{
		if (fseek(open->fp, pos.offset, SEEK_SET) == -1)
		{
			return 1;
		}
		return 0;
	}

	if (open->fp != NULL)
	{
		fclose(open->fp);
		open->fp = NULL;
	}

	open->path = chunk_path(directory, prefix, pos.file_id);
	open->fp = fopen(open->path.c_str(), "rb");
	if (open->fp == NULL)
	{
		return 2;
	}
	open->file_id = pos.file_id;
	if (fseek(open->fp, pos.offset, SEEK_SET) == -1)
	{
		fclose(open->fp);
		open->fp = NULL;
		return 3;
	}
	return 0;
}

static int setup_multicast_socket(const s_interfacesink &sink,
	struct sockaddr_in *sockaddr_mc_output)
{
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0)
	{
		return -1;
	}

	int ttl = sink.ttl;
	if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, (void *)&ttl,
		sizeof(ttl)) == -1)
	{
		syslog(LOG_WARNING, "player: failed to set multicast ttl %d: %s",
			ttl, strerror(errno));
	}

	memset(sockaddr_mc_output, 0, sizeof(*sockaddr_mc_output));
	sockaddr_mc_output->sin_family = AF_INET;
	sockaddr_mc_output->sin_addr.s_addr = inet_addr(sink.host);
	sockaddr_mc_output->sin_port = htons(sink.port);

	return fd;
}

static int send_packet(int fd, const struct sockaddr_in &sockaddr_mc_output,
	const s_interfacesink &sink, uint8_t *packet, const rudpheader &header)
{
	int strip_rtp = 0;
	if (sink.rawudp && packet[RUDP_HEADER_SIZE] != 0x47)
	{
		strip_rtp = RTP_MIN_HEADER_SIZE;
	}

	if (header.payloadlength <= strip_rtp ||
		header.payloadlength > PKTSIZE + RTP_MIN_HEADER_SIZE)
	{
		return 1;
	}

	ssize_t wanted = header.payloadlength - strip_rtp;
	ssize_t sent = sendto(fd, packet + RUDP_HEADER_SIZE + strip_rtp, wanted, 0,
		(const struct sockaddr *)&sockaddr_mc_output, sizeof(sockaddr_mc_output));
	if (sent != wanted)
	{
		return 2;
	}

	return 0;
}

static void reset_play_clock(playback_state *state, uint64_t packet_time,
	uint64_t wall_time)
{
	state->record_base_time = packet_time;
	state->wall_base_time = wall_time;
}

static void set_playerstats(s_playerstats *stats, const playback_state &state,
	uint64_t streaminterface_id, uint64_t recording_id)
{
	memset(stats, 0, sizeof(*stats));
	stats->streaminterface_id = streaminterface_id;
	stats->pathnumber = 0;
	stats->recording_id = recording_id;
	stats->e_state = state.state == PLAYER_RUN_PLAY ? PLAY : PAUSE;
	stats->startplayat_timestamp = state.startplayat == NO_TIME_LIMIT ? 0 :
		state.startplayat;
	stats->startpos_timestamp = state.startpos;
	stats->lastpos_timestamp = state.lastpos;
	stats->currpos_timestamp = state.currpos;
	stats->playto_timestamp = state.playto == NO_TIME_LIMIT ? 0 : state.playto;
}

static void update_stats(PGconn **dbconn, const s_interface &streaminterface,
	const s_interfacesource &source, const s_interfacesink &sink,
	const playback_state &state, uint64_t recording_id, uint64_t sent_bytes,
	uint64_t elapsed_usec, uint64_t stats_streaminterface_id)
{
	s_playerstats playerstats;
	set_playerstats(&playerstats, state, stats_streaminterface_id, recording_id);
	db_insert_playerstats(dbconn, &playerstats);

	s_interfacelivestatistic live;
	memset(&live, 0, sizeof(live));
	live.streaminterface_id = streaminterface.id;
	strncpy(live.mode, streaminterface.config.mode, DB_MAX_MODE_LENGTH - 1);
	strncpy(live.state, state.state == PLAYER_RUN_PLAY ? "sending" : "pause",
		DB_MAX_STATE - 1);
	live.bitrateout = elapsed_usec > 0 ? (sent_bytes * 8 * 1000000ULL) /
		elapsed_usec : 0;
	live.pathnumber = 0;
	strncpy(live.pathname, sink.name, DB_MAX_NAME_LENGTH - 1);
	snprintf(live.source, DB_MAX_HOSTNAME_PORT_LENGTH, "%s:%d",
		source.host, source.port);
	snprintf(live.destination, DB_MAX_HOSTNAME_PORT_LENGTH, "%s:%d",
		sink.host, sink.port);
	live.ttl = sink.ttl;
	live.multipath = streaminterface.config.multipath;
	live.mploadbalance = streaminterface.config.mploadbalance;
	dbupdatestreaminterfacelivestatistic(dbconn, &live);
}

static int fetch_next_command(PGconn **dbconn, uint64_t primary_id,
	uint64_t fallback_id, s_playercoms *command, uint64_t *command_owner_id)
{
	int num_fetched = 0;
	int ret = db_fetch_playercoms(dbconn, command, 1, &num_fetched, primary_id);
	if (ret == 0 && num_fetched == 1)
	{
		*command_owner_id = primary_id;
		return 0;
	}

	if (fallback_id != primary_id)
	{
		ret = db_fetch_playercoms(dbconn, command, 1, &num_fetched, fallback_id);
		if (ret == 0 && num_fetched == 1)
		{
			*command_owner_id = fallback_id;
			return 0;
		}
	}

	return 1;
}

static int apply_command(PGconn **dbconn, const std::string &directory,
	const std::string &prefix, std::map<uint32_t, chunk_info> *chunks,
	playback_state *state, const s_playercoms &command)
{
	if (command.pathnumber != 0)
	{
		syslog(LOG_WARNING, "player: ignoring command for path %" PRIu64,
			command.pathnumber);
		db_delete_playercomsbyid(dbconn, command.id);
		return 0;
	}

	load_chunks(directory, prefix, chunks);
	const uint64_t now = now_usec();

	switch (command.e_action)
	{
	case JUMP:
		{
		playback_position new_pos;
		if (find_position(*chunks, command.jumptopos_timestamp, &new_pos) != 0)
		{
			syslog(LOG_WARNING, "player: could not jump to %" PRIu64,
				command.jumptopos_timestamp);
			db_delete_playercomsbyid(dbconn, command.id);
			return 1;
		}

		state->pos = new_pos;
		state->have_position = true;
		state->currpos = new_pos.packet_time;
		state->startpos = new_pos.packet_time;
		state->playto = command.playto_timestamp == 0 ? NO_TIME_LIMIT :
			command.playto_timestamp;
		if (command.startplayat_timestamp != 0)
		{
			state->startplayat = command.startplayat_timestamp;
			state->state = PLAYER_RUN_PAUSE;
		}
		else
		{
			state->startplayat = NO_TIME_LIMIT;
			state->state = PLAYER_RUN_PAUSE;
		}
		reset_play_clock(state, new_pos.packet_time,
			command.startplayat_timestamp != 0 ? command.startplayat_timestamp : now);
		}
		break;
	case PLAY:
		if (!state->have_position)
		{
			playback_position new_pos;
			if (find_position(*chunks, 0, &new_pos) != 0)
			{
				db_delete_playercomsbyid(dbconn, command.id);
				return 1;
			}
			state->pos = new_pos;
			state->have_position = true;
			state->currpos = new_pos.packet_time;
			state->startpos = new_pos.packet_time;
		}
		state->state = PLAYER_RUN_PLAY;
		state->startplayat = NO_TIME_LIMIT;
		state->playto = command.playto_timestamp == 0 ? state->playto :
			command.playto_timestamp;
		reset_play_clock(state, state->currpos, now);
		break;
	case PAUSE:
		state->state = PLAYER_RUN_PAUSE;
		state->startplayat = NO_TIME_LIMIT;
		break;
	case PLAYTO:
		state->playto = command.playto_timestamp == 0 ? NO_TIME_LIMIT :
			command.playto_timestamp;
		break;
	default:
		db_delete_playercomsbyid(dbconn, command.id);
		return 1;
	}

	db_delete_playercomsbyid(dbconn, command.id);
	return 0;
}

static bool command_wait(uint64_t usec)
{
	while (usec > 0 && !shutdown_requested && !reload_requested)
	{
		uint64_t step = std::min<uint64_t>(usec, 100000);
		usleep(step);
		usec -= step;
	}
	return reload_requested != 0;
}

static bool advance_to_next_chunk(const std::string &directory,
	const std::string &prefix, std::map<uint32_t, chunk_info> *chunks,
	playback_position *pos, open_chunk *current, bool recording_can_grow)
{
	while (!shutdown_requested)
	{
		load_chunks(directory, prefix, chunks);
		std::map<uint32_t, chunk_info>::iterator next = chunks->upper_bound(pos->file_id);
		if (next != chunks->end())
		{
			pos->file_id = next->first;
			pos->offset = 0;
			pos->packet_time = next->second.start_time;
			if (current->fp != NULL)
			{
				fclose(current->fp);
				current->fp = NULL;
			}
			return true;
		}

		if (!recording_can_grow)
		{
			return false;
		}

		if (command_wait(100000))
		{
			return true;
		}
	}

	return false;
}

int main(int argc, char **argv)
{
	std::string interfacename;
	std::string recording_dir_arg;
	int run_as_daemon = 1;
	int arg_ret = parse_arguments(argc, argv, &interfacename, &recording_dir_arg,
		&run_as_daemon);
	if (arg_ret != 0)
	{
		return arg_ret > 0 ? 0 : 1;
	}

	char pidfilename[MAX_PID_FILENAME];
	snprintf(pidfilename, sizeof(pidfilename), PID_PLAYER_FILENAME_PATTERN,
		interfacename.c_str());
	if (access(pidfilename, F_OK) != -1)
	{
		syslog(LOG_ERR, "PID file %s already exists!", pidfilename);
		return 1;
	}

	if (run_as_daemon)
	{
		daemonize();
	}

	int pidfd = create_pid(pidfilename);
	if (pidfd == -1)
	{
		syslog(LOG_ERR, "Cant create pid file %s", pidfilename);
		return 1;
	}

	PGconn *dbconn = DBConnect();
	if (dbconn == NULL)
	{
		syslog(LOG_ERR, "%s: failed to connect to database",
			interfacename.c_str());
		close_pid_file(pidfd, pidfilename);
		return 1;
	}

	s_interface streaminterface;
	memset(&streaminterface, 0, sizeof(streaminterface));
	int retval = dbfetchstreaminterfacebyname(&dbconn, &streaminterface,
		const_cast<char *>(interfacename.c_str()));
	if (retval || streaminterface.id == 0)
	{
		syslog(LOG_ERR, "%s: failed to fetch streaminterface (%d)",
			interfacename.c_str(), retval);
		DBClose(dbconn);
		close_pid_file(pidfd, pidfilename);
		return 1;
	}

	retval = dbfetchstreaminterfaceconfigbystreaminterfaceid(&dbconn,
		&streaminterface.config, streaminterface.id);
	if (retval || streaminterface.config.id == 0)
	{
		syslog(LOG_ERR, "%s: failed to fetch player config (%d)",
			interfacename.c_str(), retval);
		DBClose(dbconn);
		close_pid_file(pidfd, pidfilename);
		return 1;
	}

	if (strncmp(streaminterface.config.mode, "player", DB_MAX_MODE_LENGTH) != 0)
	{
		syslog(LOG_ERR, "%s: Mode is not player", interfacename.c_str());
		DBClose(dbconn);
		close_pid_file(pidfd, pidfilename);
		return 1;
	}

	memset(&streaminterface.source, 0, DB_MAX_SOURCE * sizeof(s_interfacesource));
	retval = dbfetchstreaminterfacesourcebystreaminterfaceconfigid(&dbconn,
		&streaminterface.source[0], streaminterface.config.id);
	if (retval)
	{
		syslog(LOG_ERR, "%s: failed to fetch source config (%d)",
			interfacename.c_str(), retval);
		DBClose(dbconn);
		close_pid_file(pidfd, pidfilename);
		return 1;
	}

	memset(&streaminterface.sink, 0, DB_MAX_SINK * sizeof(s_interfacesink));
	retval = dbfetchstreaminterfacesinkbystreaminterfaceconfigid(&dbconn,
		&streaminterface.sink[0], streaminterface.config.id);
	if (retval)
	{
		syslog(LOG_ERR, "%s: failed to fetch single sink config (%d)",
			interfacename.c_str(), retval);
		DBClose(dbconn);
		close_pid_file(pidfd, pidfilename);
		return 1;
	}

	if (streaminterface.sink[0].port == 0 ||
		strnlen(streaminterface.sink[0].host, DB_MAX_HOSTNAME_LENGTH) < 3)
	{
		syslog(LOG_ERR, "%s: invalid sink configuration", interfacename.c_str());
		DBClose(dbconn);
		close_pid_file(pidfd, pidfilename);
		return 1;
	}

	char base_video_path[VIDEO_PATH_LEN];
	memset(base_video_path, 0, sizeof(base_video_path));
	dbfetch_video_path(&dbconn, base_video_path);

	s_recordings recording;
	memset(&recording, 0, sizeof(recording));
	uint64_t recording_id = streaminterface.source[0].recording_id;
	if (recording_id == 0)
	{
		recording_id = streaminterface.sink[0].recording_id;
	}
	if (recording_id != 0)
	{
		retval = db_fetch_recording(&dbconn, &recording, recording_id);
		if (retval || recording.id == 0)
		{
			syslog(LOG_ERR, "%s: failed to fetch recording %" PRIu64 " (%d)",
				interfacename.c_str(), recording_id, retval);
			DBClose(dbconn);
			close_pid_file(pidfd, pidfilename);
			return 1;
		}
	}

	std::string recording_directory = recording_dir_arg;
	if (recording_directory.empty())
	{
		if (recording.id == 0)
		{
			syslog(LOG_ERR, "%s: no recording directory or recording_id configured",
				interfacename.c_str());
			DBClose(dbconn);
			close_pid_file(pidfd, pidfilename);
			return 1;
		}
		recording_directory = std::string(base_video_path) + "/" + recording.name;
	}

	std::string chunk_prefix;
	if (recording.streaminterface_id != 0)
	{
		s_interface recorder_interface;
		memset(&recorder_interface, 0, sizeof(recorder_interface));
		if (dbfetchstreaminterfacebyid(&dbconn, &recorder_interface,
			recording.streaminterface_id) == 0 && recorder_interface.id != 0)
		{
			chunk_prefix = recorder_interface.name;
		}
	}
	if (chunk_prefix.empty())
	{
		int prefix_ret = discover_chunk_prefix(recording_directory, &chunk_prefix);
		if (prefix_ret != 0)
		{
			syslog(LOG_ERR, "%s: failed to discover chunk prefix in %s (%d)",
				interfacename.c_str(), recording_directory.c_str(), prefix_ret);
			DBClose(dbconn);
			close_pid_file(pidfd, pidfilename);
			return 1;
		}
	}

	std::map<uint32_t, chunk_info> chunks;
	if (load_chunks(recording_directory, chunk_prefix, &chunks) != 0)
	{
		syslog(LOG_ERR, "%s: no readable chunks in %s for prefix %s",
			interfacename.c_str(), recording_directory.c_str(), chunk_prefix.c_str());
		DBClose(dbconn);
		close_pid_file(pidfd, pidfilename);
		return 1;
	}

	if (setup_signal_handlers() != 0)
	{
		syslog(LOG_ERR, "%s: failed to setup signal handlers",
			interfacename.c_str());
		DBClose(dbconn);
		close_pid_file(pidfd, pidfilename);
		return 1;
	}

	db_delete_streaminterfacelivestatistic(&dbconn, streaminterface.id);
	db_delete_playerstats(&dbconn, streaminterface.config.id);
	db_delete_playercoms(&dbconn, streaminterface.config.id);
	if (streaminterface.config.id != streaminterface.id)
	{
		db_delete_playercoms(&dbconn, streaminterface.id);
	}

	struct sockaddr_in sockaddr_mc_output;
	int fd_mc_output = setup_multicast_socket(streaminterface.sink[0],
		&sockaddr_mc_output);
	if (fd_mc_output < 0)
	{
		syslog(LOG_ERR, "%s: failed to setup multicast output socket",
			interfacename.c_str());
		DBClose(dbconn);
		close_pid_file(pidfd, pidfilename);
		return 1;
	}

	playback_state state;
	memset(&state, 0, sizeof(state));
	state.state = PLAYER_RUN_PLAY;
	state.have_position = false;
	state.startplayat = NO_TIME_LIMIT;
	state.playto = NO_TIME_LIMIT;
	if (find_position(chunks, recording.recordingstart_timestamp, &state.pos) != 0)
	{
		if (find_position(chunks, 0, &state.pos) != 0)
		{
			syslog(LOG_ERR, "%s: failed to locate first playback position",
				interfacename.c_str());
			close(fd_mc_output);
			DBClose(dbconn);
			close_pid_file(pidfd, pidfilename);
			return 1;
		}
	}
	state.have_position = true;
	state.record_base_time = state.pos.packet_time;
	state.wall_base_time = now_usec();
	state.startpos = state.pos.packet_time;
	state.currpos = state.pos.packet_time;
	state.lastpos = chunks.rbegin()->second.end_time;

	open_chunk current;
	current.fp = NULL;
	current.file_id = 0;

	uint8_t packet[PACKET_BUFFER_SIZE];
	rudpheader header;
	uint64_t stats_last_time = now_usec();
	uint64_t stats_sent_bytes = 0;
	bool recording_can_grow = recording.id == 0 || recording.e_state == RECORDING ||
		recording.e_state == INIT;

	syslog(LOG_NOTICE, "%s: player started from %s prefix %s to %s:%d",
		interfacename.c_str(), recording_directory.c_str(), chunk_prefix.c_str(),
		streaminterface.sink[0].host, streaminterface.sink[0].port);

	while (!shutdown_requested)
	{
		if (reload_requested)
		{
			s_playercoms command;
			uint64_t command_owner_id = 0;
			if (fetch_next_command(&dbconn, streaminterface.config.id,
				streaminterface.id, &command, &command_owner_id) == 0)
			{
				(void)command_owner_id;
				apply_command(&dbconn, recording_directory, chunk_prefix, &chunks,
					&state, command);
				if (current.fp != NULL)
				{
					fclose(current.fp);
					current.fp = NULL;
				}
			}
			reload_requested = 0;
		}

		uint64_t now = now_usec();
		if (state.state == PLAYER_RUN_PAUSE &&
			state.startplayat != NO_TIME_LIMIT && now >= state.startplayat)
		{
			state.state = PLAYER_RUN_PLAY;
			reset_play_clock(&state, state.currpos, now);
			state.startplayat = NO_TIME_LIMIT;
		}

		if (state.state == PLAYER_RUN_PAUSE)
		{
			command_wait(100000);
		}
		else
		{
			if (open_chunk_at(recording_directory, chunk_prefix, state.pos,
				&current) != 0)
			{
				if (!advance_to_next_chunk(recording_directory, chunk_prefix,
					&chunks, &state.pos, &current, recording_can_grow))
				{
					state.state = PLAYER_RUN_PAUSE;
				}
				continue;
			}

			size_t read_count = fread(packet, PACKET_BUFFER_SIZE, 1, current.fp);
			if (read_count != 1)
			{
				clearerr(current.fp);
				if (!advance_to_next_chunk(recording_directory, chunk_prefix,
					&chunks, &state.pos, &current, recording_can_grow))
				{
					state.state = PLAYER_RUN_PAUSE;
				}
				continue;
			}

			parseHeader(packet, &header);
			state.currpos = header.sendtime;
			if (state.playto != NO_TIME_LIMIT && header.sendtime >= state.playto)
			{
				state.state = PLAYER_RUN_PAUSE;
				continue;
			}

			uint64_t due = state.wall_base_time;
			if (header.sendtime >= state.record_base_time)
			{
				due += header.sendtime - state.record_base_time;
			}
			now = now_usec();
			if (due > now && command_wait(due - now))
			{
				if (fseek(current.fp, -PACKET_BUFFER_SIZE, SEEK_CUR) == 0)
				{
					state.pos.offset = ftell(current.fp);
				}
				continue;
			}

			if (send_packet(fd_mc_output, sockaddr_mc_output,
				streaminterface.sink[0], packet, header) != 0)
			{
				syslog(LOG_ERR, "%s: failed to send multicast packet",
					interfacename.c_str());
				break;
			}

			stats_sent_bytes += header.payloadlength;
			state.pos.offset += PACKET_BUFFER_SIZE;
			state.pos.packet_time = header.sendtime;
			state.currpos = header.sendtime;
			load_chunks(recording_directory, chunk_prefix, &chunks);
			if (!chunks.empty())
			{
				state.lastpos = chunks.rbegin()->second.end_time;
			}
		}

		now = now_usec();
		if (now - stats_last_time >= 1000000ULL)
		{
			update_stats(&dbconn, streaminterface, streaminterface.source[0],
				streaminterface.sink[0], state, recording_id, stats_sent_bytes,
				now - stats_last_time, streaminterface.config.id);
			stats_last_time = now;
			stats_sent_bytes = 0;
		}
	}

	if (current.fp != NULL)
	{
		fclose(current.fp);
	}
	close(fd_mc_output);
	update_stats(&dbconn, streaminterface, streaminterface.source[0],
		streaminterface.sink[0], state, recording_id, stats_sent_bytes,
		now_usec() - stats_last_time, streaminterface.config.id);

	syslog(LOG_NOTICE, "%s: stopped as player", interfacename.c_str());
	DBClose(dbconn);
	close_pid_file(pidfd, pidfilename);
	return 0;
}
