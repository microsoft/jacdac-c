// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_pins.h"
#include "jacdac/dist/c/motion.h"

#define SAMPLING_MS 128 // overkill anyways

struct srv_state {
    SENSOR_COMMON;
    uint8_t moving;
    uint8_t prev_moving;
    uint32_t nextSample;
    const motion_cfg_t *cfg;
};

static void update(srv_t *state) {
    state->moving = pin_get(state->cfg->pin) != state->cfg->inactive;

    if (state->moving != state->prev_moving) {
        state->prev_moving = state->moving;
        if (state->moving) {
            jd_send_event(state, JD_MOTION_EV_MOVEMENT);
        } else {
            jd_send_event(state, JD_EV_INACTIVE);
        }
    }
}

void motion_process(srv_t *state) {
    if (jd_should_sample(&state->nextSample, SAMPLING_MS * 1024)) {
        update(state);
    }
    sensor_process_simple(state, &state->moving, sizeof(state->moving));
}

void motion_handle_packet(srv_t *state, jd_packet_t *pkt) {
    if (service_handle_variant(pkt, state->cfg->variant))
        return;
    sensor_handle_packet_simple(state, pkt, &state->moving, sizeof(state->moving));
}

SRV_DEF(motion, JD_SERVICE_CLASS_MOTION);

void motion_init(const motion_cfg_t *cfg) {
    SRV_ALLOC(motion);
    state->cfg = cfg;
    pin_setup_input(state->cfg->pin, PIN_PULL_NONE);
    update(state);
}
