#include "defs.h"
#include "buffer.h"
#include "prot_rudp.h"
#include "print_struct.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
//#include <signal.h>
#include <unistd.h>
#include <errno.h>


void print_bufferelement(bufferelement *buf)
{
	fprintf(stderr, "========== print_bufferelement ================\n");

	print_rudpheader(&buf->header);
	fprintf(stderr, "buf->rudp_sendtime %" PRIu64 "\n\n", buf->rudp_sendtime);
	fprintf(stderr, "buf->rtp %d\n\n", buf->rtp);
	
//	printPacketBuffer(buf->packet_buffer+RUDP_HEADER_SIZE);

	fprintf(stderr, "buf->prev %p\n", (void*)buf->prev);
	fprintf(stderr, "buf->next %p\n", buf->next);
	fprintf(stderr, "buf->last %p\n", buf->last);

	fprintf(stderr, "========== print_bufferelement ================\n\n");
}

void print_rudpheader(rudpheader *rudp)
{
	fprintf(stderr, "========== print_rudpheader =============\n");
	switch(rudp->typeofpacket)
	{
	case RUDP_PACK_SETUP:
		fprintf(stderr, "rudp->typeofpacket: %d (RUDP_PACK_SETUP)\n", rudp->typeofpacket);
		break;
	case RUDP_PACK_CLOSE:
		fprintf(stderr, "rudp->typeofpacket: %d (RUDP_PACK_CLOSE)\n", rudp->typeofpacket);
		break;
	case RUDP_PACK_HEARTBEAT:
		fprintf(stderr, "rudp->typeofpacket: %d (RUDP_PACK_HEARTBEAT)\n", rudp->typeofpacket);
		break;
	case RUDP_PACK_DATA:
		fprintf(stderr, "rudp->typeofpacket: %d (RUDP_PACK_DATA)\n", rudp->typeofpacket);
		break;
	case RUDP_PACK_DATAACK:
		fprintf(stderr, "rudp->typeofpacket: %d (RUDP_PACK_DATAACK)\n", rudp->typeofpacket);
		break;
	}
	fprintf(stderr, "rudp->reserved: %x\n", rudp->reserved);

	fprintf(stderr, "rudp->sessionid: %u\n", rudp->sessionid);
	fprintf(stderr, "rudp->payloadlength: %u\n", rudp->payloadlength);
	fprintf(stderr, "rudp->channelnumber: %u\n", rudp->channelnumber);
	fprintf(stderr, "rudp->counter: %u\n", rudp->counter);
	fprintf(stderr, "rudp->sendtime: %" PRIu64 "\n", rudp->sendtime);
	
	fprintf(stderr, "========== print_rudpheader =============\n\n");
}

void print_rtp_header(uint8_t *pck)
{
	struct raw_mpeg_packet *mpeg = (struct raw_mpeg_packet *)(pck);

	if (mpeg->sync_byte == MPEG_SYNC_BYTE)
	{
		fprintf(stderr, "Warning - first byte seems to be an MPEG sync byte!\n\n");
	}
	
	struct raw_rtp_packet *packet = (struct raw_rtp_packet *) pck;
	fprintf(stderr, "=========== print_rtp_header ============\n");
	fprintf(stderr, "Version:\t\t\t%x\n", packet->VPXCC&RTP_VERSION);
	fprintf(stderr, "Padding:\t\t\t%x\n", packet->VPXCC&RTP_PADDING);
	fprintf(stderr, "Extemsopm:\t\t\t%x\n", packet->VPXCC&RTP_EXTENSION);
	fprintf(stderr, "CSRC count:\t\t\t%x\n", packet->VPXCC&RTP_CSRC_COUNT);
	fprintf(stderr, "Marker:\t\t\t\t%x\n", packet->MPT&RTP_MARKER);
	fprintf(stderr, "Payload type:\t\t\t%x\n", packet->MPT&RTP_PAYLOAD_TYPE);
	fprintf(stderr, "sekvens number:\t\t\t%x\n", ntohs(packet->sequence_number));
	fprintf(stderr, "timestamp:\t\t\t%x\n", ntohl(packet->timestamp));
	fprintf(stderr, "SSRC:\t\t\t\t%x\n", packet->SSRC);
	fprintf(stderr, "=========== print_rtp_header ============\n\n");
}

