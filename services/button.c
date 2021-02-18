// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_pins.h"
#include "jacdac/dist/c/button.h"

struct srv_state {
    SENSOR_COMMON;
    uint8_t pin;
    uint8_t backlight_pin;
    uint8_t pressed;
    uint8_t prev_pressed;
    uint8_t active;
    uint32_t press_time;
    uint32_t nextSample;
};

static void update(srv_t *state) {
    state->pressed = pin_get(state->pin) == state->active;
    if (state->pressed != state->prev_pressed) {
        state->prev_pressed = state->pressed;
        pin_set(state->backlight_pin, state->pressed);
        if (state->pressed) {
            jd_send_event(state, JD_BUTTON_EV_DOWN);
            state->press_time = now;
        } else {
            jd_send_event(state, JD_BUTTON_EV_UP);
            uint32_t presslen = now - state->press_time;
            if (presslen > 500000)
                jd_send_event(state, JD_BUTTON_EV_LONG_CLICK);
            else
                jd_send_event(state, JD_BUTTON_EV_CLICK);
        }
    }
}

void btn_process(srv_t *state) {
    if (jd_should_sample(&state->nextSample, 9000)) {
        update(state);
    }
}

void btn_handle_packet(srv_t *state, jd_packet_t *pkt) {
    sensor_handle_packet_simple(state, pkt, &state->pressed, sizeof(state->pressed));
}

SRV_DEF(btn, JD_SERVICE_CLASS_BUTTON);

void btn_init(uint8_t pin, bool active, uint8_t backlight_pin) {
    SRV_ALLOC(btn);
    state->pin = pin;
    state->backlight_pin = backlight_pin;
    state->active = active;
    pin_setup_output(backlight_pin);
    pin_setup_input(state->pin, state->active == 0 ? 1 : -1);
    update(state);
}
