// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

/*
 * A transmission queue for JD packets.
 */

#pragma once

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
int jd_send_not_implemented(jd_packet_t *pkt);

void jd_send_event_ext(srv_t *srv, uint32_t eventid, const void *data, uint32_t data_bytes);
static inline void jd_send_event(srv_t *srv, uint32_t eventid) {
    jd_send_event_ext(srv, eventid, 0, 0);
}
void jd_process_event_queue(void);
