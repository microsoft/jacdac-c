// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_pins.h"
#include "interfaces/jd_pwm.h"
#include "interfaces/jd_hw_pwr.h"
#include "jacdac/dist/c/led.h"

#ifdef PIN_LED_R

#define PERIOD 600
#define RGB_IN_TIM 0x01

#define FRAME_US 100000

#ifndef LED_RGB_COMMON_CATHODE
#define LED_OFF_STATE 0
#define LED_ON_STATE 1
#else
#define LED_OFF_STATE 1
#define LED_ON_STATE 0
#endif

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
    uint8_t numleds;
    uint8_t status;
    uint32_t step_sample;
    channel_t channels[3];
};

REG_DEFINITION(                   //
    rgbled_regs,                  //
    REG_SRV_BASE,                 //
    REG_U8(JD_LED_REG_LED_COUNT), //
)

static void rgbled_show(srv_t *state) {
    int sum = 0;
    for (int i = 0; i < 3; ++i) {
        channel_t *ch = &state->channels[i];
        int32_t c = ch->value;
        c = (c * ch->mult) >> (7 + 8);
        sum += c;
        if (c == 0) {
            pin_set(ch->pin, LED_OFF_STATE);
            pwm_enable(ch->pwm, 0);
        } else {
#ifndef LED_RGB_COMMON_CATHODE
            c = PERIOD - c;
            if (c < 0)
                c = 0;
#else
            if (c >= PERIOD)
                c = PERIOD - 1;
#endif
            pwm_set_duty(ch->pwm, c);
            pwm_enable(ch->pwm, 1);
        }
    }

    if (sum == 0 && (state->status & RGB_IN_TIM)) {
        pwr_leave_tim();
        state->status &= ~RGB_IN_TIM;
    } else if (sum != 0 && !(state->status & RGB_IN_TIM)) {
        pwr_enter_tim();
        state->status |= RGB_IN_TIM;
    }
}

void rgbled_process(srv_t *state) {
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
        rgbled_show(state);
}

static void rgbled_animate(srv_t *state, jd_led_animate_t *anim) {
    uint8_t *to = &anim->to_red;
    for (int i = 0; i < 3; ++i) {
        channel_t *ch = &state->channels[i];
        ch->target = to[i];
        ch->speed = (((to[i] << 8) - ch->value) * anim->speed) >> 1;
    }
}

void rgbled_handle_packet(srv_t *state, jd_packet_t *pkt) {
    if (service_handle_register(state, pkt, rgbled_regs))
        return;

    switch (pkt->service_command) {
    case JD_GET(JD_LED_REG_COLOR): {
        uint8_t color[3];
        for (int i = 0; i < 3; ++i)
            color[i] = state->channels[i].value >> 8;
        jd_send(state->service_number, pkt->service_command, color, sizeof(color));
        break;
    }
    case JD_LED_CMD_ANIMATE:
        if (pkt->service_size >= 4)
            rgbled_animate(state, (jd_led_animate_t *)pkt->data);
        break;
    }
}

SRV_DEF(rgbled, JD_SERVICE_CLASS_LED);
void rgbled_init() {
    SRV_ALLOC(rgbled);

    state->channels[0].pin = PIN_LED_R;
    state->channels[0].mult = LED_R_MULT;
    state->channels[1].pin = PIN_LED_G;
    state->channels[1].mult = LED_G_MULT;
    state->channels[2].pin = PIN_LED_B;
    state->channels[2].mult = LED_B_MULT;

    for (int i = 0; i < 3; ++i) {
        channel_t *ch = &state->channels[i];
        ch->pwm = pwm_init(ch->pin, PERIOD, 0, 1);
    }

    rgbled_show(state);
}

#endif