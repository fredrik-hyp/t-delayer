#ifndef __DBSTREAMINTERFACES_H__
#define __DBSTREAMINTERFACES_H__

#include "dbcommon.h"
#include "dbvirtualsources.h"

typedef struct _streaminterfaceconfig {
	uint64_t id;
	uint64_t streaminterface_id;
	char typeofconfig[DB_MAX_TYPEOFCONFIG];
	int autostart;
	int msbuffer;
	char resendmode[DB_MAX_RESENDMODE];
	char mode[DB_MAX_MODE_LENGTH];
	int nosessionid;
	int backuptoprimary;
	int multipath;
	int mploadbalance;
	int actasserver;
} s_interfaceconfig;

typedef struct _streaminterfaceruninfo {
	uint64_t id;
	uint64_t streaminterface_id;
	uint64_t lastchanged;
	uint64_t pid;
	char startstopinfo[DB_MAX_STARTSTOPINFO];
} s_interfaceruninfo;

typedef struct _streaminterfacesource {
	uint64_t id;
	uint64_t streaminterfaceconfig_id;
	char name[DB_MAX_NAME_LENGTH];
	int priority;
	char sourcetype[DB_MAX_INTERFACETYPE];
	char host[DB_MAX_HOSTNAME_LENGTH];
	uint16_t port;
	char talkbackhost[DB_MAX_HOSTNAME_LENGTH];
	uint16_t talkbackport;
	uint64_t hostclientcertificate_id;
	uint64_t talkbackclientcertificate_id;
	uint16_t maxretrytime;
	uint16_t streammissfailovertimeout;
	uint64_t virtualsource_id;
	uint16_t vsourcerevision;
	// rec_player
	uint64_t recording_id;

	uint16_t addrtp;
} s_interfacesource;

typedef struct _streaminterfacebackupsource {
	uint64_t id;
	uint64_t streaminterfacesource_id;
	char name[DB_MAX_NAME_LENGTH];
	int priority;
	char sourcetype[DB_MAX_INTERFACETYPE];
	char host[DB_MAX_HOSTNAME_LENGTH];
	uint16_t port;
	char talkbackhost[DB_MAX_HOSTNAME_LENGTH];
	uint16_t talkbackport;
	uint64_t hostclientcertificate_id;
	uint64_t talkbackclientcertificate_id;
	uint16_t maxretrytime;
	uint16_t streammissfailovertimeout;
} s_interfacebackupsource;

typedef struct _streaminterfacesink {
	uint64_t id;
	uint64_t streaminterfaceconfig_id;
	char name[DB_MAX_NAME_LENGTH];
	char sinktype[DB_MAX_INTERFACETYPE];
	char host[DB_MAX_HOSTNAME_LENGTH];
	uint16_t port;
	char talkbackhost[DB_MAX_HOSTNAME_LENGTH];
	uint16_t talkbackport;
	uint16_t ttl;
	uint64_t hostclientcertificate_id;
	uint64_t talkbackclientcertificate_id;
	uint16_t priority;
	uint16_t maxretrytime;
	uint16_t streammissfailovertimeout;
	uint64_t maxbps;
	uint16_t rawudp;
	// rec_player
	uint64_t recording_id;
} s_interfacesink;

typedef struct _streaminterfacebackupsink {
	uint64_t id;
	uint64_t streaminterfacesink_id;
	char name[DB_MAX_NAME_LENGTH];
	char sinktype[DB_MAX_INTERFACETYPE];
	char host[DB_MAX_HOSTNAME_LENGTH];
	uint16_t port;
	char talkbackhost[DB_MAX_HOSTNAME_LENGTH];
	uint16_t talkbackport;
	uint64_t hostclientcertificate_id;
	uint64_t talkbackclientcertificate_id;
	uint16_t priority;
	uint16_t maxretrytime;
	uint16_t streammissfailovertimeout;
	uint64_t maxbps;
	uint16_t rawudp;
	uint16_t ttl;
} s_interfacebackupsink;

