#include "jd_services.h"
#include "interfaces/jd_pins.h"
#include "jacdac/dist/c/vibrationmotor.h"

struct srv_state {
    SRV_COMMON;
    uint32_t next_sample;
    uint8_t idx;
    const vibration_motor_api_t *api;
    jd_vibration_motor_vibrate_t
        sequence[JD_SERIAL_PAYLOAD_SIZE / sizeof(jd_vibration_motor_vibrate_t)];
};

static inline int min(int a, int b) {
    return a < b ? a : b;
}

void vibration_process(srv_t *state) {
    if (!jd_should_sample(&state->next_sample, 10000))
        return;

    jd_vibration_motor_vibrate_t *curr = &state->sequence[state->idx];

    if (curr->duration == 0)
        return; // end-of-seq

    if (curr->intensity == 0) {
        // just a pause
        state->next_sample = now + curr->duration * 8000;
        state->idx++;
        return;
    }

    int maxdur = state->api->max_duration / 8;
    int dur = curr->duration;
    if (dur > maxdur)
        dur = maxdur;

    if (state->api->write_amplitude(curr->intensity, dur * 8) == -1) {
        // busy! retry in 16ms
        state->next_sample = now + (16 << 10);
        return;
    }

    if (curr->duration == dur)
        state->idx++;
    else
        curr->duration -= dur;
    state->next_sample = now + dur * 8000;
}

static void handle_vibrate_cmd(srv_t *state, jd_packet_t *pkt) {
    memset(state->sequence, 0, sizeof(state->sequence));
    memcpy(state->sequence, pkt->data, min(sizeof(state->sequence), pkt->service_size));
    state->idx = 0;
}

void vibration_handle_packet(srv_t *state, jd_packet_t *pkt) {
    switch (pkt->service_command) {
    case JD_VIBRATION_MOTOR_CMD_VIBRATE:
        handle_vibrate_cmd(state, pkt);
        break;

    default:
        jd_send_not_implemented(pkt);
        break;
    }
}

SRV_DEF(vibration, JD_SERVICE_CLASS_VIBRATION_MOTOR);
void vibration_motor_init(const vibration_motor_api_t *api) {
    SRV_ALLOC(vibration);
    state->api = api;

    memset(state->sequence, 0x00, sizeof(state->sequence));

    state->api->init();
}