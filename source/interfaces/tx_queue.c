// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_protocol.h"

static jd_frame_t *sendFrame;
static uint8_t bufferPtr, isSending;

#if JD_RAW_FRAME
uint8_t rawFrameSending;
jd_frame_t *rawFrame;
#endif

#if JD_SEND_FRAME
static jd_queue_t send_queue;
static uint8_t q_sending;
int jd_send_frame(jd_frame_t *f) {
    if (target_in_irq())
        jd_panic();
    int r = jd_queue_push(send_queue, f);
    if (r)
        ERROR("send_frm ovf");
    jd_packet_ready();
    return r;
}
#endif

static int has_oob_frame(void) {
#if JD_RAW_FRAME
    if (rawFrame || rawFrameSending)
        return 1;
#endif
#if JD_SEND_FRAME
    if (q_sending || jd_queue_front(send_queue))
        return 1;
#endif

    return 0;
}

int jd_tx_is_idle() {
    return !has_oob_frame() && !isSending && sendFrame[bufferPtr].size == 0;
}

void jd_tx_init(void) {
    if (!sendFrame) {
        sendFrame = (jd_frame_t *)jd_alloc(sizeof(jd_frame_t) * 2);
#if JD_SEND_FRAME
        send_queue = jd_queue_alloc(sizeof(jd_frame_t) * 2);
#endif
    }
}

int jd_send(unsigned service_num, unsigned service_cmd, const void *data, unsigned service_size) {
    if (target_in_irq())
        jd_panic();

    void *trg = jd_push_in_frame(&sendFrame[bufferPtr], service_num, service_cmd, service_size);
    if (!trg) {
        ERROR("send ovf");
        return -1;
    }
    if (data)
        memcpy(trg, data, service_size);

    return 0;
}

// bridge between phys and queue imp, phys calls this to get the next frame.
jd_frame_t *jd_tx_get_frame(void) {
#if JD_RAW_FRAME
    if (rawFrame) {
        jd_frame_t *r = rawFrame;
        rawFrame = NULL;
        rawFrameSending = true;
        return r;
    }
#endif
#if JD_SEND_FRAME
    JD_ASSERT(!q_sending);
    jd_frame_t *f = jd_queue_front(send_queue);
    if (f) {
        q_sending = true;
        return f;
    }
#endif
    if (isSending == 1) {
        isSending = 2;
        return &sendFrame[bufferPtr ^ 1];
    }
    return NULL;
}

// bridge between phys and queue imp, marks as sent.
void jd_tx_frame_sent(jd_frame_t *pkt) {
#if JD_RAW_FRAME
    if (rawFrameSending) {
        rawFrameSending = false;
        goto done;
    }
#endif
#if JD_SEND_FRAME
    if (q_sending) {
        jd_queue_shift(send_queue);
        q_sending = false;
        goto done;
    }
#endif

    isSending = 0;
    goto done; // avoid warning

done:
    if (isSending == 1 || has_oob_frame())
        jd_packet_ready();
}

void jd_tx_flush() {
    if (target_in_irq())
        jd_panic();
    if (isSending == 1) {
        // jd_packet_ready();
        return;
    }
    if (sendFrame[bufferPtr].size == 0)
        return;
    sendFrame[bufferPtr].device_identifier = jd_device_id();
    jd_compute_crc(&sendFrame[bufferPtr]);
    bufferPtr ^= 1;
    isSending = 1;
    jd_packet_ready();
    jd_services_packet_queued();

    jd_reset_frame(&sendFrame[bufferPtr]);
}