void print_all_mpeg_header(uint8_t *packet)
{
	int i ;
	for (i = 0; i < 7; i++)
	{
		struct raw_mpeg_packet *mpeg = (struct raw_mpeg_packet *)(packet+188*i);
		if (mpeg->sync_byte != MPEG_SYNC_BYTE)
		{
			fprintf(stderr, "Got byte without syncbyte\n");
			continue;
		}
		print_mpeg_header(mpeg);
	}
}

void print_all_mpeg_header_and_adaption_field(uint8_t *packet)
{
	int i ;
	for (i = 0; i < 7; i++)
	{
		struct raw_mpeg_packet *mpeg = (struct raw_mpeg_packet *)(packet+188*i);
		if (mpeg->sync_byte != MPEG_SYNC_BYTE)
		{
			fprintf(stderr, "Got byte without syncbyte\n");
			continue;
		}
		print_mpeg_header(mpeg);

		if (MPEG_ADAPTATION_FIELD_EXIST_CALC(mpeg->SC_AF_CC) == MPEG_ADAPTION_FIELD_ONLY ||
			 MPEG_ADAPTATION_FIELD_EXIST_CALC(mpeg->SC_AF_CC) == MPEG_ADAPTION_FIELD_AND_PAYLOAD)
		{
			struct raw_adaption_field *adaption = (struct raw_adaption_field*)(packet+4+188*i);
			print_adaptation_field(adaption);
		}
	}
}


void print_if_adaption_field_exist_mpeg_header_and_adaption_field(uint8_t *packet)
{
	int i ;
	for (i = 0; i < 7; i++)
	{
		struct raw_mpeg_packet *mpeg = (struct raw_mpeg_packet *)(packet+188*i);
		if (mpeg->sync_byte != MPEG_SYNC_BYTE)
		{
			fprintf(stderr, "Got byte without syncbyte\n");
			continue;
		}
		if (MPEG_ADAPTATION_FIELD_EXIST_CALC(mpeg->SC_AF_CC) == MPEG_ADAPTION_FIELD_PAYLOAD_ONLY)
		{
			continue;
		}
		print_mpeg_header(mpeg);

		struct raw_adaption_field *adaption = (struct raw_adaption_field*)(packet+4+188*i);
		print_adaptation_field(adaption);
	}
}


void print_if_pcr_exist_mpeg_header_adaption_field_and_pcr(uint8_t *packet)
{
	int i ;
	for (i = 0; i < 7; i++)
	{
		struct raw_mpeg_packet *mpeg = (struct raw_mpeg_packet *)(packet+188*i);
		if (mpeg->sync_byte != MPEG_SYNC_BYTE)
		{
			fprintf(stderr, "Got byte without syncbyte\n");
			continue;
		}
		if (MPEG_ADAPTATION_FIELD_EXIST_CALC(mpeg->SC_AF_CC) == MPEG_ADAPTION_FIELD_PAYLOAD_ONLY)
		{
			continue;
		}
		struct raw_adaption_field *adaption = (struct raw_adaption_field*)(packet+4+188*i);
		if ((adaption->flags&PCR_FLAG)==0)
		{
			continue;
		}
		print_mpeg_header(mpeg);

		print_adaptation_field(adaption);

		unsigned long *pcr = (unsigned long *)(packet+4+2+188*i);
		print_pcr_field(pcr);
	}
}


