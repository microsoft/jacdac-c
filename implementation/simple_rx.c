#include "jd_config.h"
#include "interfaces/core/jd_rx.h"

static jd_frame_t *frameToHandle;

void jd_rx_init(void) {
}

int jd_rx_frame_received(jd_frame_t *frame) {
    if (frameToHandle)
        return -1;

    frameToHandle = frame;

    return 0;
}

jd_frame_t* jd_rx_get_frame(void) {
    jd_frame_t* f = NULL;

    if (frameToHandle) {
        f = frameToHandle;
        frameToHandle = NULL;
    }

    return f;
}