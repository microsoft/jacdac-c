// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_protocol.h"

static uint8_t occupied;
static jd_frame_t frameToHandle;

void jd_rx_init(void) {}

int jd_rx_frame_received(jd_frame_t *frame) {
#ifdef JD_SERVICES_PROCESS_FRAME_PRE
    JD_SERVICES_PROCESS_FRAME_PRE(frame);
#endif
    if (!frame)
        return 0;

    if (occupied)
        return -1;

    occupied = 1;
    memcpy(&frameToHandle, frame, JD_FRAME_SIZE(frame));
    return 0;
}

#if JD_CLIENT
int jd_rx_frame_received_loopback(jd_frame_t *frame) {
    return jd_rx_frame_received(frame);
}
#endif

jd_frame_t *jd_rx_get_frame(void) {
    return occupied ? &frameToHandle : NULL;
}

void jd_rx_release_frame(jd_frame_t *frame) {
    JD_ASSERT(occupied);
    occupied = 0;
}