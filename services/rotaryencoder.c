// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_pins.h"
#include "interfaces/jd_pwm.h"
#include "jacdac/dist/c/rotaryencoder.h"

// after any movement, we probe quickly (1000Hz) for 8s, and then go slow (200Hz)
// on G0, in slow mode we take around 100uA and 300uA in fast mode
#define GO_SLOW (8 * 1024 * 1024)
#define PROBE_FAST 1000
#define PROBE_SLOW 5000

struct srv_state {
    SENSOR_COMMON;
    uint8_t state;
    uint8_t pin0, pin1;
    uint8_t pos_shift;
    uint8_t pwm;
    uint16_t clicks_per_turn;
    uint16_t prevpos;
    int32_t sample, position;
    uint32_t nextSample;
    uint32_t go_slow;
};

#ifndef JD_ROTARY_HW
static const int8_t posMap[] = {0, +1, -1, +2, -1, 0, -2, +1, +1, -2, 0, -1, +2, -1, +1, 0};
#endif

static void update(srv_t *state) {
#ifdef JD_ROTARY_HW
    uint16_t newpos = encoder_get(state->pwm);
    state->position += (int16_t)(state->prevpos - newpos);
    state->prevpos = newpos;
#else
    // based on comments in https://github.com/PaulStoffregen/Encoder/blob/master/Encoder.h
    pin_set_pull(state->pin0, PIN_PULL_UP);
    pin_set_pull(state->pin1, PIN_PULL_UP);
    uint16_t s = state->state & 3;
    if (pin_get(state->pin0))
        s |= 4;
    if (pin_get(state->pin1))
        s |= 8;
#if 1
    pin_set_pull(state->pin0, PIN_PULL_NONE);
    pin_set_pull(state->pin1, PIN_PULL_NONE);
#endif

    state->state = (s >> 2);
    if (posMap[s]) {
        tim_max_sleep = PROBE_FAST;
        state->go_slow = now + GO_SLOW;
        state->position += posMap[s];
    }
#endif
    state->sample = state->position >> (state->pos_shift & 0xf);
    if (state->pos_shift & 0x80)
        state->sample = -state->sample;
}

static void maybe_init(srv_t *state) {
    if (sensor_maybe_init(state)) {
#ifdef JD_ROTARY_HW
        state->pwm = encoder_init(state->pin0, state->pin1);
        pin_set_pull(state->pin0, PIN_PULL_UP);
        pin_set_pull(state->pin1, PIN_PULL_UP);
#else
        pin_setup_input(state->pin0, PIN_PULL_NONE);
        pin_setup_input(state->pin1, PIN_PULL_NONE);
#endif
        update(state);
        // make sure we start at 0
        state->position = 0;
        state->sample = 0;
    }
}

void rotaryencoder_process(srv_t *state) {
    maybe_init(state);

    if (jd_should_sample(&state->nextSample, 900) && state->jd_inited) {
        update(state);
#ifndef JD_ROTARY_HW
        if (state->go_slow && in_past(state->go_slow)) {
            tim_max_sleep = PROBE_SLOW;
            state->go_slow = 0;
        }
#endif
    }

    sensor_process_simple(state, &state->sample, sizeof(state->sample));
}

void rotaryencoder_handle_packet(srv_t *state, jd_packet_t *pkt) {
    if (pkt->service_command == JD_GET(JD_ROTARY_ENCODER_REG_CLICKS_PER_TURN))
        jd_send(state->service_index, JD_GET(JD_ROTARY_ENCODER_REG_CLICKS_PER_TURN),
                &(state->clicks_per_turn), sizeof(uint16_t));
    else
        sensor_handle_packet_simple(state, pkt, &state->sample, sizeof(state->sample));
}

SRV_DEF(rotaryencoder, JD_SERVICE_CLASS_ROTARY_ENCODER);

// specify pin0/1 so that clockwise rotations gives higher readings
void rotaryencoder_init(uint8_t pin0, uint8_t pin1, uint16_t clicks_per_turn, uint32_t flags) {
    SRV_ALLOC(rotaryencoder);
    state->pin0 = pin0;
    state->pin1 = pin1;
    state->clicks_per_turn = clicks_per_turn;

    state->pos_shift = flags & ROTARY_ENC_FLAG_DENSE ? 1 : 2;
    if (flags & ROTARY_ENC_FLAG_INVERTED)
        state->pos_shift |= 0x80;

#ifndef JD_ROTARY_HW
    tim_max_sleep = PROBE_SLOW;
#endif
}
