// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

/*
 * A transmission queue for JD packets.
 */
#ifndef JD_TX_H
#define JD_TX_H

#include "jd_service_framework.h"

void jd_tx_init(void);
void jd_tx_flush(void);
int jd_tx_is_idle(void);
jd_frame_t *jd_tx_get_frame(void);
void jd_tx_frame_sent(jd_frame_t *frame);

int jd_send(unsigned service_num, unsigned service_cmd, const void *data, unsigned service_size);

// wrappers around jd_send()
int jd_respond_u8(jd_packet_t *pkt, uint8_t v);
int jd_respond_u16(jd_packet_t *pkt, uint16_t v);
int jd_respond_u32(jd_packet_t *pkt, uint32_t v);
int jd_respond_empty(jd_packet_t *pkt);
int jd_respond_string(jd_packet_t *pkt, const char *str);
int jd_send_not_implemented(jd_packet_t *pkt);

/**
 * If pkt contains GET or SET on the given registers, send an error response and return 1.
 * Otherwise return 0.
 */
int jd_block_register(jd_packet_t *pkt, uint16_t reg_code);

void jd_send_event_ext(srv_t *srv, uint32_t eventid, const void *data, uint32_t data_bytes);
static inline void jd_send_event(srv_t *srv, uint32_t eventid) {
    jd_send_event_ext(srv, eventid, 0, 0);
}
void jd_process_event_queue(void);

// this is needed for pipes and clients, not regular servers
// this will send the frame on the wire and on the USB bridge
int jd_send_frame(jd_frame_t *f);
// this will not forward to USB port - it is called from USB code
int jd_send_frame_raw(jd_frame_t *f);

bool jd_tx_will_fit(unsigned size);

// wrapper around jd_send_frame()
int jd_send_pkt(jd_packet_t *pkt);

#if JD_RAW_FRAME
extern uint8_t rawFrameSending;
extern jd_frame_t *rawFrame;
#endif

#endif
