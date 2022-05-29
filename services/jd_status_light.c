// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_pins.h"
#include "interfaces/jd_pwm.h"
#include "interfaces/jd_hw_pwr.h"
#include "jacdac/dist/c/control.h"

#if JD_CONFIG_STATUS == 1

#ifndef RGB_LED_PERIOD
#define RGB_LED_PERIOD 512
#endif

#if RGB_LED_PERIOD < 512
#error "RGB_LED_PERIOD must be at least 512"
#endif

#define FRAME_US 65536

// assume a non-RGB LED is connected like this: MCU -|>- GND
#ifndef PIN_LED_R
#ifndef PIN_LED_ACTIVE_LO
#define LED_RGB_COMMON_CATHODE 1
#endif
#endif

#ifdef LED_RGB_COMMON_CATHODE
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

typedef struct {
    uint32_t step_sample;
    uint32_t glow_step;
    uint32_t blink_step;

    uint32_t glow;
    uint8_t glow_phase;

    uint8_t in_tim;
    uint8_t curr_rep;
    uint8_t color_override;
    uint8_t queued_blinks[3];

    channel_t channels[3];
} status_ctx_t;
static status_ctx_t status_ctx;

static void rgbled_show(status_ctx_t *state) {
#ifdef PIN_LED_R
    int sum = 0;
    for (int i = 0; i < 3; ++i) {
        channel_t *ch = &state->channels[i];
        int32_t c = ch->value;
        c = (c * ch->mult) >> (7 + 8);
        if (state->color_override & (1 << i))
            c = RGB_LED_PERIOD - 1;
        sum += c;
#ifdef LED_RGB_COMMON_CATHODE
        if (c >= RGB_LED_PERIOD)
            c = RGB_LED_PERIOD - 1;
#else
        c = RGB_LED_PERIOD - c;
        if (c < 0)
            c = 0;
#endif
        // set duty first, in case pwm_enable() is not impl.
        pwm_set_duty(ch->pwm, c);
        if (c == 0) {
            pin_set(ch->pin, LED_OFF_STATE);
            pwm_enable(ch->pwm, 0);
        } else {
            pwm_enable(ch->pwm, 1);
        }
    }

    if (sum == 0 && state->in_tim) {
        pwr_leave_tim();
        state->in_tim = 0;
    } else if (sum != 0 && !state->in_tim) {
        pwr_enter_tim();
        state->in_tim = 1;
    }
#else
    int fl = 0;
    for (int i = 0; i < 3; ++i) {
        channel_t *ch = &state->channels[i];
        if (ch->value >> 8)
            fl = 1;
    }
    if (fl || state->color_override) {
        state->in_tim = 1;
        pin_set(PIN_LED, LED_ON_STATE);
    } else {
        state->in_tim = 0;
        pin_set(PIN_LED, LED_OFF_STATE);
    }
#endif
}

// TODO remove this
void jd_status_set_ch(int ch, uint8_t v) {
    status_ctx_t *state = &status_ctx;
    state->channels[ch].target = v;
    state->channels[ch].value = v << 8;
    state->channels[ch].speed = 0;
    rgbled_show(state);
}

static void jd_status_set(status_ctx_t *state, const jd_control_set_status_light_t *anim) {
    const uint8_t *to = &anim->to_red;
    for (int i = 0; i < 3; ++i) {
        channel_t *ch = &state->channels[i];
        ch->target = to[i];
        ch->speed = ((to[i] - (ch->value >> 8)) * anim->speed) >> 1;
    }
}

#define IS_FAINT(encoded) (((encoded) >> 6) == JD_BLINK_DURATION_FAINT)

static void set_blink_color(status_ctx_t *state, uint8_t encoded) {
    state->color_override = encoded & 7;
    rgbled_show(state);
}

