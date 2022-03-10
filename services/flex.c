// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_pins.h"
#include "interfaces/jd_adc.h"
#include "jacdac/dist/c/flex.h"

struct srv_state {
    SENSOR_COMMON;
    uint8_t pinH, pinL, pinM;
    uint16_t sample;
    uint32_t nextSample;
};

static void update(srv_t *state) {
    pin_setup_output(state->pinH);
    pin_set(state->pinH, 1);
    pin_setup_output(state->pinL);
    pin_set(state->pinL, 0);

    state->sample = adc_read_pin(state->pinM);

    // save power
    pin_setup_analog_input(state->pinH);
    pin_setup_analog_input(state->pinL);
}

static void maybe_init(srv_t *state) {
    if (sensor_maybe_init(state))
        update(state);
}

void flex_process(srv_t *state) {
    maybe_init(state);

    if (jd_should_sample(&state->nextSample, 9000) && state->jd_inited)
        update(state);

    sensor_process_simple(state, &state->sample, sizeof(state->sample));
}

void flex_handle_packet(srv_t *state, jd_packet_t *pkt) {
    sensor_handle_packet_simple(state, pkt, &state->sample, sizeof(state->sample));
}

SRV_DEF(flex, JD_SERVICE_CLASS_FLEX);

void flex_init(uint8_t pinL, uint8_t pinM, uint8_t pinH) {
    SRV_ALLOC(flex);
    state->pinL = pinL;
    state->pinM = pinM;
    state->pinH = pinH;
}