typedef struct _streaminterfacelog {
	uint64_t id;
	uint64_t streaminterface_id;
	char timestamp[DB_MAX_TIMESTAMP];
	uint8_t level;
	char text[DB_MAX_LOGTEXT];
} s_interfacelog;

typedef struct _streaminterfacelivestatistic {
	uint64_t id;
	uint64_t streaminterface_id;
	char mode[DB_MAX_MODE_LENGTH];
	char state[DB_MAX_STATE];
	uint64_t timestamp;
	uint64_t bitratein;
	uint64_t bitrateout;
	uint64_t bufferlevel;
	uint16_t rtt;
	char source[DB_MAX_HOSTNAME_PORT_LENGTH];
	char destination[DB_MAX_HOSTNAME_PORT_LENGTH];
	uint64_t udpdrops;
	uint64_t acc_rtp_drops;
	uint64_t pathnumber;
	char pathname[DB_MAX_NAME_LENGTH];
    uint64_t hostclientcertificate_id;
    uint64_t talkbackclientcertificate_id;
	uint16_t ttl;
	uint16_t multipath;
	uint16_t mploadbalance;
/*	char mp1state[DB_MAX_NAME_LENGTH];
	uint64_t mp1bitratein;
	uint64_t mp1bitrateout;
	char mp1source[DB_MAX_HOSTNAME_LENGTH+5];
	char mp1destination[DB_MAX_HOSTNAME_LENGTH+5];
	uint64_t mp1rtpdrops;
	char mp1pathname[DB_MAX_NAME_LENGTH];
    uint64_t mp1cliencertificate_id;
	char mp2state[DB_MAX_NAME_LENGTH];
	uint64_t mp2bitratein;
	uint64_t mp2bitrateout;
	char mp2source[DB_MAX_HOSTNAME_LENGTH+5];
	char mp2destination[DB_MAX_HOSTNAME_LENGTH+5];
	uint64_t mp2rtpdrops;
	char mp2pathname[DB_MAX_NAME_LENGTH];
    uint64_t mp1cliencertificate_id;*/
} s_interfacelivestatistic;

typedef struct _streamstats {
	uint64_t id;
	uint64_t streaminterface_id;
	char timestamp[DB_MAX_TIMESTAMP];
	char mode[DB_MAX_MODE_LENGTH];
	char source_host[DB_MAX_HOSTNAME_LENGTH];
	uint16_t source_port;
	char sink_host[DB_MAX_HOSTNAME_LENGTH];
	uint16_t sink_port;
	uint16_t drops;
	uint64_t bitrate;
	uint64_t bitrate_in;
	uint64_t bitrate_out;
	uint64_t resendcounter;
	uint16_t rudprtt;
	uint64_t pathnumber;
	char pathname[DB_MAX_NAME_LENGTH];
	uint64_t linkdrops;
	uint16_t multipath;
	uint16_t mploadbalance;
} s_streamstats; 

typedef struct _streaminterface {
	uint64_t id;
	char interfacetype[DB_MAX_STREAMINTERFACETYPE];
	char name[DB_MAX_NAME_LENGTH];
	char usertext[DB_MAX_NAME_LENGTH];
	s_interfaceconfig config;

	s_interfacesource source[DB_MAX_SOURCE];
	unsigned short num_backup_source;
	unsigned short active_source;

	s_interfacesink sink[DB_MAX_SINK];
	unsigned short num_backup_sink;	
	unsigned short active_sink;
} s_interface;

typedef struct _rootcertificates {
	uint64_t id;
	char created[DB_MAX_TIMESTAMP];
	uint64_t expiration_days;
	uint64_t organisation_id;
	char name[DB_MAX_NAME_LENGTH];
	char public_part[DB_MAX_CERTIFICATE_LEN];
	char private_part[DB_MAX_CERTIFICATE_LEN];
} s_rootcertificates;

