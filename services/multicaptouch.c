// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_pins.h"
#include "interfaces/jd_adc.h"
#include "jd_console.h"
#include "jacdac/dist/c/multitouch.h"

#define PIN_LOG 0
#ifdef JD_CONSOLE
#define CON_LOG 1
#else
#define CON_LOG 0
#endif

/* Once you define SAVE_HISTORY, run the module and do the touch gestures.
 * Then run 'make gdb' and inside gdb run:
 * (gdb) dump memory history.bin history (history+history_ptr+2)
 * then from shell run:
 * $ node jacdac-c/scripts/decode_multitouch_history.js history.bin
 * This will show the history of touches. Every dash is SAMPLING_US.
 */

// #define SAVE_HISTORY 1

#define PRESS_THRESHOLD 1500
#define PRESS_TICKS 1

#define SAMPLING_US 33000 // cap1298 has 35ms sampling cycle

#define SWIPE_DELTA_MIN 0
#define SWIPE_DELTA_MAX 500

typedef struct pin_desc {
    uint8_t pin;
    int8_t ticks_pressed;
    uint32_t start_debounced;
    uint32_t start_press;
    uint32_t end_press;
} pin_t;

struct srv_state {
    SENSOR_COMMON;
    uint8_t numpins;
    uint8_t num_baseline_samples;
    pin_t *pins;
    uint16_t *readings;
    uint32_t nextSample;
    uint32_t next_baseline_sample;
};

static void detect_swipe(srv_t *state) {
    if (state->numpins < 3)
        return;

    int delta = 0;
    for (int i = 1; i < state->numpins; ++i) {
        pin_t *p = &state->pins[i];
        pin_t *p0 = &state->pins[i - 1];

        int d0 = p->start_debounced - p0->start_debounced;
        if (d0 < 0) {
            delta--;
            d0 = -d0;
        } else {
            if (d0)
                delta++;
        }

        if (!(SWIPE_DELTA_MIN <= d0 && d0 <= SWIPE_DELTA_MAX))
            return;
    }

#if PIN_LOG
    pin_pulse(LOG_SW, 1);
#endif
    for (int i = 0; i < state->numpins; ++i) {
        pin_t *p = &state->pins[i];
        p->start_debounced = 0;
    }

    if (delta > -2 && delta < 2)
        return;

    if (delta > 0)
        jd_send_event(state, JD_MULTITOUCH_EV_SWIPE_POS);
    else
        jd_send_event(state, JD_MULTITOUCH_EV_SWIPE_NEG);

#if CON_LOG
    jdcon_warn("swp %d", delta);
#endif
}

#ifdef SAVE_HISTORY
uint8_t history[1024];
uint32_t history_ptr;
#endif

static void update(srv_t *state) {
    uint32_t nowms = tim_get_micros() >> 10; // roughly milliseconds

    uint16_t *channel_data = state->api->get_reading();

    uint32_t mask = 0;

    for (int i = 0; i < state->numpins; ++i) {
        pin_t *p = &state->pins[i];

        uint16_t reading = channel_data[i];
        state->readings[i] = reading;

        bool was_pressed = p->ticks_pressed >= PRESS_TICKS;

        if (reading > 100) {
            p->ticks_pressed++;
            mask |= (1 << i);
        } else {
            p->ticks_pressed--;
        }
        if (p->ticks_pressed > PRESS_TICKS * 2)
            p->ticks_pressed = PRESS_TICKS * 2;
        else if (p->ticks_pressed < 0)
            p->ticks_pressed = 0;

        bool is_pressed = p->ticks_pressed >= PRESS_TICKS;
#if PIN_LOG
        pin_set(logpins[i], is_pressed);
#endif
        if (is_pressed != was_pressed) {
            if (is_pressed) {
                jd_send_event_ext(state, JD_MULTITOUCH_EV_TOUCH, &i, sizeof(i));
                p->start_press = nowms;
                if (nowms - p->start_debounced > 500)
                    p->start_debounced = p->start_press;
                detect_swipe(state);
            } else {
                p->end_press = nowms;
                jd_send_event_ext(state, JD_MULTITOUCH_EV_RELEASE, &i, sizeof(i));
#if CON_LOG
                jdcon_log("press p%d %dms", i, nowms - p->start_press);
#endif
            }
        }
    }

#ifdef SAVE_HISTORY
    mask &= 0xff;
    if (history[history_ptr] != mask) {
        history_ptr += 2;
        if (history_ptr >= sizeof(history)) {
            history_ptr = 0;
        }
        history[history_ptr] = mask;
        history[history_ptr + 1] = 1;
    } else {
        if ((uint8_t)++history[history_ptr + 1] == 0)
            history[history_ptr + 1] = 0xff;
    }
#endif
}

void multicaptouch_process(srv_t *state) {
    if (jd_should_sample_delay(&state->nextSample, SAMPLING_US * 9 / 10)) {
        update(state);
        sensor_process_simple(state, state->readings, state->numpins * sizeof(uint16_t));
    }
}

void multicaptouch_handle_packet(srv_t *state, jd_packet_t *pkt) {
    sensor_handle_packet_simple(state, pkt, state->readings, state->numpins * sizeof(uint16_t));
}

SRV_DEF(multicaptouch, JD_SERVICE_CLASS_MULTITOUCH);

void multicaptouch_init(const captouch_api_t *cfg, uint32_t channels) {
    SRV_ALLOC(multicaptouch);
    state->api = cfg;
    state->pins = jd_alloc(channels * sizeof(pin_t));
    state->readings = jd_alloc(channels * sizeof(uint16_t));
    state->streaming_interval = 50;
    state->numpins = channels;

    state->api->init();
}
