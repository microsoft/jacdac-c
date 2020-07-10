#include "jd_protocol.h"

static jd_frame_t *sendFrame;
static uint8_t bufferPtr, isSending;

int jd_tx_is_idle() {
    return !isSending && sendFrame[bufferPtr].size == 0;
}

void jd_tx_init(void) {
    if (!sendFrame)
        sendFrame = (jd_frame_t *)jd_alloc(sizeof(jd_frame_t) * 2);
}

void *jd_send(unsigned service_num, unsigned service_cmd, const void *data,
               unsigned service_size) {
    void *trg = jd_push_in_frame(&sendFrame[bufferPtr], service_num, service_cmd, service_size);
    if (!trg) {
        JD_LOG("send overflow!");
        return NULL;
    }
    if (data)
        memcpy(trg, data, service_size);
    return trg;
}

void jd_send_event_ext(srv_t *srv, uint32_t eventid, uint32_t arg) {
    srv_common_t *state = (srv_common_t *)srv;
    if (eventid >> 16)
        jd_panic();
    uint32_t data[] = {eventid, arg};
    jd_send(state->service_number, JD_CMD_EVENT, data, 8);
}

// bridge between phys and queue imp, phys calls this to get the next frame.
jd_frame_t *jd_tx_get_frame(void) {
    isSending = true;
    return &sendFrame[bufferPtr ^ 1];
}

// bridge between phys and queue imp, marks as sent.
void jd_tx_frame_sent(jd_frame_t *pkt) {
    isSending = false;
}

void jd_tx_flush() {
    if (isSending)
        return;
    if (sendFrame[bufferPtr].size == 0)
        return;
    sendFrame[bufferPtr].device_identifier = jd_device_id();
    jd_compute_crc(&sendFrame[bufferPtr]);
    bufferPtr ^= 1;
    jd_packet_ready();

    jd_reset_frame(&sendFrame[bufferPtr]);
}
