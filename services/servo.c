// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_pwm.h"
#include "interfaces/jd_pins.h"
#include "interfaces/jd_hw_pwr.h"
// include the generated header with constants defining registers, commands, etc
#include "jacdac/dist/c/servo.h"

#define SERVO_PERIOD 20000

// this is always called struct srv_state
struct srv_state {
    SRV_COMMON;
    // the new three fields are exposed as registers in the REG_DEFINITION() below
    int32_t angle;
    int32_t offset;
    servo_params_t params;
    uint8_t intensity;
    // these fields are not exposed
    uint8_t pwm_pin;
    uint8_t is_on;
    const servo_params_t *params0;
    uint32_t pulse;
};

REG_DEFINITION(                   //
    servo_regs,                   //
    REG_SRV_COMMON,               //
    REG_I32(JD_SERVO_REG_ANGLE),  // this must match the uint32_t type on 'angle' field in srv_state
    REG_I32(JD_SERVO_REG_OFFSET), // ditto for 'offset'
    REG_U8(JD_REG_PADDING),       // pin
    REG_U8(JD_REG_PADDING),       // fixed
    REG_U8(JD_REG_PADDING),       // power pin
    REG_I32(JD_SERVO_REG_MIN_ANGLE), //
    REG_I32(JD_SERVO_REG_MAX_ANGLE), //
    REG_U16(JD_SERVO_REG_MIN_PULSE), //
    REG_U16(JD_SERVO_REG_MAX_PULSE), //
    REG_U8(JD_SERVO_REG_ENABLED),    // same, for 'uint8_t enabled'
)

static void set_pwr(srv_t *state, int on) {
    if (state->is_on == on)
        return;
    if (on) {
        pin_setup_output(state->params.power_pin);
        pin_set(state->params.power_pin, 0);
        pwr_enter_pll();
        // configure at 1MHz
        if (!state->pwm_pin)
            state->pwm_pin = jd_pwm_init(state->params.pin, SERVO_PERIOD, 0, cpu_mhz);
        jd_pwm_enable(state->pwm_pin, 1);
    } else {
        pin_setup_input(state->params.power_pin, PIN_PULL_NONE);
        pin_set(state->params.pin, 0);
        jd_pwm_enable(state->pwm_pin, 0);
        pwr_leave_pll();
    }
    jd_power_enable(on);
    state->is_on = on;
}

void servo_process(srv_t *state) {}

static int clamp(int low, int v, int hi) {
    if (v < low)
        return low;
    if (v > hi)
        return hi;
    return v;
}

void servo_handle_packet(srv_t *state, jd_packet_t *pkt) {
    // service_handle_register() will read or update 'state' according to mappings specified
    // in 'servo_regs', and according to 'pkt';
    // it will either send a response with the current state, or update state.
    // It returns the code of the written (or read) register.
    // Here, we just assume if anything was updated, we sync our state to hardware
    if (service_handle_register_final(state, pkt, servo_regs) > 0) {
        set_pwr(state, !!state->intensity);

        servo_params_t *p = &state->params;
        if (p->fixed) {
            // if the config if fixed, just overwrite whatever the user might have written
            *p = *state->params0;
        } else {
            // otherwise, clamp the supplied user config
            // the clamping values are chosen so that the multiplication in pulse computation
            // doesn't overflow we take a larger positive max_angle for linear servos
            p->min_angle = clamp(-(360 << 16), p->min_angle, 1024 << 16);
            p->max_angle = clamp(-(360 << 16), p->max_angle, 1024 << 16);
            p->min_pulse = clamp(0, p->min_pulse, 6000);
            p->max_pulse = clamp(0, p->max_pulse, 6000);
        }

        // clamp offset to allowed range
        state->offset = clamp(p->min_angle, state->offset, p->max_angle);
        // compute the effective angle, with offset and clamp it
        int angle = clamp(p->min_angle, state->angle + state->offset, p->max_angle);
        // make sure the user sees the actual value of angle if they read it back
        state->angle = angle - state->offset;

        // compute pulse length based on servo parameters and pulse lengths
        state->pulse = ((angle - p->min_angle) >> 8) * (p->max_pulse - p->min_pulse) /
                       ((p->max_angle - p->min_angle) >> 8);
        state->pulse += p->min_pulse;

        if (state->is_on)
            jd_pwm_set_duty(state->pwm_pin, state->pulse);
    }
}

SRV_DEF(servo, JD_SERVICE_CLASS_SERVO);
void servo_init(const servo_params_t *params) {
    SRV_ALLOC(servo);
    state->params = *params;
    state->params0 = params;
}
