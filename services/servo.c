// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_protocol.h"
#include "interfaces/jd_sensor.h"
#include "interfaces/jd_pwm.h"
#include "interfaces/jd_pins.h"
#include "interfaces/jd_hw_pwr.h"
// include the generated header with constants defining registers, commands, etc
#include "jacdac/dist/c/servo.h"

#define SERVO_PERIOD 20000

// this is always called struct srv_state
struct srv_state {
    SRV_COMMON;
    // the new two fields are exposed as registers in the REG_DEFINITION() below
    uint32_t pulse;
    uint8_t intensity;
    // these fields are not exposed
    uint8_t pwm_pin;
    uint8_t is_on;
    uint8_t pin;
};

REG_DEFINITION(                   //
    servo_regs,                   //
    REG_SRV_BASE,                 //
    REG_U32(JD_SERVO_REG_PULSE),  // this must match the uint32_t type on 'pulse' field in srv_state
    REG_U8(JD_SERVO_REG_ENABLED), // same, for 'uint8_t enabled'
)

static void set_pwr(srv_t *state, int on) {
    if (state->is_on == on)
        return;
    if (on) {
        pwr_enter_pll();
        // configure at 1MHz
        if (!state->pwm_pin)
            state->pwm_pin = pwm_init(state->pin, SERVO_PERIOD, 0, cpu_mhz);
        pwm_enable(state->pwm_pin, 1);
    } else {
        pin_set(state->pin, 0);
        pwm_enable(state->pwm_pin, 0);
        pwr_leave_pll();
    }
    jd_power_enable(on);
    state->is_on = on;
}

void servo_process(srv_t *state) {}

void servo_handle_packet(srv_t *state, jd_packet_t *pkt) {
    // service_handle_register() will read or update 'state' according to mappings specified
    // in 'servo_regs', and according to 'pkt';
    // it will either send a response with the current state, or update state.
    // It returns the code of the written (or read) register.
    // Here, we just assume if anything was updated, we sync our state to hardware
    if (service_handle_register(state, pkt, servo_regs) > 0) {
        set_pwr(state, !!state->intensity);
        if (state->is_on)
            pwm_set_duty(state->pwm_pin, state->pulse);
    }
}

SRV_DEF(servo, JD_SERVICE_CLASS_SERVO);
void servo_init(uint8_t pin) {
    SRV_ALLOC(servo);
    state->pin = pin;
}
