#include "jd_config.h"
#include "interfaces/jd_rx.h"

static jd_frame_t *frameToHandle;

void jd_rx_init(void) {
}

int jd_rx_frame_received(jd_frame_t *frame) {
#ifdef JD_SERVICES_PROCESS_FRAME_PRE
    JD_SERVICES_PROCESS_FRAME_PRE(frame);
#endif
    if (!frame)
        return 0;

    int ret = frameToHandle ? -1 : 0;

    // always takes latest frame - the old one might just get overwritten
    frameToHandle = frame;

    return ret;
}

jd_frame_t* jd_rx_get_frame(void) {
    jd_frame_t* f = NULL;

    if (frameToHandle) {
        f = frameToHandle;
        frameToHandle = NULL;
    }

    return f;
}