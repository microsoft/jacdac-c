#include "jd_protocol.h"

__attribute__((weak)) int jd_tx_is_idle() {
    return 0;
}

__attribute__((weak)) void jd_tx_init(void) {
}

__attribute__((weak)) void *jd_send(unsigned, unsigned, const void *,
               unsigned) {
    return NULL;
}

__attribute__((weak)) void jd_send_event_ext(srv_t *, uint32_t, uint32_t) {
}

__attribute__((weak)) jd_frame_t *jd_tx_get_frame(void) {
    return NULL;
}

__attribute__((weak)) void jd_tx_frame_sent(jd_frame_t *pkt) {
}

__attribute__((weak)) void jd_tx_flush() {
}