typedef struct _clientcertificates {
	uint64_t id;
	char created[DB_MAX_TIMESTAMP];
	uint64_t expiration_days;
	uint64_t rootcertificate_id;
	char name[DB_MAX_NAME_LENGTH];
	char public_part[DB_MAX_CERTIFICATE_LEN];
	char private_part[DB_MAX_CERTIFICATE_LEN];
} s_clientcertificates;



extern s_interface streaminterface;

// === FETCH FUNCTIONs ===
int dbfetchallstreaminterfaces(PGconn **conn,
				int no_of_sinterface, s_interface *streaminterfaces, int *numinterfaces_copied);
int dbfetchstreaminterfaceruninfobystreaminterfaceid(PGconn **conn,
				s_interfaceruninfo *streaminterfaceruninfo, uint64_t streaminterfaceid);

// -- streaminterface SINK --
int dbfetchstreaminterfacesinkbystreaminterfaceconfigid(PGconn **conn,
				s_interfacesink *streaminterfacesink, uint64_t streaminterfaceconfigid);

int dbfetch_streaminterfacebackupsink_by_streaminterfacesinkid(PGconn **conn,
				s_interfacesink *streaminterfacesink, unsigned short *num_backup_sink,
				uint64_t streaminterfacesink_id);

// -- streaminterface SOURCE --
int dbfetchstreaminterfacesourcebystreaminterfaceconfigid(PGconn **conn,
				s_interfacesource *streaminterfacesource, uint64_t streaminterfaceconfigid);

int dbfetchstreaminterfacesource(PGconn **conn, 
                s_interfacesource *streaminterfacesource, s_interfaceruninfo *streaminterfaceruninfo,int *nRows);
                
int dbfetch_streaminterfacebackupsource_by_streaminterfacesourceid(PGconn **conn,
				s_interfacesource *streaminterfacesource, unsigned short *num_backup_source,
				uint64_t streaminterfacesource_id);

// -- streaminterface CONFIG --
int dbfetchstreaminterfaceconfigbyid(PGconn **conn,
								s_interfaceconfig *streaminterfaceconfig, uint64_t  id);
int dbfetchstreaminterfaceconfigbystreaminterfaceid(PGconn **conn,
				s_interfaceconfig *streaminterfaceconfig, uint64_t  streaminterfaceid);
int dbfetchstreaminterfaceconfigsforrecorderandplayer(PGconn **conn,
												s_interfaceconfig *streaminterfaceconfig, uint16_t maxconfigs);

// -- STREAMINTERFACE --
int dbfetchstreaminterfacebyid(PGconn **conn, s_interface *streaminterface, uint64_t id);
int dbfetchstreaminterfacebyname(PGconn **conn, s_interface *streaminterface, char *name);

// -- streaminterface LOG --
int dbfetchstreaminterfacelogbystreaminterfaceid(PGconn **conn,
				s_interfacelog *streaminterfacelog, uint64_t streaminterfaceid);
int dbfetch_interfacelog(PGconn **conn,
			uint64_t streaminterface_id, s_interfacelog *streaminterfacelog,
			uint16_t loglen, uint16_t *fetched_rows);


// -- streaminterface LIVESTATISTIC --
int dbfetchstreaminterfacelivestatisticbystreaminterfaceid(PGconn **conn,
				s_interfacelivestatistic *streaminterfacelivestatistic, uint64_t streaminterfaceid);
int dbfetchstreaminterfacelivestatisticmpbystreaminterfaceid(PGconn **conn,
				s_interfacelivestatistic *streaminterfacelivestatistics,
				uint64_t streaminterfaceid,
				uint16_t *fetchedrows);
				
// -- Root certificates --
int dbfetch_rootcertificates(PGconn **conn, uint64_t id, s_rootcertificates *rootcertificates);
int dbfetch_clientcertificates(PGconn **conn, uint64_t id, s_clientcertificates *clientcertificates);


