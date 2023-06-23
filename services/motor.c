// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_pins.h"
#include "interfaces/jd_pwm.h"
#include "interfaces/jd_hw_pwr.h"
#include "jacdac/dist/c/motor.h"

#define PWM_BITS 9
#define SERVO_PERIOD (1 << PWM_BITS)

#define LOG JD_LOG
// #define LOG JD_NOLOG

typedef struct channel {
    uint8_t pin;
    uint8_t pwm_pin;
    uint16_t target_duty;
    uint16_t current_duty;
} channel_t;

struct srv_state {
    SRV_COMMON;
    int16_t value;
    uint8_t intensity;
    uint8_t pin_en;
    uint8_t is_on;

    channel_t ch1;
    channel_t ch2;

    uint32_t duty_step_sample;
};

REG_DEFINITION(                   //
    motor_regs,                   //
    REG_SRV_COMMON,               //
    REG_I16(JD_MOTOR_REG_SPEED),  //
    REG_U8(JD_MOTOR_REG_ENABLED), //
)

static void disable_ch(channel_t *ch) {
    pin_set(ch->pin, 0);
    if (ch->pwm_pin) {
        jd_pwm_set_duty(ch->pwm_pin, 0);
        jd_pwm_enable(ch->pwm_pin, 0);
    }
    ch->current_duty = 0;
    ch->target_duty = 0;
}

static void enable_ch(channel_t *ch) {
    if (!ch->pwm_pin)
        ch->pwm_pin = jd_pwm_init(ch->pin, SERVO_PERIOD, 0, cpu_mhz / 16);
    jd_pwm_enable(ch->pwm_pin, 1);
}

static void set_pwr(srv_t *state, int on) {
    if (state->is_on == on)
        return;

    LOG("PWR %d", on);

    disable_ch(&state->ch1);
    disable_ch(&state->ch2);

    if (on) {
        pwr_enter_tim();
        enable_ch(&state->ch1);
        enable_ch(&state->ch2);
        pin_set(state->pin_en, 1);
    } else {
        pin_set(state->pin_en, 0);
        pwr_leave_tim();
    }
    state->is_on = on;
    LOG("PWR OK");
}

static void duty_step(channel_t *ch) {
    if (ch->target_duty == ch->current_duty)
        return;

    // if we're below, step up
    if (ch->target_duty > ch->current_duty)
        ch->current_duty += SERVO_PERIOD / 32;

    // never go above, and also if need to step down, so it immediately
    if (ch->target_duty < ch->current_duty)
        ch->current_duty = ch->target_duty;

    jd_pwm_set_duty(ch->pwm_pin, ch->current_duty);
}

void motor_process(srv_t *state) {
    if (!jd_should_sample(&state->duty_step_sample, 9000))
        return;
    duty_step(&state->ch1);
    duty_step(&state->ch2);
}

static int pwm_value(int v) {
    v >>= 15 - PWM_BITS;
    if (v >= (1 << PWM_BITS))
        v = (1 << PWM_BITS) - 1;
    return v;
}

void motor_handle_packet(srv_t *state, jd_packet_t *pkt) {
    if (service_handle_register_final(state, pkt, motor_regs)) {
        set_pwr(state, !!state->intensity);
        if (state->is_on) {
            LOG("PWM set %d", state->value);
            if (state->value < 0) {
                state->ch1.target_duty = 0;
                state->ch2.target_duty = pwm_value(-state->value);
            } else {
                state->ch1.target_duty = pwm_value(state->value);
                state->ch2.target_duty = 0;
            }
            LOG("PWM set %d %d", state->ch1.target_duty, state->ch2.target_duty);
        }
    }
}

SRV_DEF(motor, JD_SERVICE_CLASS_MOTOR);
void motor_init(uint8_t pin1, uint8_t pin2, uint8_t pin_nsleep) {
    SRV_ALLOC(motor);
    state->ch1.pin = pin1;
    state->ch2.pin = pin2;
    state->pin_en = pin_nsleep;
    pin_setup_output(state->pin_en);
    pin_setup_output(state->ch1.pin);
    pin_setup_output(state->ch2.pin);
}

#if JD_DCFG
void motor_config(void) {
    motor_init(jd_srvcfg_pin("pin1"), jd_srvcfg_pin("pin2"), jd_srvcfg_pin("pinEnable"));
}
#endif
