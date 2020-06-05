#include "jdsimple.h"

static jd_frame_t *frameToHandle;

int app_handle_frame(jd_frame_t *frame) {
    if (frameToHandle)
        return -1;
    frameToHandle = frame;
    return 0;
}

void app_process_frame() {
    if (frameToHandle) {
        if (frameToHandle->flags & JD_FRAME_FLAG_ACK_REQUESTED &&
            frameToHandle->flags & JD_FRAME_FLAG_COMMAND &&
            frameToHandle->device_identifier == device_id())
            txq_push(JD_SERVICE_NUMBER_CRC_ACK, frameToHandle->crc, NULL, 0);

        for (;;) {
            app_handle_packet((jd_packet_t *)frameToHandle);
            if (!jd_shift_frame(frameToHandle))
                break;
        }

        frameToHandle = NULL;
    }
}