// === UPDATE FUNCTIONS ===

int dbupdatestreaminterfacesourcefromvirtualsource(PGconn **conn, dbvirtualsource *vsource, uint64_t streaminterfaceconfigid);
int dbupdatestreaminterfaceusertext(PGconn **conn, uint64_t streaminterfaceid, char *_usertext);

int dbupdatestreaminterfaceconfigautostart(PGconn **conn, uint64_t streaminterfaceconfigid, int autostart);
int dbupdatestreaminterfaceconfigmode(PGconn **conn, uint64_t streaminterfaceconfigid, char *_mode);
int dbupdatestreaminterfaceconfigmsbuffer(PGconn **conn, uint64_t streaminterfaceconfigid, int _msbuffer);
int dbupdatestreaminterfaceconfigmultipath(PGconn **conn, uint64_t streaminterfaceconfigid, int multipath);
int dbupdatestreaminterfaceconfigmploadbalance(PGconn **conn, uint64_t streaminterfaceconfigid, int mploadbalance);

int dbupdatestreaminterfacesourcetype(PGconn **conn, uint64_t streaminterfacesourceid, char *_sourcetype);
int dbupdatestreaminterfacesourcehost(PGconn **conn, uint64_t streaminterfacesourceid, char *_sourcehost);
int dbupdatestreaminterfacesourceport(PGconn **conn, uint64_t streaminterfacesourceid,uint16_t sourceport);
int dbupdatestreaminterfacesourcerecordingid(PGconn **conn, uint64_t streaminterfacesourceid, uint64_t recordingid);
int dbupdatestreaminterfacesourcetalkbackhost(PGconn **conn, uint64_t streaminterfacesourceid, char *_talkbackhost);
int dbupdatestreaminterfacesourcetalkbackport(PGconn **conn, uint64_t streaminterfacesourceid, uint16_t talkbackport);
int dbupdatestreaminterfacesourcevirtualsourceid(PGconn **conn, uint64_t streaminterfacesourceid, uint64_t virtualsourceid);
int dbupdatestreaminterfacesourcehostclientcertificateid(PGconn **conn, uint64_t streaminterfacesourceid, uint64_t hostclientcertificateid);
int dbupdatestreaminterfacesourcetalkbackclientcertificateid(PGconn **conn, uint64_t streaminterfacesourceid, uint64_t talkbackclientcertificateid);

int dbupdatestreaminterfacesinktype(PGconn **conn, uint64_t streaminterfacesinkeid, char *_sinktype);
int dbupdatestreaminterfacesinkttl(PGconn **conn, uint64_t streaminterfacesourceid,uint16_t ttl);
int dbupdatestreaminterfacesinkhost(PGconn **conn, uint64_t streaminterfacesourceid, char *_sourcehost);
int dbupdatestreaminterfacesinkport(PGconn **conn, uint64_t streaminterfacesourceid,uint16_t sourceport);
int dbupdatestreaminterfacesinktalkbackhost(PGconn **conn, uint64_t streaminterfacesourceid, char *_talkbackhost);
int dbupdatestreaminterfacesinktalkbackport(PGconn **conn, uint64_t streaminterfacesourceid, uint16_t talkbackport);
int dbupdatestreaminterfacesinkhostclientcertificateid(PGconn **conn, uint64_t streaminterfacesinkid, uint64_t hostclientcertificateid);
int dbupdatestreaminterfacesinktalkbackclientcertificateid(PGconn **conn, uint64_t streaminterfacesinkid, uint64_t talkbackclientcertificateid);

