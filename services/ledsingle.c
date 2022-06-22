// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_pins.h"
#include "interfaces/jd_pwm.h"
#include "interfaces/jd_hw_pwr.h"
#include "jacdac/dist/c/ledsingle.h"

#define RGB_IN_TIM 0x01
#define FRAME_US 65536

#define LOG JD_LOG
// #define LOG JD_NOLOG

typedef struct {
    uint16_t value;
    int16_t speed;
    uint8_t target;
    uint8_t pin;
    uint8_t pwm;
    uint8_t mult;
} channel_t;

struct srv_state {
    SRV_COMMON;

    led_params_t params;
    uint8_t numch;
    uint8_t flags;
    uint32_t step_sample;
    channel_t channels[3];
};

REG_DEFINITION(                               //
    led_regs,                                 //
    REG_SRV_COMMON,                           //
    REG_OPT8(JD_REG_VARIANT),                 //
    REG_OPT16(JD_LED_SINGLE_REG_WAVE_LENGTH),        //
    REG_OPT16(JD_LED_SINGLE_REG_LED_COUNT),          //
    REG_OPT16(JD_LED_SINGLE_REG_MAX_POWER),          //
    REG_OPT16(JD_LED_SINGLE_REG_LUMINOUS_INTENSITY), //
)

static void led_show(srv_t *state) {
    int sum = 0;

    int prevR = state->channels[0].value;

    if (state->numch == 1) {
        // take double green to avoid division when computing mean
        int tmp =
            state->channels[0].value + 2 * state->channels[1].value + state->channels[2].value;
        state->channels[0].value = tmp >> 2;
    }

    for (int i = 0; i < state->numch; ++i) {
        channel_t *ch = &state->channels[i];
        int32_t c = ch->value;
        c = (c * ch->mult) >> (7 + 8);
        sum += c;
        if (c == 0) {
            pin_set(ch->pin, state->params.active_high ? 0 : 1);
            jd_pwm_enable(ch->pwm, 0);
        } else {
            if (state->params.active_high) {
                if (c >= state->params.pwm_period)
                    c = state->params.pwm_period - 1;
            } else {
                c = state->params.pwm_period - c;
                if (c < 0)
                    c = 0;
            }
            jd_pwm_set_duty(ch->pwm, c);
            jd_pwm_enable(ch->pwm, 1);
        }
    }

    state->channels[0].value = prevR;

    if (sum == 0 && (state->flags & RGB_IN_TIM)) {
        pwr_leave_tim();
        state->flags &= ~RGB_IN_TIM;
    } else if (sum != 0 && !(state->flags & RGB_IN_TIM)) {
        pwr_enter_tim();
        state->flags |= RGB_IN_TIM;
    }
}

static void led_animate(srv_t *state, const jd_control_set_status_light_t *anim) {
    const uint8_t *to = &anim->to_red;
    for (int i = 0; i < 3; ++i) {
        channel_t *ch = &state->channels[i];
        ch->target = to[i];
        ch->speed = ((to[i] - (ch->value >> 8)) * anim->speed) >> 1;
    }
}

void led_process(srv_t *state) {
    if (!jd_should_sample(&state->step_sample, FRAME_US))
        return;

    int chg = 0;

    for (int i = 0; i < 3; ++i) {
        channel_t *ch = &state->channels[i];
        int v0 = ch->value;
        int v = v0 + ch->speed;
        if (ch->speed == 0 ||                           // immediate
            (ch->speed < 0 && (v >> 8) < ch->target) || // undershoot
            (ch->speed > 0 && (v >> 8) > ch->target)    // overshoot
        ) {
            ch->speed = 0;
            ch->value = ch->target << 8;
        } else {
            ch->value = v;
        }
        if (v0 != ch->value)
            chg = 1;
    }

    if (chg)
        led_show(state);
}

void led_handle_packet(srv_t *state, jd_packet_t *pkt) {
    switch (pkt->service_command) {
    case JD_GET(JD_LED_SINGLE_REG_COLOR): {
        uint8_t color[3];
        for (int i = 0; i < 3; ++i)
            color[i] = state->channels[i].value >> 8;
        jd_send(state->service_index, pkt->service_command, color, sizeof(color));
        break;
    }

    case JD_LED_SINGLE_CMD_ANIMATE:
        if (pkt->service_size >= sizeof(jd_control_set_status_light_t))
            led_animate(state, (jd_control_set_status_light_t *)pkt->data);
        break;

    default:
        service_handle_register_final(state, pkt, led_regs);
        break;
    }
}

SRV_DEF(led, JD_SERVICE_CLASS_LED_SINGLE);
void led_service_init(const led_params_t *params) {
    SRV_ALLOC(led);

    state->params = *params;
    state->channels[0].pin = params->pin_r;
    state->channels[0].mult = params->mult_r;
    state->channels[1].pin = params->pin_g;
    state->channels[1].mult = params->mult_g;
    state->channels[2].pin = params->pin_b;
    state->channels[2].mult = params->mult_b;

    if (state->params.pwm_period < 512)
        state->params.pwm_period = 512;

    state->numch = params->pin_g == NO_PIN ? 1 : 3;

    for (int i = 0; i < state->numch; ++i) {
        channel_t *ch = &state->channels[i];
        if (!ch->mult)
            ch->mult = 0xff;
        // assuming 64MHz clock and 512 period, we get 125kHz (which is slow enough for motor
        // drivers like DRV8837C) with 8MHz, we get 15kHz (which is plenty fast not to flicker)
        ch->pwm = jd_pwm_init(ch->pin, state->params.pwm_period, 0, 1);
    }

    led_show(state);
}
