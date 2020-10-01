// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_protocol.h"
#include "interfaces/jd_sensor.h"
#include "interfaces/jd_pwm.h"
#include "interfaces/jd_pins.h"
#include "interfaces/jd_hw_pwr.h"

#define SERVO_PERIOD 20000

struct srv_state {
    SRV_COMMON;
    uint32_t pulse;
    uint8_t intensity;
    uint8_t pwm_pin;
    uint8_t is_on;
    uint8_t pin;
};

REG_DEFINITION(               //
    servo_regs,               //
    REG_SRV_BASE,             //
    REG_U32(JD_REG_VALUE),    //
    REG_U8(JD_REG_INTENSITY), //
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
    if (service_handle_register(state, pkt, servo_regs)) {
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
