// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#if 0

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

#define PRESS_THRESHOLD (70 * 16)
#define PRESS_TICKS 2

#define SAMPLING_US 500
#define SAMPLE_WINDOW 7

#define BASELINE_SAMPLES 20
#define BASELINE_SUPER_SAMPLES 10
#define BASELINE_FREQ (1000000 / BASELINE_SAMPLES)

#define SWIPE_DELTA_MIN 10
#define SWIPE_DELTA_MAX 150

#if PIN_LOG
#include "pinnames.h"
#define LOG_SW PA_10
const uint8_t logpins[] = {PA_5, PA_6, PA_7, PA_9};
#endif

typedef struct pin_desc {
    uint8_t pin;
    int8_t ticks_pressed;
    uint32_t start_debounced;
    uint32_t start_press;
    uint32_t end_press;
    uint16_t reading;
    uint16_t readings[SAMPLE_WINDOW];
    uint16_t baseline_samples[BASELINE_SAMPLES];
    uint16_t baseline_super_samples[BASELINE_SUPER_SAMPLES];
    uint16_t baseline;
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

static void sort(uint16_t *a, int n) {
    for (int i = 1; i < n; i++)
        for (int j = i; j > 0; j--)
            if (a[j] < a[j - 1]) {
                uint16_t tmp = a[j];
                a[j] = a[j - 1];
                a[j - 1] = tmp;
            }
}

static uint16_t add_sample(uint16_t *samples, uint8_t len, uint16_t sample) {
    uint16_t rcopy[len];
    memcpy(rcopy, samples + 1, (len - 1) * 2);
    rcopy[len - 1] = sample;
    memcpy(samples, rcopy, len * 2);
    sort(rcopy, len);
    return rcopy[len >> 1];
}

static uint16_t read_pin(uint8_t pin) {
    for (;;) {
        pin_set(pin, 1);
        uint64_t t0 = tim_get_micros();
        pin_setup_output(pin);
        pin_setup_analog_input(pin);
        target_wait_us(50);
        int r = adc_read_pin(pin);
        int dur = tim_get_micros() - t0;
        // only return results from runs not interrupted
        if (dur < 1800)
            return r;
    }
}

static uint16_t read_pin_avg(uint8_t pin) {
    int n = 3;
    uint16_t arr[n];
    for (int i = 0; i < n; ++i) {
        arr[i] = read_pin(pin);
    }
    sort(arr, n);
    return arr[n >> 1];
}

static void detect_swipe(srv_t *state) {
    if (state->numpins < 3)
        return;

    int delta = 0;
    for (int i = 1; i < state->numpins; ++i) {
        pin_t *p = &state->pins[i];
        pin_t *p0 = &state->pins[i - 1];
        if (p->start_debounced == 0 || p0->start_debounced == 0)
            return;

        int d0 = p->start_debounced - p0->start_debounced;
        if (d0 < 0) {
            if (delta > 0)
                return;
            delta = -1;
            d0 = -d0;
        } else {
            if (delta < 0)
                return;
            delta = 1;
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

    for (int i = 0; i < state->numpins; ++i) {
        pin_t *p = &state->pins[i];
        p->reading = add_sample(p->readings, SAMPLE_WINDOW, read_pin_avg(p->pin));
        state->readings[i] = p->reading - p->baseline;

        bool was_pressed = p->ticks_pressed >= PRESS_TICKS;

        if (p->reading - p->baseline > PRESS_THRESHOLD) {
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

static void update_baseline(srv_t *state) {
    state->num_baseline_samples++;
    if (state->num_baseline_samples >= BASELINE_SAMPLES)
        state->num_baseline_samples = 0;
    for (int i = 0; i < state->numpins; ++i) {
        pin_t *p = &state->pins[i];
        uint16_t tmp = add_sample(p->baseline_samples, BASELINE_SAMPLES, p->reading);
        if (state->num_baseline_samples == 0) {
            p->baseline = add_sample(p->baseline_super_samples, BASELINE_SUPER_SAMPLES, tmp);
#if CON_LOG
            if (i == 1)
                jdcon_log("re-calib: %d %d", state->pins[0].baseline, state->pins[1].baseline);
#endif
        }
    }
}

static void calibrate(srv_t *state) {
    for (int k = 0; k < BASELINE_SUPER_SAMPLES; ++k) {
        for (int j = 0; j < BASELINE_SAMPLES; ++j) {
            for (int i = 0; i < state->numpins; ++i) {
                pin_t *p = &state->pins[i];
                p->reading = read_pin_avg(p->pin);
            }
            update_baseline(state);
        }
        target_wait_us(100);
    }
    DMESG("calib: %d", state->pins[0].baseline);
}

void multitouch_process(srv_t *state) {
    if (jd_should_sample(&state->nextSample, SAMPLING_US * 9 / 10)) {
        update(state);
        if (jd_should_sample(&state->next_baseline_sample, BASELINE_FREQ))
            update_baseline(state);
        sensor_process_simple(state, state->readings, state->numpins * sizeof(int32_t));
    }
}

void multitouch_handle_packet(srv_t *state, jd_packet_t *pkt) {
    sensor_handle_packet_simple(state, pkt, state->readings, state->numpins * sizeof(int32_t));
}

SRV_DEF(multitouch, JD_SERVICE_CLASS_MULTITOUCH);

void multitouch_init(const uint8_t *pins) {
    SRV_ALLOC(multitouch);

    tim_max_sleep = SAMPLING_US;

    while (pins[state->numpins] != 0xff)
        state->numpins++;
    state->pins = jd_alloc(state->numpins * sizeof(pin_t));
    state->readings = jd_alloc(state->numpins * sizeof(int32_t));

    for (int i = 0; i < state->numpins; ++i) {
        state->pins[i].pin = pins[i];
        pin_setup_input(pins[i], PIN_PULL_NONE);
#if PIN_LOG
        pin_setup_output(logpins[i]);
        pin_setup_output(LOG_SW);
#endif
    }

    state->streaming_interval = 50;

    calibrate(state);
}

#endif