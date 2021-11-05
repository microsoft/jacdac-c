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

struct status_anim {
    jd_control_set_status_light_t color;
    uint32_t time;
};

// .color is {R, G, B, speed}
static const struct status_anim jd_status_animations[] = {
    {.color = {0, 0, 0, 0}, .time = 0},              // OFF
    {.color = {0, 255, 0, 150}, .time = 200 * 1000}, // STARTUP
    // the time on CONNECTED is used in only used with a non-RGB LED; note that there's already a
    // ~30us overhead
    {.color = {0, 255, 0, 0}, .time = 100},            // CONNECTED
    {.color = {255, 0, 0, 100}, .time = 500 * 1000},   // DISCONNECTED
    {.color = {0, 0, 255, 0}, .time = 50 * 1000},      // IDENTIFY
    {.color = {255, 255, 0, 100}, .time = 500 * 1000}, // UNKNOWN STATE
};

typedef struct {
    uint16_t value;
    int16_t speed;
    uint8_t target;
    uint8_t pin;
    uint8_t pwm;
    uint8_t mult;
} channel_t;

typedef struct {
    uint8_t numleds;
    uint8_t in_tim;
    uint8_t jd_status; // JD_STATUS_*
    uint32_t step_sample;
    uint32_t jd_status_stop;
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
        sum += c;
        if (c == 0) {
            pin_set(ch->pin, LED_OFF_STATE);
            pwm_enable(ch->pwm, 0);
        } else {
#ifdef LED_RGB_COMMON_CATHODE
            if (c >= RGB_LED_PERIOD)
                c = RGB_LED_PERIOD - 1;
#else
            c = RGB_LED_PERIOD - c;
            if (c < 0)
                c = 0;
#endif
            pwm_set_duty(ch->pwm, c);
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
    if (fl) {
        state->in_tim = 1;
        pin_set(PIN_LED, LED_ON_STATE);
    } else {
        state->in_tim = 0;
        pin_set(PIN_LED, LED_OFF_STATE);
    }
#endif
}

static void rgbled_animate(status_ctx_t *state, const jd_control_set_status_light_t *anim) {
    const uint8_t *to = &anim->to_red;
    for (int i = 0; i < 3; ++i) {
        channel_t *ch = &state->channels[i];
        ch->target = to[i];
        ch->speed = ((to[i] - (ch->value >> 8)) * anim->speed) >> 1;
    }
}

void jd_status_process() {
    status_ctx_t *state = &status_ctx;

    if (!jd_should_sample(&state->step_sample, FRAME_US))
        return;

    if (state->jd_status && in_past(state->jd_status_stop)) {
        jd_control_set_status_light_t off_color = {
            .speed = jd_status_animations[state->jd_status].color.speed};
        rgbled_animate(state, &off_color);
        state->jd_status = JD_STATUS_OFF;
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
        if (pkt->service_size >= sizeof(jd_control_set_status_light_t))
            rgbled_animate(state, (jd_control_set_status_light_t *)pkt->data);
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

// definitions for @status are contained in jd_io.h
void jd_status(int status) {
    status_ctx_t *state = &status_ctx;

    if (status == JD_STATUS_CONNECTED) {
        // if the PWM is on, we ignore this one
        if (state->in_tim)
            return;
        channel_t *ch = &state->channels[1]; // green
        pin_set(ch->pin, LED_ON_STATE);
#ifdef PIN_LED_R
        pwm_enable(ch->pwm, 0);
#else
        target_wait_us(jd_status_animations[status].time);
#endif
        pin_set(ch->pin, LED_OFF_STATE);
        return;
    }

    rgbled_animate(state, &jd_status_animations[status].color);
    state->jd_status = status;
    state->jd_status_stop = now + jd_status_animations[status].time;
}

#endif
