#include "jdsimple.h"

static jd_frame_t *sendFrame;
static uint8_t bufferPtr, isSending;

int txq_is_idle() {
    return !isSending && sendFrame[bufferPtr].size == 0;
}

void txq_init(void) {
    if (!sendFrame)
        sendFrame = alloc(sizeof(jd_frame_t) * 2);
}

void *txq_push(unsigned service_num, unsigned service_cmd, const void *data,
               unsigned service_size) {
    void *trg = jd_push_in_frame(&sendFrame[bufferPtr], service_num, service_cmd, service_size);
    if (!trg) {
        DMESG("send overflow!");
        return NULL;
    }
    if (data)
        memcpy(trg, data, service_size);
    return trg;
}

void txq_push_event_ex(srv_t *srv, uint32_t eventid, uint32_t arg) {
    srv_common_t *state = (srv_common_t *)srv;
    if (eventid >> 16)
        jd_panic();
    uint32_t data[] = {eventid, arg};
    txq_push(state->service_number, JD_CMD_EVENT, data, 8);
}

jd_frame_t *app_pull_frame(void) {
    isSending = true;
    return &sendFrame[bufferPtr ^ 1];
}

void app_frame_sent(jd_frame_t *pkt) {
    isSending = false;
}

void txq_flush() {
    if (isSending)
        return;
    if (sendFrame[bufferPtr].size == 0)
        return;
    sendFrame[bufferPtr].device_identifier = device_id();
    jd_compute_crc(&sendFrame[bufferPtr]);
    bufferPtr ^= 1;
    jd_packet_ready();

    jd_reset_frame(&sendFrame[bufferPtr]);
}
