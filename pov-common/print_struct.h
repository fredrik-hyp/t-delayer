#ifndef __PRINT_STRUCT_H__
#define __PRINT_STRUCT_H__

#include "buffer.h"
#include "prot_rudp.h"
#include "defs.h"

// print_all_* ittereates through a whole 1316 packet and print what it finds
void print_all_mpeg_header(uint8_t *packet);

// --- Adaption fields ---
void print_all_mpeg_header_and_adaption_field(uint8_t *packet);
void print_if_adaption_field_exist_mpeg_header_and_adaption_field(uint8_t *packet);

// --- PCR information ---
void print_if_pcr_exist_mpeg_header_adaption_field_and_pcr(uint8_t *packet);
void print_pcr_if_exist(uint8_t *packet);


// print specific header

void print_bufferelement(bufferelement *buf);
void print_rudpheader(rudpheader *rudp);
void print_rtp_header(uint8_t *packet);

void print_mpeg_header(struct raw_mpeg_packet *packet);
void print_adaptation_field(struct raw_adaption_field *packet);

void print_pcr_field(unsigned long *pcr);


#endif //__PRINT_STRUCT_H__

