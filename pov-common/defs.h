#ifndef DEFS_H
#define DEFS_H

#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>
#if __linux__
#include <endian.h>
#else
#include <sys/endian.h>
#endif

//#define DEBUG

#define RTPPKTSIZE 1328

struct rawpackets
{

    unsigned short      rtpseq;
    char                data[RTPPKTSIZE]; // RTP + MPEG packets.
    size_t              data_length;
    char                clients_sent;
    struct rawpackets   *next;
    struct rawpackets   *prev;
};

typedef enum
{
    UDP = 0,
    TCP = 1,
    MULTICAST = 2
} transport_type;

typedef enum
{
    RAW = 0,
    RTP = 1
} packet_type;

struct connection
{
    pthread_t           thread_id;
    pthread_mutex_t     thread_mutex;
    bool                enabled;
    int                 priority;
    transport_type      transportType;
    packet_type         packetType;
    struct in_addr      ipaddress;
    struct sockaddr_in  sockaddr;
    struct ip_mreq      mreq;
    uint16_t            port;
    int                 fd;
    char                client_sent;
    short               current_rtpseq;
    struct rawpackets   *current_buffer;
    struct config       *config;
};

#define RTP_VERSION         0xC0
#define RTP_PADDING         0x20
#define RTP_EXTENSION       0x10
#define RTP_CSRC_COUNT      0x0F
#define RTP_MARKER          0x80
#define RTP_PAYLOAD_TYPE    0x7F
#define RTP_SEQUENCE_NR     0x0000FFFF

struct raw_rtp_packet
{
    char                VPXCC;          // 1 byte Version(2bit), padding(1bit), Extension(1bit) CSRC count(4bit)
    char                MPT;            // 1 byte Marker(1bit) Payload Type(7 bit)
    uint16_t            sequence_number;// 2 byte
    uint32_t            timestamp;      // 4 byte
    uint32_t            SSRC;           // 4 byte
    uint32_t            CSRC;           // 4 byte+4BxCC
    // Profile specific extension headerid + extension header length position depends on CC field 4byte*CC
    // Extension header, depends on extension header length
} __attribute__((packed)) ;

#define MPEG_SYNC_BYTE				0x47
#define MPEG_TRANSPORT_ERROR_INDICATOR		0x80
#define MPEG_PAYLOAD_UNIT_START_INDICATOR	0x40
#define MPEG_TRANSPORT_PRIORITY			0x20
#define MPEG_PID				0x1F
#define MPEG_SCRAMBLING_CONTROL			0xC0
#define MPEG_ADAPTATION_FIELD_EXIST		0x30
#define MPEG_CONTINUITY_COUNTER			0x0F

#define MPEG_TRANSPORT_ERROR_INDICATOR_CALC(X)		(X&MPEG_TRANSPORT_ERROR_INDICATOR)>>8
#define MPEG_PAYLOAD_UNIT_START_INDICATOR_CALC(X)	(X& MPEG_PAYLOAD_UNIT_START_INDICATOR)>>7
#define MPEG_TRANSPORT_PRIORITY_CALC(X)			(X& MPEG_TRANSPORT_PRIORITY) >> 5
#define MPEG_PID_CALC(X,Y)				((X&MPEG_PID)<<8)|(0xFF & Y)
#define MPEG_SCRAMBLING_CONTROL_CALC(X)			(X& MPEG_SCRAMBLING_CONTROL)>>6
#define MPEG_ADAPTATION_FIELD_EXIST_CALC(X)		(X& MPEG_ADAPTATION_FIELD_EXIST)>>4

// Adaption field
// 01 = no adaptation fields, payload only
// 10 = adaptation field only
// 11 = adaptation field and payload
#define MPEG_ADAPTION_FIELD_PAYLOAD_ONLY	0x01
#define MPEG_ADAPTION_FIELD_ONLY		0x02
#define MPEG_ADAPTION_FIELD_AND_PAYLOAD		0x03

struct raw_mpeg_packet
{
    char                sync_byte;      // 1 byte (0x47)
    char                TEI_PUS_TP_PID; // 1 byte, TransportErrorIndicator(1bit), Payload unit start indicator(1bit) Transport priority(1bit), PID(3bit)
    char                PID;		// 1 byte PID(8 bit)
    char                SC_AF_CC;       // 1 byte, Scrambling control(2bit), Adaption field exist(2bit), Continuity counter (4bit)
    // Adaption field, 0 or more..
    // Payload data, 0 or more
} __attribute__((packed)) ;

#define DISCONTINUITY_INDICATOR                 0x80
#define RANDOM_ACCESS_INDICATOR                 0x40
#define ELEMENTARY_STREAM_PRIORITY_INDICATOR    0x20
#define PCR_FLAG                                0x10
#define OPCR_FLAG                               0x08
#define SPLICING_POINT_FLAG                     0x04
#define TRANSPORT_PRIVATE_DATA_FLAG             0x02
#define ADAPTATION_FIELD_EXTENSION_FLAG         0x01


struct raw_adaption_field
{
    char                adaption_field_length;  // 1 byte
    // Adaptation Field Length          8 	Number of bytes in the adaptation field immediately following this byte
    // Discontinuity indicator          1 	Set to 1 if current TS packet is in a discontinuity state with respect to either the continuity counter or the program clock reference
    // Random Access indicator          1 	Set to 1 if the PES packet in this TS packet starts a video/audio sequence
    // Elementary stream priority indicator 	1 	1 = higher priority
    // PCR flag                         1 	1 means adaptation field does contain a PCR field
    // OPCR flag                        1 	1 means adaptation field does contain an OPCR field
    // Splicing point flag              1 	1 means presence of splice countdown field in adaptation field
    // Transport private data flag 	1 	1 means presence of private data bytes in adaptation field
    // Adaptation field extension flag 	1 	1 means presence of adaptation field extension
    char                flags;                  // 1 byte

    // PCR                  33+6+9 	Program clock reference, stored in 6 octets in big-endian as 33 bits base, 6 bits padding, 9 bits extension.
    // OPCR                 33+6+9 	Original Program clock reference. Helps when one TS is copied into another
    // Splice countdown     8 	Indicates how many TS packets from this one a splicing point occurs (may be negative)
    // stuffing bytes       variable

} __attribute__((packed));

#define PCR_BASE        0xFFFFFFFF80000000
#define PCR_PADDING     0x000000007E000000
#define PCR_EXTENSION   0x0000000001FF0000

#define PCR_BASE_TO_LONG(X)         (be64toh(X)&PCR_BASE)>>31
#define PCR_EXTENSION_TO_LONG(X)    (be64toh(X)&PCR_EXTENSION)>>16

struct raw_PCR_fields
{
    // Base = 33 bit
    // padding = 6 bit
    // extension = 9 bit
    // = 48 bit = 6 byte
    char                pcr_bytes[6];
} __attribute__((packed));


#endif // DEFS_H
