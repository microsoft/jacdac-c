// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_protocol.h"

#if JD_RX_QUEUE
static jd_queue_t rx_queue;
#else
static uint8_t occupied;
static jd_frame_t frameToHandle;
#endif

void jd_rx_init(void) {
#if JD_RX_QUEUE
    if (!rx_queue)
        rx_queue = jd_queue_alloc(JD_RX_QUEUE_SIZE);
#endif
}

static int jd_rx_frame_received_core(jd_frame_t *frame, bool is_loop) {
#ifdef JD_SERVICES_PROCESS_FRAME_PRE
    JD_SERVICES_PROCESS_FRAME_PRE(frame);
#endif
    if (!frame)
        return 0;

    if (!is_loop)
        JD_BRIDGE_SEND(frame);

    if (!jd_services_needs_frame(frame))
        return 0;

    if (is_loop)
        frame->flags |= JD_FRAME_FLAG_LOOPBACK;
    else
        frame->flags &= ~JD_FRAME_FLAG_LOOPBACK;

    JD_WAKE_MAIN();

#if JD_RX_QUEUE
    // DMESG("PUSH %x l=%d", frame->crc, is_loop);
    return jd_queue_push(rx_queue, frame);
#else
    if (occupied)
        return -1;
    occupied = 1;
    memcpy(&frameToHandle, frame, JD_FRAME_SIZE(frame));
    return 0;
#endif
}

int jd_rx_frame_received(jd_frame_t *frame) {
    return jd_rx_frame_received_core(frame, 0);
}

#if JD_CLIENT || JD_BRIDGE
int jd_rx_frame_received_loopback(jd_frame_t *frame) {
    return jd_rx_frame_received_core(frame, 1);
}
#endif

jd_frame_t *jd_rx_get_frame(void) {
#if JD_RX_QUEUE
    return jd_queue_front(rx_queue);
#else
    return occupied ? &frameToHandle : NULL;
#endif
}

void jd_rx_release_frame(jd_frame_t *frame) {
#if JD_RX_QUEUE
    jd_queue_shift(rx_queue);
#else
    JD_ASSERT(occupied);
    occupied = 0;
#endif
}

bool jd_rx_has_frame(void) {
#if JD_RX_QUEUE
    return jd_queue_front(rx_queue) != NULL;
#else
    return occupied;
#endif
}