void jd_status_process() {
    status_ctx_t *state = &status_ctx;

    uint8_t encoded = state->queued_blinks[0];
    if (encoded && jd_should_sample(&state->blink_step, 128 << 10)) {
        if (IS_FAINT(encoded)) {
            set_blink_color(state, encoded);
            target_wait_us(100);
            set_blink_color(state, 0);
            state->curr_rep += 2;
        } else {
            if (state->curr_rep & 1) {
                set_blink_color(state, 0);
            } else {
                set_blink_color(state, encoded);
                state->blink_step = now + (encoded >> 6) * (64 << 10);
            }
            state->curr_rep++;
        }
        if ((state->curr_rep >> 1) > ((encoded >> 3) & 7)) {
            for (int i = 1; i < sizeof(state->queued_blinks); ++i)
                state->queued_blinks[i - 1] = state->queued_blinks[i];
            state->blink_step = now + (256 << 10);
        }
    }

    if (!jd_should_sample(&state->step_sample, FRAME_US))
        return;

    if (state->glow && in_past(state->glow_step)) {
        if (state->glow == JD_GLOW_PROTECT) {
            state->glow = 0;
        } else {
            jd_control_set_status_light_t color;
            int c = _JD_GLOW_COLOR(state->glow);
            if (state->glow_phase) {
                c = 0;
                state->glow_step = now + _JD_GLOW_GAP(state->glow);
                state->glow_phase = 1;
            } else {
                state->glow_step = now + _JD_GLOW_DURATION(state->glow);
                state->glow_phase = 0;
            }
            color.to_red = (c & 1) ? 0xff : 0;
            color.to_green = (c & 2) ? 0xff : 0;
            color.to_blue = (c & 4) ? 0xff : 0;
            color.speed = _JD_GLOW_SPEED(state->glow);
            jd_status_set(state, &color);
        }
    }

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

int jd_status_handle_packet(jd_packet_t *pkt) {
    status_ctx_t *state = &status_ctx;

    switch (pkt->service_command) {
#if 0
    case JD_GET(JD_CONTROL_REG_STATUS_COLOR): {
        uint8_t color[3];
        for (int i = 0; i < 3; ++i)
            color[i] = state->channels[i].value >> 8;
        jd_send(0, pkt->service_command, color, sizeof(color));
        return 1;
    }
#endif

    case JD_CONTROL_CMD_SET_STATUS_LIGHT:
        if (pkt->service_size >= sizeof(jd_control_set_status_light_t)) {
            state->glow = JD_GLOW_PROTECT;
            state->glow_step = now + (2 << 20); // no glow for 2s
            jd_status_set(state, (jd_control_set_status_light_t *)pkt->data);
        }
        return 1;
    }

    return 0;
}

#ifdef PIN_LED_R
const uint8_t mults[] = {LED_R_MULT, LED_G_MULT, LED_B_MULT};
const uint8_t pins[] = {PIN_LED_R, PIN_LED_G, PIN_LED_B};
#endif

void jd_status_init() {
    status_ctx_t *state = &status_ctx;

#ifdef PIN_LED_R
    for (int i = 0; i < 3; ++i) {
        channel_t *ch = &state->channels[i];
        ch->pin = pins[i];
        ch->mult = mults[i];
        ch->pwm = pwm_init(ch->pin, RGB_LED_PERIOD, 0, 1);
    }
#else
    for (int i = 0; i < 3; ++i) {
        channel_t *ch = &state->channels[i];
        ch->pin = PIN_LED;
        ch->mult = 100;
    }
#endif

    rgbled_show(state);
}

void jd_blink(uint8_t encoded) {
    status_ctx_t *state = &status_ctx;
    for (int i = 0; i < sizeof(state->queued_blinks); ++i) {
        if (state->queued_blinks[i] == encoded)
            return;
        if (state->queued_blinks[i] == 0) {
            state->queued_blinks[i] = encoded;
            if (i == 0)
                state->blink_step = now;
            return;
        }
    }
}

void jd_glow(uint32_t glow) {
    status_ctx_t *state = &status_ctx;
    if (state->glow == glow || state->glow == JD_GLOW_PROTECT)
        return;
    if (_JD_GLOW_CHANNEL(glow) >= _JD_GLOW_CHANNEL(state->glow)) {
        if (!_JD_GLOW_COLOR(glow)) {
            state->glow = 0; // if turning off, go down to CH_0
            jd_control_set_status_light_t color = {0};
            jd_status_set(state, &color);
        } else {
            state->glow = glow;
            state->glow_phase = 0;
            state->glow_step = now;
        }
    }
}

#endif
