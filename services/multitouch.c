#include "jd_protocol.h"
#include "interfaces/jd_sensor.h"
#include "interfaces/jd_pins.h"
#include "interfaces/jd_adc.h"

#define EVT_DOWN 1
#define EVT_UP 2
#define EVT_CLICK 3
#define EVT_LONG_CLICK 4

#define PRESS_THRESHOLD 150

#define SAMPLING_US 1000
#define AVG_SAMPLES 45

typedef struct pin_desc {
    uint8_t pin;
    uint8_t ticks_pressed;
    uint8_t avg_samples;
    uint16_t reading;
    uint16_t baseline;
    uint32_t avg_sum;
} pin_t;

struct srv_state {
    SENSOR_COMMON;
    uint8_t numpins;
    pin_t *pins;
    int32_t *readings;
    uint32_t nextSample;
};

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

static void sort(uint16_t *a, int n) {
    for (int i = 1; i < n; i++)
        for (int j = i; j > 0; j--)
            if (a[j] < a[j - 1]) {
                uint16_t tmp = a[j];
                a[j] = a[j - 1];
                a[j - 1] = tmp;
            }
}

static uint16_t read_pin_avg(uint8_t pin) {
    int n = 5;
    uint16_t arr[n];
    for (int i = 0; i < n; ++i) {
        arr[i] = read_pin(pin);
    }
    sort(arr, n);
    return arr[n >> 1];
}

static void update(srv_t *state) {
    for (int i = 0; i < state->numpins; ++i) {
        pin_t *p = &state->pins[i];
        p->reading = read_pin_avg(p->pin);
        p->avg_sum += p->reading;
        p->avg_samples++;
        if (p->avg_samples >= AVG_SAMPLES) {
            state->readings[i] = p->avg_sum / p->avg_samples;
            p->avg_sum = 0;
            p->avg_samples = 0;
        }
    }
}

static void calibrate(srv_t *state) {
    for (int i = 0; i < state->numpins; ++i) {
        pin_t *p = &state->pins[i];
        p->baseline = read_pin_avg(p->pin);
    }
    update(state);
}

void multitouch_process(srv_t *state) {
    if (jd_should_sample(&state->nextSample, SAMPLING_US * 9 / 10)) {
        update(state);
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
        pin_setup_input(pins[i], 0);
    }

    state->streaming_interval = 50;

    calibrate(state);
}
