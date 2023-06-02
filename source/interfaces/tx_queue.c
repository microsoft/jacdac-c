// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_protocol.h"

#if JD_SEND_FRAME
static jd_frame_t tx_acc_buffer;
#else
static jd_frame_t *sendFrame;
static uint8_t bufferPtr, isSending;
#define tx_acc_buffer sendFrame[bufferPtr]
#endif

#if JD_RAW_FRAME
uint8_t rawFrameSending;
jd_frame_t *rawFrame;
#endif

#if JD_SEND_FRAME

#if !JD_DEVICESCRIPT
bool jd_need_to_send(jd_frame_t *f) {
#if JD_USB_BRIDGE || JD_NET_BRIDGE
    if ((f->flags & JD_FRAME_FLAG_COMMAND) && f->device_identifier == jd_device_id())
        return false;

    return true;
#else
    // when no USB and no network, there is no reason to filter packets
    // to be sent
    return true;
#endif
}
#endif

static jd_queue_t send_queue;
static uint8_t q_sending;
int jd_send_frame_raw(jd_frame_t *f) {
    int r = 0;

    if (jd_need_to_send(f)) {
        // put in sendQ first
        r = jd_queue_push(send_queue, f);
        if (r)
            OVF_ERROR("frm send ovf");
    }

    // this may modify flags
    if (jd_rx_frame_received_loopback(f))
        OVF_ERROR("loopback rx ovf");

    jd_packet_ready();

    return r;
}
int jd_send_frame(jd_frame_t *f) {
    JD_BRIDGE_SEND(f);
    return jd_send_frame_raw(f);
}
static int jd_send_frame_with_crc(jd_frame_t *f) {
    f->device_identifier = jd_device_id();
    jd_compute_crc(f);
    return jd_send_frame(f);
}

bool jd_tx_will_fit(unsigned size) {
    return jd_queue_will_fit(send_queue, size);
}
#endif

int jd_tx_is_idle(void) {
#if JD_RAW_FRAME
    if (rawFrame || rawFrameSending)
        return 0;
#endif
#if JD_SEND_FRAME
    if (q_sending || jd_queue_front(send_queue))
        return 0;
#else
    if (isSending)
        return 0;
#endif

    if (tx_acc_buffer.size != 0)
        return 0;

    return 1;
}

void jd_tx_init(void) {
#if JD_SEND_FRAME
    if (!send_queue)
        send_queue = jd_queue_alloc(JD_SEND_FRAME_SIZE);
#else
    if (!sendFrame)
        sendFrame = (jd_frame_t *)jd_alloc(sizeof(jd_frame_t) * 2);
#endif
}

int jd_send(unsigned service_num, unsigned service_cmd, const void *data, unsigned service_size) {
    if (target_in_irq())
        JD_PANIC();

    void *trg = jd_push_in_frame(&tx_acc_buffer, service_num, service_cmd, service_size);
    if (!trg) {
#if JD_SEND_FRAME
        jd_send_frame_with_crc(&tx_acc_buffer);
        jd_reset_frame(&tx_acc_buffer);
        trg = jd_push_in_frame(&tx_acc_buffer, service_num, service_cmd, service_size);
        JD_ASSERT(trg != NULL);
#else
        OVF_ERROR("send ovf");
        return -1;
#endif
    }

    if (service_size > 0) {
        JD_ASSERT(data != NULL);
        memcpy(trg, data, service_size);
    }

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
#else
    if (isSending == 1) {
        isSending = 2;
        return &sendFrame[bufferPtr ^ 1];
    }
#endif
    return NULL;
}

static void poke_ready(void) {
#if JD_RAW_FRAME
    if (rawFrame)
        jd_packet_ready();
#endif
#if JD_SEND_FRAME
    if (jd_queue_front(send_queue))
        jd_packet_ready();
#else
    if (isSending)
        jd_packet_ready();
#endif
}

// bridge between phys and queue imp, marks as sent.
void jd_tx_frame_sent(jd_frame_t *pkt) {
#if JD_RAW_FRAME
    if (rawFrameSending) {
        rawFrameSending = false;
        poke_ready();
        return;
    }
#endif

#if JD_SEND_FRAME
    JD_ASSERT(q_sending);
    jd_queue_shift(send_queue);
    q_sending = false;
#else
    JD_ASSERT(isSending == 2);
    isSending = 0;
#endif

    poke_ready();
}

void jd_tx_flush(void) {
    if (target_in_irq())
        JD_PANIC();
    jd_frame_t *f = &tx_acc_buffer;
    if (f->size == 0)
        return;
#if !JD_SEND_FRAME
    if (isSending)
        return;
#endif
#if JD_SEND_FRAME
    jd_send_frame_with_crc(f);
#else
    f->device_identifier = jd_device_id();
    jd_compute_crc(f);

    bufferPtr ^= 1;
    isSending = 1;
    jd_packet_ready();
#endif
    jd_services_packet_queued();

    jd_reset_frame(&tx_acc_buffer);
}
