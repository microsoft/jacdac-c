// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_pins.h"
#include "jacdac/dist/c/switch.h"

struct srv_state {
    SENSOR_COMMON;
    uint8_t pressed;
    uint8_t prev_pressed;
    uint8_t variant;
    active_cb_t is_active;
    uint32_t active_state;
};

static void update(srv_t *state) {
    state->pressed = state->is_active(&state->active_state);
    if (state->pressed != state->prev_pressed) {
        state->prev_pressed = state->pressed;
        if (state->pressed) {
            jd_send_event(state, JD_SWITCH_EV_ON);
        } else {
            jd_send_event(state, JD_SWITCH_EV_OFF);
        }
    }
}

void switch_process(srv_t *state) {
    update(state);
    sensor_process_simple(state, &state->pressed, sizeof(state->pressed));
}

void switch_handle_packet(srv_t *state, jd_packet_t *pkt) {
    if (pkt->service_command == JD_GET(JD_SWITCH_REG_VARIANT))
        jd_respond_u8(pkt, state->variant);
    else
        sensor_handle_packet_simple(state, pkt, &state->pressed, sizeof(state->pressed));
}

SRV_DEF(switch, JD_SERVICE_CLASS_SWITCH);

void switch_init(active_cb_t is_active, uint8_t variant) {
    SRV_ALLOC(switch);
    state->variant = variant;
    state->is_active = is_active;
    update(state);
}
