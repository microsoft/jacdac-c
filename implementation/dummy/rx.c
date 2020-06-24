#include "jd_config.h"
#include "interfaces/core/jd_rx.h"

__attribute__((weak)) void jd_rx_init(void) {
}

__attribute__((weak)) int jd_rx_frame_received(jd_frame_t *frame) {
}

__attribute__((weak)) jd_frame_t* jd_rx_get_frame(void) {
}