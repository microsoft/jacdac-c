// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_pins.h"
#include "interfaces/jd_pwm.h"
#include "interfaces/jd_hw_pwr.h"
#include "jacdac/dist/c/motor.h"

#define PERIOD 600
#define RGB_IN_TIM 0x01

struct rgb_state {
    uint8_t status;
    rgbled_channel_t channels[3];
};

static struct rgb_state rgb_ctx;

void rgbled_init(const rgbled_params_t *params) {
    struct rgb_state *ctx = &rgb_ctx;
    memcpy(ctx->channels, &params->r, sizeof(ctx->channels));
    for (int i = 0; i < 3; ++i) {
        rgbled_channel_t *ch = &ctx->channels[i];
        ch->pwm = pwm_init(ch->pin, PERIOD, 0, 1);
    }
    rgbled_set(0);
}

void rgbled_set(uint32_t color) {
    struct rgb_state *ctx = &rgb_ctx;

    if (color == 0) {
        for (int i = 0; i < 3; ++i) {
            rgbled_channel_t *ch = &ctx->channels[i];
            pwm_enable(ch->pwm, 0);
            pin_setup_analog_input(ch->pin);
        }
        if (ctx->status & RGB_IN_TIM) {
            pwr_leave_tim();
            ctx->status &= ~RGB_IN_TIM;
        }
    } else {
        if (!(ctx->status & RGB_IN_TIM)) {
            pwr_enter_tim();
            ctx->status |= RGB_IN_TIM;
        }

        for (int i = 0; i < 3; ++i) {
            rgbled_channel_t *ch = &ctx->channels[i];
            int32_t c = (color >> (16 - (i * 8))) & 0xff;
            c = (c * ch->mult) >> 7;
            c = PERIOD - c;
            if (c < 0)
                c = 0;
            pwm_set_duty(ch->pwm, c);
            pwm_enable(ch->pwm, 1);
        }
    }
}