int dbupdatestreaminterfacebackupsourcename(PGconn **conn, uint64_t streaminterfacecbackupsourceid, char *_name);
int dbupdatestreaminterfacebackupsourcemaxretrytime(PGconn **conn, uint64_t streaminterfacebackupsourceid, uint16_t maxretrytime);
int dbupdatestreaminterfacebackupsourcestreammissfailovertimeout(PGconn **conn, uint64_t streaminterfacebackupsourceid, uint16_t streammisstimeout);
int dbupdatestreaminterfacebackupsourcetype(PGconn **conn, uint64_t streaminterfacebackupsourceid, char *_sinktype);
int dbupdatestreaminterfacebackupsourcehost(PGconn **conn, uint64_t streaminterfacebackupsourceid, char *_host);
int dbupdatestreaminterfacebackupsourceport(PGconn **conn, uint64_t streaminterfacebackupsourceid, uint16_t port);
int dbupdatestreaminterfacebackupsourcetalkbackhost(PGconn **conn, uint64_t streaminterfacebackupsourceid, char *_talkbackhost);
int dbupdatestreaminterfacebackupsourcetalkbackport(PGconn **conn, uint64_t streaminterfacebackupsourceid, uint16_t talkbackport);
int dbupdatestreaminterfacebackupsourcehostclientcertificateid(PGconn **conn, uint64_t streaminterfacebackupsourceid, uint64_t hostclientcertificateid);
int dbupdatestreaminterfacebackupsourcetalkbackclientcertificateid(PGconn **conn, uint64_t streaminterfacebacakupsourceid, uint64_t talkbackclientcertificateid);

int dbupdatestreaminterfacebackupsinkname(PGconn **conn, uint64_t streaminterfacebackupsinkid, char *_name);
int dbupdatestreaminterfacebackupsinkmaxretrytime(PGconn **conn, uint64_t streaminterfaceackupsinkid, uint16_t maxretrytime);
int dbupdatestreaminterfacebackupsinktype(PGconn **conn, uint64_t streaminterfaceackupsinkid, char *_sinktype);
int dbupdatestreaminterfacebackupsinkhost(PGconn **conn, uint64_t streaminterfaceackupsinkid, char *_host);
int dbupdatestreaminterfacebackupsinkport(PGconn **conn, uint64_t streaminterfaceackupsinkid, uint16_t port);
int dbupdatestreaminterfacebackupsinktalkbackhost(PGconn **conn, uint64_t streaminterfaceackupsinkid, char *_talkbackhost);
int dbupdatestreaminterfacebackupsinktalkbackport(PGconn **conn, uint64_t streaminterfaceackupsinkid, uint16_t talkbackport);
int dbupdatestreaminterfacebackupsinkhostclientcertificateid(PGconn **conn, uint64_t streaminterfacebackupsinkid, uint64_t hostclientcertificateid);
int dbupdatestreaminterfacebackupsinktalkbackclientcertificateid(PGconn **conn, uint64_t streaminterfacebackupsinkid, uint64_t talkbackclientcertificateid);

int dbaddstreaminterfacebackupsink(PGconn **conn, s_interfacebackupsink *backupsink);
int dbaddstreaminterfacebackupsource(PGconn **conn, s_interfacebackupsource *backupsource);

int dbdeletestreaminterfacebackupsourcebyid(PGconn **conn, uint64_t id);
int dbdeletestreaminterfacebackupsinkbyid(PGconn **conn, uint64_t id);

// === Update - fallback to insert FUNCTION ===
int dbupdatestreaminterfacelivestatistic(PGconn **conn, s_interfacelivestatistic *streaminterfacelivestatistic);
int dbupdatestreaminterfaceruninfo(PGconn **conn, s_interfaceruninfo *streaminterfaceruninfo);

// === ADD FUNCTION ===
int dbaddstreaminterfacelog(PGconn **conn, s_interfacelog *streaminterfacelog);

// === INSERT FUNCTION ===
int dbinsertstreamstats(PGconn **conn, s_streamstats *streamstats);

// === CLEANUP FUNCTIONS ===
int db_delete_streaminterfacelivestatistic(PGconn **conn, uint64_t streaminterfaceid);

#endif
