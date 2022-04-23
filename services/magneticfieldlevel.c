// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_pins.h"
#include "interfaces/jd_adc.h"
#include "jacdac/dist/c/magneticfieldlevel.h"

struct srv_state {
    SENSOR_COMMON;
    int16_t value;
    int8_t detected;
    int8_t prev_detected;
    uint32_t nextSample;
    const magneticfieldlevel_cfg_t *cfg;
};

static void update(srv_t *state) {
    const magneticfieldlevel_cfg_t *cfg = state->cfg;
    int val = adc_read_pin(cfg->pin);
    val -= 0x8000;
    if (cfg->inverted)
        val = -val;
    val += cfg->offset;
    if (val < -0x8000)
        val = -0x8000;
    if (val > 0x7fff)
        val = 0x7fff;
    state->value = val;

    if (val < (state->prev_detected < 0 ? cfg->thr_south_off : cfg->thr_south_on))
        state->detected = -1;
    else if (val > (state->prev_detected > 0 ? cfg->thr_north_off : cfg->thr_north_on))
        state->detected = 1;
    else
        state->detected = 0;

    if (state->detected != state->prev_detected) {
        state->prev_detected = state->detected;
        if (state->detected) {
            jd_send_event(state, JD_EV_ACTIVE);
        } else {
            jd_send_event(state, JD_EV_INACTIVE);
        }
    }
}

void magneticfieldlevel_process(srv_t *state) {
    if (jd_should_sample(&state->nextSample, 9000)) {
        update(state);
    }
    sensor_process_simple(state, &state->value, sizeof(state->value));
}

void magneticfieldlevel_handle_packet(srv_t *state, jd_packet_t *pkt) {
    if (service_handle_variant(pkt, state->cfg->variant))
        return;
    sensor_handle_packet_simple(state, pkt, &state->value, sizeof(state->value));
}

SRV_DEF(magneticfieldlevel, JD_SERVICE_CLASS_MAGNETIC_FIELD_LEVEL);
void magneticfieldlevel_init(const magneticfieldlevel_cfg_t *cfg) {
    SRV_ALLOC(magneticfieldlevel);
    state->cfg = cfg;
    pin_setup_analog_input(state->cfg->pin);
    update(state);
}
