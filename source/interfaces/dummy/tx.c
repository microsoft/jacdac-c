#include "jd_protocol.h"

__attribute__((weak)) int jd_tx_is_idle(void) {
    return 0;
}

__attribute__((weak)) void jd_tx_init(void) {
}

__attribute__((weak)) void *jd_send(unsigned service_num, unsigned service_cmd, const void *data, unsigned service_size) {
    return NULL;
}

__attribute__((weak)) void jd_send_event_ext(srv_t *srv, uint32_t eventid, uint32_t arg) {
}

__attribute__((weak)) jd_frame_t *jd_tx_get_frame(void) {
    return NULL;
}

__attribute__((weak)) void jd_tx_frame_sent(jd_frame_t *pkt) {
}

__attribute__((weak)) void jd_tx_flush(void) {
}
