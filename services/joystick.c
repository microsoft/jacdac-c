// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_pins.h"
#include "interfaces/jd_adc.h"
#include "jacdac/dist/c/joystick.h"

struct srv_state {
    SENSOR_COMMON;
    uint8_t inited;
    uint8_t variant;
    uint8_t pinH, pinL, pinX, pinY;
    jd_joystick_direction_t direction;
    uint32_t nextSample;
};

static void update(srv_t *state) {
    pin_setup_output(state->pinH);
    pin_set(state->pinH, 1);
    pin_setup_output(state->pinL);
    pin_set(state->pinL, 0);

    uint16_t x = adc_read_pin(state->pinX) - 2048;
    uint16_t y = adc_read_pin(state->pinY) - 2048;

    // x = x / (2^15);
    // y = y / (2^15);

    state->direction.x = x << 4;
    state->direction.y = y << 4;

    // save power
    pin_setup_analog_input(state->pinH);
    pin_setup_analog_input(state->pinL);
}

static void maybe_init(srv_t *state) {
    if (state->got_query && !state->inited) {
        state->inited = true;
        update(state);
    }
}

void analog_joystick_process(srv_t *state) {
    maybe_init(state);

    if (jd_should_sample(&state->nextSample, 9000) && state->inited)
        update(state);

    sensor_process_simple(state, &state->direction, sizeof(state->direction));
}

void analog_joystick_handle_packet(srv_t *state, jd_packet_t *pkt) {
    int r = sensor_handle_packet_simple(state, pkt, &state->direction, sizeof(state->direction));

    if (r)
        return;
    
    switch (pkt->service_command) {
        case JD_GET(JD_JOYSTICK_REG_VARIANT):
            jd_send(pkt->service_number, pkt->service_command, &state->variant, 1);
            break;
        case JD_GET(JD_JOYSTICK_REG_DIGITAL):
            jd_send(pkt->service_number, pkt->service_command, 0, 1);
            break;
    }
}

SRV_DEF(analog_joystick, JD_SERVICE_CLASS_JOYSTICK);

void analog_joystick_init(uint8_t pinL, uint8_t pinH, uint8_t pinX, uint8_t pinY, uint8_t variant) {
    SRV_ALLOC(analog_joystick);
    state->pinL = pinL;
    state->pinH = pinH;
    state->pinX = pinX;
    state->pinY = pinY;
    state->variant = variant;
}