void print_pcr_if_exist(uint8_t *packet)
{
	int i ;
	for (i = 0; i < 7; i++)
	{
		struct raw_mpeg_packet *mpeg = (struct raw_mpeg_packet *)(packet+188*i);
		if (mpeg->sync_byte != MPEG_SYNC_BYTE)
		{
			fprintf(stderr, "Got byte without syncbyte\n");
			continue;
		}
		if (MPEG_ADAPTATION_FIELD_EXIST_CALC(mpeg->SC_AF_CC) == MPEG_ADAPTION_FIELD_PAYLOAD_ONLY)
		{
			continue;
		}
		struct raw_adaption_field *adaption = (struct raw_adaption_field*)(packet+4+188*i);
		if ((adaption->flags&PCR_FLAG)==0)
		{
			continue;
		}
		unsigned long *pcr = (unsigned long *)(packet+4+2+188*i);
		print_pcr_field(pcr);
	}
}

void print_mpeg_header(struct raw_mpeg_packet *packet)
{
	fprintf(stderr, "=========== print_mpeg_header ============\n");
	fprintf(stderr, "syncbyte:\t\t\t%x\n", packet->sync_byte);
	fprintf(stderr, "Transport Error Indicator:\t%x\n", MPEG_TRANSPORT_ERROR_INDICATOR_CALC(packet->TEI_PUS_TP_PID));
	fprintf(stderr, "Payload Unit Start Indicator:\t%x\n", MPEG_PAYLOAD_UNIT_START_INDICATOR_CALC(packet->TEI_PUS_TP_PID));
	fprintf(stderr, "Transport Priority:\t\t%x\n", MPEG_TRANSPORT_PRIORITY_CALC(packet->TEI_PUS_TP_PID));
	fprintf(stderr, "PID:\t\t\t\t%d\n", MPEG_PID_CALC(packet->TEI_PUS_TP_PID, packet->PID));
	fprintf(stderr, "Scrambling control:\t\t%x\n", MPEG_SCRAMBLING_CONTROL_CALC(packet->SC_AF_CC));
	fprintf(stderr, "Adaptation field exist:\t\t%x\n", MPEG_ADAPTATION_FIELD_EXIST_CALC(packet->SC_AF_CC));
	fprintf(stderr, "Continuity counter:\t\t%x\n", packet->SC_AF_CC&MPEG_CONTINUITY_COUNTER);
	fprintf(stderr, "=========== print_mpeg_header ============\n\n");
}

void print_adaptation_field(struct raw_adaption_field *packet)
{
	fprintf(stderr, "=========== print_adaptation_field ============\n");
	fprintf(stderr, "Adaptation Field Length\t\t\t%x\n", packet->adaption_field_length);
	fprintf(stderr, "Discontinuity indicator\t\t\t%x\n", packet->flags&DISCONTINUITY_INDICATOR);
	fprintf(stderr, "Random Access indicator\t\t\t%x\n", packet->flags&RANDOM_ACCESS_INDICATOR);
	fprintf(stderr, "Elementary stream priority indicator\t%x\n", packet->flags&ELEMENTARY_STREAM_PRIORITY_INDICATOR);
	fprintf(stderr, "PCR flag\t\t\t\t%x\n", packet->flags&PCR_FLAG);
	fprintf(stderr, "OPCR flag\t\t\t\t%x\n", packet->flags&OPCR_FLAG);
	fprintf(stderr, "Splicing point flag\t\t\t%x\n", packet->flags&SPLICING_POINT_FLAG);
	fprintf(stderr, "Transport private data flag:\t\t%x\n", packet->flags&TRANSPORT_PRIVATE_DATA_FLAG);
	fprintf(stderr, "Adaptation field extension flag:\t%x\n", packet->flags&ADAPTATION_FIELD_EXTENSION_FLAG);
	fprintf(stderr, "=========== print_adaptation_field ============\n\n");
}

void print_pcr_field(unsigned long *pcr)
{
	fprintf(stderr, "*pcr %lu pcr->base: %lu\tpcr->extension: %lu time: %lu\n",
		*pcr,
		(unsigned long) PCR_BASE_TO_LONG(*pcr),
		(unsigned long) PCR_EXTENSION_TO_LONG(*pcr),
		time(NULL));
	//ntohs(((struct raw_rtp_packet *)packet)->sequence_number));
}
