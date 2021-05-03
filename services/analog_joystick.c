// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_pins.h"
#include "interfaces/jd_adc.h"
#include "jacdac/dist/c/joystick.h"

struct srv_state {
    SENSOR_COMMON;
    joystick_params_t params;
    uint8_t inited;
    jd_joystick_direction_t direction;
    uint32_t nextSample;
};

REG_DEFINITION(                                 //
    analog_joystick_regs,                       //
    REG_SENSOR_COMMON,                          //
    REG_U32(JD_JOYSTICK_REG_BUTTONS_AVAILABLE), //
    REG_U8(JD_JOYSTICK_REG_VARIANT),            //
)

#define THRESHOLD_SWITCH 0x3000
#define THRESHOLD_KEEP 0x2000

static void update(srv_t *state) {
    uint32_t btns0 = state->direction.buttons;
    uint32_t btns = 0;

    for (unsigned i = 0; i < sizeof(state->params.pinBtns); ++i) {
        if ((1 << i) & state->params.buttons_available) {
            if (pin_get(state->params.pinBtns[i]) == 0) {
                btns |= 1 << i;
            }
        }
    }

    if (state->params.pinX == 0xff) {
        state->direction.x = (btns & JD_JOYSTICK_BUTTONS_LEFT)    ? -0x8000
                             : (btns & JD_JOYSTICK_BUTTONS_RIGHT) ? 0x7fff
                                                                  : 0;
        state->direction.y = (btns & JD_JOYSTICK_BUTTONS_UP)     ? -0x8000
                             : (btns & JD_JOYSTICK_BUTTONS_DOWN) ? 0x7fff
                                                                 : 0;
    } else {
        pin_setup_output(state->params.pinH);
        pin_set(state->params.pinH, 1);
        pin_setup_output(state->params.pinL);
        pin_set(state->params.pinL, 0);

        uint16_t x = adc_read_pin(state->params.pinX) - 2048;
        uint16_t y = adc_read_pin(state->params.pinY) - 2048;

        // save power
        pin_setup_analog_input(state->params.pinH);
        pin_setup_analog_input(state->params.pinL);

        state->direction.x = x << 4;
        state->direction.y = y << 4;

        if (state->direction.x <
            ((btns0 & JD_JOYSTICK_BUTTONS_LEFT) ? -THRESHOLD_KEEP : -THRESHOLD_SWITCH))
            btns |= JD_JOYSTICK_BUTTONS_LEFT;
        if (state->direction.x >
            ((btns0 & JD_JOYSTICK_BUTTONS_RIGHT) ? THRESHOLD_KEEP : THRESHOLD_SWITCH))
            btns |= JD_JOYSTICK_BUTTONS_RIGHT;
        if (state->direction.y <
            ((btns0 & JD_JOYSTICK_BUTTONS_UP) ? -THRESHOLD_KEEP : -THRESHOLD_SWITCH))
            btns |= JD_JOYSTICK_BUTTONS_UP;
        if (state->direction.y >
            ((btns0 & JD_JOYSTICK_BUTTONS_DOWN) ? THRESHOLD_KEEP : THRESHOLD_SWITCH))
            btns |= JD_JOYSTICK_BUTTONS_DOWN;
    }

    if (btns0 != btns) {
        state->direction.buttons = btns;
        jd_send_event_ext(state, JD_JOYSTICK_EV_BUTTONS_CHANGED, &state->direction.buttons, 4);
    }
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

    service_handle_register(state, pkt, analog_joystick_regs);
}

SRV_DEF(analog_joystick, JD_SERVICE_CLASS_JOYSTICK);

void analog_joystick_init(const joystick_params_t *params) {
    SRV_ALLOC(analog_joystick);
    state->params = *params;
}
