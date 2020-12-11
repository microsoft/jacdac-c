// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include "jd_services.h"

void jd_tx_init(void);
void jd_tx_flush(void);
int jd_tx_is_idle(void);
jd_frame_t *jd_tx_get_frame(void);
void jd_tx_frame_sent(jd_frame_t *frame);

int jd_send(unsigned service_num, unsigned service_cmd, const void *data, unsigned service_size);
void jd_send_event_ext(srv_t *srv, uint32_t eventid, const void *data, uint32_t datalen);
static inline void jd_send_event(srv_t *srv, uint32_t eventid) {
    jd_send_event_ext(srv, eventid, 0, 0);
}
void jd_process_event_queue(void);

