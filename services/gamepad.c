// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_pins.h"
#include "interfaces/jd_adc.h"
#include "jacdac/dist/c/gamepad.h"

struct srv_state {
    SENSOR_COMMON;
    gamepad_params_t params;
    jd_gamepad_direction_t direction;
    uint32_t nextSample;
};

REG_DEFINITION(                                //
    gamepad_regs,                              //
    REG_SENSOR_COMMON,                         //
    REG_U32(JD_GAMEPAD_REG_BUTTONS_AVAILABLE), //
    REG_U8(JD_GAMEPAD_REG_VARIANT),            //
)

#define THRESHOLD_SWITCH 0x3000
#define THRESHOLD_KEEP 0x2000

static void update(srv_t *state) {
    uint32_t btns0 = state->direction.buttons;
    uint32_t btns = 0;

    for (unsigned i = 0; i < sizeof(state->params.pinBtns); ++i) {
        if ((1 << i) & state->params.buttons_available) {
            if (pin_get(state->params.pinBtns[i]) == state->params.active_level) {
                btns |= 1 << i;
            }
        }
    }

    if (state->params.pinX == 0xff) {
        state->direction.x = (btns & JD_GAMEPAD_BUTTONS_LEFT)    ? -0x8000
                             : (btns & JD_GAMEPAD_BUTTONS_RIGHT) ? 0x7fff
                                                                 : 0;
        state->direction.y = (btns & JD_GAMEPAD_BUTTONS_UP)     ? -0x8000
                             : (btns & JD_GAMEPAD_BUTTONS_DOWN) ? 0x7fff
                                                                : 0;
    } else {
        pin_setup_output(state->params.pinH);
        pin_set(state->params.pinH, 1);
        pin_setup_output(state->params.pinL);
        pin_set(state->params.pinL, 0);

        int x = adc_read_pin(state->params.pinX) - 0x8000;
        int y = adc_read_pin(state->params.pinY) - 0x8000;

        // save power
        pin_setup_analog_input(state->params.pinH);
        pin_setup_analog_input(state->params.pinL);

        state->direction.x = x;
        state->direction.y = y;

        if (state->direction.x <
            ((btns0 & JD_GAMEPAD_BUTTONS_LEFT) ? -THRESHOLD_KEEP : -THRESHOLD_SWITCH))
            btns |= JD_GAMEPAD_BUTTONS_LEFT;
        if (state->direction.x >
            ((btns0 & JD_GAMEPAD_BUTTONS_RIGHT) ? THRESHOLD_KEEP : THRESHOLD_SWITCH))
            btns |= JD_GAMEPAD_BUTTONS_RIGHT;
        if (state->direction.y <
            ((btns0 & JD_GAMEPAD_BUTTONS_UP) ? -THRESHOLD_KEEP : -THRESHOLD_SWITCH))
            btns |= JD_GAMEPAD_BUTTONS_UP;
        if (state->direction.y >
            ((btns0 & JD_GAMEPAD_BUTTONS_DOWN) ? THRESHOLD_KEEP : THRESHOLD_SWITCH))
            btns |= JD_GAMEPAD_BUTTONS_DOWN;
    }

    if (btns0 != btns) {
        state->direction.buttons = btns;
        jd_send_event_ext(state, JD_GAMEPAD_EV_BUTTONS_CHANGED, &state->direction.buttons, 4);

        for (unsigned i = 0; i < sizeof(state->params.pinLeds); ++i) {
            if ((1 << i) & state->params.buttons_available) {
                pin_set(state->params.pinLeds[i], btns & (1 << i) ? 0 : 1);
            }
        }
    }
}

static void maybe_init(srv_t *state) {
    if (sensor_maybe_init(state)) {
        for (unsigned i = 0; i < sizeof(state->params.pinBtns); ++i) {
            if ((1 << i) & state->params.buttons_available) {
                pin_setup_input(state->params.pinBtns[i],
                                state->params.active_level ? PIN_PULL_DOWN : PIN_PULL_UP);
                pin_setup_output(state->params.pinLeds[i]);
            }
        }
        update(state);
    }
}

void gamepad_process(srv_t *state) {
    maybe_init(state);

    if (jd_should_sample(&state->nextSample, 9000) && state->jd_inited)
        update(state);

    sensor_process_simple(state, &state->direction, sizeof(state->direction));
}

void gamepad_handle_packet(srv_t *state, jd_packet_t *pkt) {
    if (service_handle_register(state, pkt, gamepad_regs))
        return;
    sensor_handle_packet_simple(state, pkt, &state->direction, sizeof(state->direction));
}

SRV_DEF(gamepad, JD_SERVICE_CLASS_GAMEPAD);

void gamepad_init(const gamepad_params_t *params) {
    SRV_ALLOC(gamepad);
    state->params = *params;

    for (unsigned i = 0; i < sizeof(state->params.pinBtns); ++i) {
        if ((1 << i) & state->params.buttons_available) {
            if (state->params.pinBtns[i] == NO_PIN) {
                // not connected? clear the flag
                state->params.buttons_available &= ~(1 << i);
            }
        }
    }
}

#if JD_DCFG
// these are synced with enum in gamedpad.md!
static const char *btns[] = { //
    "pinLeft",   "pinUp",    "pinRight", "pinDown", "pinA", "pinB", "pinMenu",
    "pinSelect", "pinReset", "pinExit",  "pinX",    "pinY", NULL};
void gamepad_config(void) {
    gamepad_params_t *p = jd_alloc(sizeof(*p));

    for (int i = 0; btns[i]; ++i) {
        uint8_t pin = jd_srvcfg_pin(btns[i]);
        if (pin != NO_PIN) {
            p->buttons_available |= (1 << i);
            p->pinBtns[i] = pin;
            p->pinLeds[i] = 0xff;
        }
    }

    p->pinX = jd_srvcfg_pin("pinAX");
    p->pinY = jd_srvcfg_pin("pinAY");
    p->pinL = jd_srvcfg_pin("pinLow");
    p->pinH = jd_srvcfg_pin("pinHigh");

    if (jd_srvcfg_has_flag("activeHigh"))
        p->active_level = 1;

    gamepad_init(p);
}
#endif
