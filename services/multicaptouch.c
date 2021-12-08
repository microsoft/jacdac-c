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

#define PRESS_THRESHOLD 1500
#define PRESS_TICKS 2

#define SAMPLING_US 500
#define SAMPLE_WINDOW 7

#define SWIPE_DELTA_MIN 0
#define SWIPE_DELTA_MAX 500

typedef struct pin_desc {
    uint8_t pin;
    int8_t ticks_pressed;
    uint32_t start_debounced;
    uint32_t start_press;
    uint32_t end_press;
    uint16_t reading;
} pin_t;

struct srv_state {
    SENSOR_COMMON;
    uint8_t numpins;
    uint8_t num_baseline_samples;
    pin_t *pins;
    int32_t *readings;
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

static void update(srv_t *state) {
    uint32_t now_ms = tim_get_micros() >> 10; // roughly milliseconds

    uint32_t channel_data = ((uint32_t *)state->api->get_reading())[0];

    for (int i = 0; i < state->numpins; ++i) {
        pin_t *p = &state->pins[i];
        
        p->reading = ((channel_data  & (1 << i)) > 0) ? 1 : 0;
        state->readings[i] = p->reading;

        bool was_pressed = p->ticks_pressed >= PRESS_TICKS;

        if (p->reading) {
            p->ticks_pressed++;
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
                p->start_press = now_ms;
                if (now_ms - p->start_debounced > 500)
                    p->start_debounced = p->start_press;
                detect_swipe(state);
            } else {
                p->end_press = now_ms;
                jd_send_event_ext(state, JD_MULTITOUCH_EV_RELEASE, &i, sizeof(i));
#if CON_LOG
                jdcon_log("press p%d %dms", i, now_ms - p->start_press);
#endif
            }
        }
    }
}

void multicaptouch_process(srv_t *state) {
    if (jd_should_sample(&state->nextSample, SAMPLING_US * 9 / 10)) {
        update(state);
        sensor_process_simple(state, state->readings, state->numpins * sizeof(int32_t));
    }
}

void multicaptouch_handle_packet(srv_t *state, jd_packet_t *pkt) {
    sensor_handle_packet_simple(state, pkt, state->readings, state->numpins * sizeof(int32_t));
}

SRV_DEF(multicaptouch, JD_SERVICE_CLASS_MULTITOUCH);

void multicaptouch_init(const captouch_api_t *cfg, uint32_t channels) {
    SRV_ALLOC(multicaptouch);
    tim_max_sleep = SAMPLING_US;
    state->api = cfg;
    state->pins = jd_alloc(channels * sizeof(pin_t));
    state->readings = jd_alloc(channels * sizeof(int32_t));
    state->streaming_interval = 50;
    state->numpins = channels;

    state->api->init();
}
