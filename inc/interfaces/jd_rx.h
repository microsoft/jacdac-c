// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

/*
 * A reception queue for JD packets.
 */

#ifndef JD_RX_H
#define JD_RX_H

#include "jd_service_framework.h"

void jd_rx_init(void);
int jd_rx_frame_received(jd_frame_t *frame);
jd_frame_t *jd_rx_get_frame(void);
void jd_rx_release_frame(jd_frame_t *frame);
bool jd_rx_has_frame(void);

#if JD_CLIENT || JD_BRIDGE
// this will not forward the frame to the USB bridge
int jd_rx_frame_received_loopback(jd_frame_t *frame);
#else
static inline int jd_rx_frame_received_loopback(jd_frame_t *frame) {
    return 0;
}
#endif

#endif
