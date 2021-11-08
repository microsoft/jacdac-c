// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_pins.h"
#include "interfaces/jd_adc.h"
#include "jacdac/dist/c/power.h"

// this is using outdated service spec
#if 0

#define LOG JD_LOG
// #define LOG JD_NOLOG

#define CHECK_PERIOD 1000 // how often to probe the ADC, in us
#define OVERLOAD_MS 1000  // how long to shut down the power for after overload, in ms

// calibrate readings
#define MA_SCALE 1890
#define MV_SCALE 419

#define READING_WINDOW 3

struct srv_state {
    SENSOR_COMMON;
    uint8_t intensity;
    uint8_t overload;
    uint16_t max_power;
    uint16_t curr_power;
    uint16_t battery_voltage;
    uint16_t pulse_duration;
    uint32_t pulse_period;
    uint32_t last_pulse;
    uint32_t nextSample;
    uint32_t overloadExpire;
    uint16_t nsum;
    uint32_t sum_gnd;
    uint32_t sum_pre;
    uint16_t readings[READING_WINDOW - 1];
    uint8_t pwr_on;
    uint8_t pin_pre_sense;
    uint8_t pin_gnd_sense;
    uint8_t pin_overload;
    uint8_t pin_pulse;
};

REG_DEFINITION(                                   //
    power_regs,                                   //
    REG_SENSOR_COMMON,                              //
    REG_U8(JD_POWER_REG_ENABLED),                 //
    REG_U8(JD_POWER_REG_OVERLOAD),                //
    REG_U16(JD_POWER_REG_MAX_POWER),              //
    REG_U16(JD_POWER_REG_CURRENT_DRAW),           //
    REG_U16(JD_POWER_REG_BATTERY_VOLTAGE),        //
    REG_U16(JD_POWER_REG_KEEP_ON_PULSE_DURATION), //
    REG_U16(JD_POWER_REG_KEEP_ON_PULSE_PERIOD),   //
)

static void sort_ints(uint16_t arr[], int n) {
    for (int i = 1; i < n; i++)
        for (int j = i; j > 0 && arr[j - 1] > arr[j]; j--) {
            int tmp = arr[j - 1];
            arr[j - 1] = arr[j];
            arr[j] = tmp;
        }
}

static void overload(srv_t *state) {
    jd_power_enable(0);
    state->pwr_on = 0;
    state->overloadExpire = now + OVERLOAD_MS * 1000;
    state->overload = 1;
}

static int overload_detect(srv_t *state, uint16_t readings[READING_WINDOW]) {
    sort_ints(readings, READING_WINDOW);
    int med_gnd = readings[READING_WINDOW / 2];
    int ma = (med_gnd * MA_SCALE) >> 7;
    if (ma > state->max_power) {
        LOG("current %dmA", ma);
        overload(state);
        return 1;
    }
    return 0;
}

static void turn_on_power(srv_t *state) {
    LOG("power on");
    uint16_t readings[READING_WINDOW] = {0};
    unsigned rp = 0;
    adc_prep_read_pin(state->pin_gnd_sense);
    uint32_t t0 = tim_get_micros();
    jd_power_enable(1);
    for (int i = 0; i < 1000; ++i) {
        int gnd = adc_convert() >> 4;
        readings[rp++] = gnd;
        if (rp == READING_WINDOW)
            rp = 0;
        if (overload_detect(state, readings)) {
            LOG("overload after %d readings %d us", i + 1, (uint32_t)tim_get_micros() - t0);
            (void)t0;
            adc_disable();
            return;
        }
    }
    adc_disable();
    state->pwr_on = 1;
}

void power_process(srv_t *state) {
    sensor_process_simple(state, &state->curr_power, sizeof(state->curr_power));

    if (!jd_should_sample(&state->nextSample, CHECK_PERIOD * 9 / 10))
        return;

    if (state->pulse_period && state->pulse_duration) {
        uint32_t pulse_delta = now - state->last_pulse;
        if (pulse_delta > state->pulse_period * 1000) {
            pulse_delta = 0;
            state->last_pulse = now;
        }
        pin_set(state->pin_pulse, pulse_delta < state->pulse_duration * 1000);
    }

    if (state->overload && in_past(state->overloadExpire))
        state->overload = 0;
    pin_set(state->pin_overload, state->overload);

    int should_be_on = state->intensity && !state->overload;
    if (should_be_on != state->pwr_on) {
        if (should_be_on) {
            turn_on_power(state);
        } else {
            LOG("power off");
            state->pwr_on = 0;
            jd_power_enable(0);
        }
    }

    int gnd = adc_read_pin(state->pin_gnd_sense) >> 4;
    int pre = adc_read_pin(state->pin_pre_sense) >> 4;

    uint16_t sortedReadings[READING_WINDOW];
    memcpy(sortedReadings + 1, state->readings, sizeof(state->readings));
    sortedReadings[0] = gnd;
    memcpy(state->readings, sortedReadings, sizeof(state->readings));

    if (overload_detect(state, sortedReadings)) {
        LOG("overload detected");
    }

    state->nsum++;
    state->sum_gnd += gnd;
    state->sum_pre += pre;

    if (state->nsum >= 1000) {
        state->curr_power = (state->sum_gnd * MA_SCALE) / (128 * state->nsum);
        state->battery_voltage = (state->sum_pre * MV_SCALE) / (256 * state->nsum);

        LOG("%dmV %dmA %s", state->battery_voltage, state->curr_power,
            state->pwr_on ? "ON" : "OFF");

        state->nsum = 0;
        state->sum_gnd = 0;
        state->sum_pre = 0;
    }
}

void power_handle_packet(srv_t *state, jd_packet_t *pkt) {
    switch (service_handle_register(state, pkt, power_regs)) {
    case JD_POWER_REG_KEEP_ON_PULSE_PERIOD:
    case JD_POWER_REG_KEEP_ON_PULSE_DURATION:
        if (state->pulse_period && state->pulse_duration) {
            // assuming 22R in 0805, we get around 1W or power dissipation, but can withstand only
            // 1/8W continous so we limit duty cycle to 10%, and make sure it doesn't stay on for
            // more than 1s
            if (state->pulse_duration > 1000)
                state->pulse_duration = 1000;
            if (state->pulse_period < state->pulse_duration * 10)
                state->pulse_period = state->pulse_duration * 10;
        }
        break;
    case 0:
        sensor_handle_packet_simple(state, pkt, &state->curr_power, sizeof(state->curr_power));
        break;
    }
}

SRV_DEF(power, JD_SERVICE_CLASS_POWER);
void power_init(uint8_t pre_sense, uint8_t gnd_sense, uint8_t overload, uint8_t pulse) {
    SRV_ALLOC(power);
    state->intensity = 1;
    state->max_power = 500;
    // These should be reasonable defaults.
    // Only 1 out of 14 batteries tested wouldn't work with these settings.
    // 0.6/20s is 7mA (at 22R), so ~6 weeks on 10000mAh battery (mAh are quoted for 3.7V not 5V).
    // These can be tuned by the user for better battery life.
    // Note that the power_process() above at 1kHz takes about 1mA on its own.
    state->pulse_duration = 600;
    state->pulse_period = 20000;
    state->last_pulse = now - state->pulse_duration * 1000;
    state->pin_pre_sense = pre_sense;
    state->pin_gnd_sense = gnd_sense;
    state->pin_overload = overload;
    state->pin_pulse = pulse;

    tim_max_sleep = CHECK_PERIOD;
    pin_setup_output(overload);
    pin_setup_output(pulse);
}

#endif