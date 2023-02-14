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

#if JD_DCFG
void jd_rgb_set(uint8_t r, uint8_t g, uint8_t b);
void jd_rgb_init(void);
#define LED_SET_RGB jd_rgb_set
#endif

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
#ifdef LED_SET_RGB
    uint8_t prev;
#endif
} channel_t;

typedef struct {
    uint32_t step_sample;
    uint32_t glow_step;
    uint32_t blink_step;

    uint8_t glow;
    uint8_t glow_ch[JD_GLOW_CHANNELS];
    uint8_t glow_phase;

    uint8_t in_tim;
    uint8_t blink_rep;
    uint8_t color_override;
    uint8_t queued_blinks[3];

    channel_t channels[3];
} status_ctx_t;
static status_ctx_t status_ctx;

#define JD_GLOW_PROTECT JD_GLOW(OFF, CH_0, WHITE)

static void jd_status_set(status_ctx_t *state, const jd_control_set_status_light_t *anim) {
    const uint8_t *to = &anim->to_red;
    for (int i = 0; i < 3; ++i) {
        channel_t *ch = &state->channels[i];
        ch->target = to[i];
        ch->speed = ((to[i] - (ch->value >> 8)) * anim->speed) >> 1;
    }
}

static void re_glow(status_ctx_t *state) {
    if (state->glow == JD_GLOW_PROTECT)
        return;

    for (int i = JD_GLOW_CHANNELS - 1; i >= 0; i--) {
        if (state->glow_ch[i] || i == 0) {
            if (state->glow != state->glow_ch[i]) {
                state->glow = state->glow_ch[i];
                state->glow_phase = 0;
                state->glow_step = now;
            }
            break;
        }
    }

    if (state->glow == 0) {
        // fade to black if no glow
        jd_control_set_status_light_t c = {.speed = 100};
        jd_status_set(state, &c);
    }
}

static void rgbled_show(status_ctx_t *state) {
#if defined(LED_SET_RGB)
    int chg = 0;
    for (int i = 0; i < 3; ++i) {
        channel_t *ch = &state->channels[i];
        int32_t c = ch->value;
        if (state->color_override)
            c = state->color_override & (1 << i) ? 0xff00 : 0x0000;
        c = (c * ch->mult) >> (8 + 8);
        if (c > 0xff)
            c = 0xff;
        if (ch->prev != c) {
            chg = 1;
            ch->prev = c;
        }
    }
    if (chg)
        LED_SET_RGB(state->channels[0].prev, state->channels[1].prev, state->channels[2].prev);
#elif defined(PIN_LED_R)
    int sum = 0;
    for (int i = 0; i < 3; ++i) {
        channel_t *ch = &state->channels[i];
        int32_t c = ch->value;
        if (state->color_override)
            c = state->color_override & (1 << i) ? 0xff00 : 0x0000;
        c = (c * ch->mult) >> (7 + 8);
        sum += c;
#ifdef LED_RGB_COMMON_CATHODE
        if (c >= RGB_LED_PERIOD)
            c = RGB_LED_PERIOD - 1;
#else
        c = RGB_LED_PERIOD - c;
        if (c < 0)
            c = 0;
#endif
        // set duty first, in case jd_pwm_enable() is not impl.
        jd_pwm_set_duty(ch->pwm, c);
        if (c == 0) {
            pin_set(ch->pin, LED_OFF_STATE);
            jd_pwm_enable(ch->pwm, 0);
        } else {
            jd_pwm_enable(ch->pwm, 1);
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

static void set_blink_color(status_ctx_t *state, uint8_t encoded) {
    state->color_override = encoded & 7;
    rgbled_show(state);
}

struct glow_desc {
    uint8_t speed;
    uint32_t time_on;
    uint32_t time_off;
};

#define GLOW(sp, ms_on, ms_off)                                                                    \
    { sp, 1000 * (ms_on), 1000 * (ms_off) }

static const struct glow_desc speeds[] = {
    GLOW(1, 1, 1000),      // constant off
    GLOW(50, 1000, 1),     // constant on
    GLOW(100, 1000, 1000), // very_slow
    GLOW(150, 500, 500),   // slow
    GLOW(150, 250, 250),   // fast
    GLOW(150, 250, 1750),  // very_slow_low
    GLOW(0, 500, 500),     // slow_blink
    GLOW(0, 250, 250),     // fast_blink
};

void jd_status_process() {
    status_ctx_t *state = &status_ctx;
    int chg = 0;

#if 0
    // calibration mode
    int cc = (now >> 19) & 3;
    for (int i = 0; i < 3; ++i) {
        channel_t *ch = &state->channels[i];
        int v = (i == cc) ? 0x2000 : 0x0000;
        if (ch->value != v) {
            ch->value = v;
            chg = 1;
        }
    }
#else
    uint16_t encoded = state->queued_blinks[0];
    if (encoded && jd_should_sample(&state->blink_step, 128 << 10)) {
        if (_JD_BLINK_DURATION(encoded) == JD_BLINK_DURATION_FAINT) {
#ifndef LED_SET_RGB
            // faint blinks don't really work with WS2812B
            set_blink_color(state, encoded);
            // target_wait_us(100); - enough delay from just calling these functions
            set_blink_color(state, 0);
#endif
            state->blink_rep += 2;
        } else {
            if (state->blink_rep & 1) {
                set_blink_color(state, 0);
            } else {
                set_blink_color(state, encoded);
                state->blink_step = now + _JD_BLINK_DURATION(encoded) * (8 << 10);
            }
            state->blink_rep++;
        }
        if ((state->blink_rep >> 1) >= _JD_BLINK_REPETITIONS(encoded)) {
            // end of this blink pattern
            for (unsigned i = 1; i < sizeof(state->queued_blinks); ++i)
                state->queued_blinks[i - 1] = state->queued_blinks[i];
            state->blink_step = now + (256 << 10);
        }
    }

    if (!jd_should_sample(&state->step_sample, FRAME_US))
        return;

    if (state->glow && in_past(state->glow_step)) {
        if (state->glow == JD_GLOW_PROTECT) {
            state->glow = 0;
            re_glow(state);
        } else {
            jd_control_set_status_light_t color;
            int c = _JD_GLOW_COLOR(state->glow);
            int sp = _JD_GLOW_SPEED(state->glow);
            if (state->glow_phase == 0) {
                c = 0;
                state->glow_step = now + speeds[sp].time_off;
                state->glow_phase = 1;
            } else {
                state->glow_step = now + speeds[sp].time_on;
                state->glow_phase = 0;
            }
            int color_on = 0x08;
            color.to_red = (c & 1) ? color_on : 0;
            color.to_green = (c & 2) ? color_on : 0;
            color.to_blue = (c & 4) ? color_on : 0;
            color.speed = speeds[sp].speed;
            jd_status_set(state, &color);
        }
    }

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
#endif

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
static const uint8_t pins[] = {PIN_LED_R, PIN_LED_G, PIN_LED_B};
#endif

#ifdef LED_R_MULT
static const uint8_t mults[] = {LED_R_MULT, LED_G_MULT, LED_B_MULT};
#endif

void jd_status_init() {
    status_ctx_t *state = &status_ctx;

#if JD_DCFG
    jd_rgb_init();
#else

#if defined(PIN_LED_R) || defined(LED_SET_RGB)
    for (int i = 0; i < 3; ++i) {
        channel_t *ch = &state->channels[i];
#ifdef PIN_LED_R
        ch->pin = pins[i];
        ch->pwm = jd_pwm_init(ch->pin, RGB_LED_PERIOD, 0, 1);
#else
        ch->pwm = 1; // force refresh
#endif
#ifdef LED_R_MULT
        ch->mult = mults[i];
#else
        ch->mult = 255;
#endif
    }
#else
    for (int i = 0; i < 3; ++i) {
        channel_t *ch = &state->channels[i];
        ch->pin = PIN_LED;
        ch->mult = 100;
    }
#endif
#endif

    rgbled_show(state);
}

uint8_t jd_connected_blink = JD_BLINK_CONNECTED;

void jd_blink(uint8_t encoded) {
#if 0
    pin_setup_output(PIN_AN);
    pin_pulse(PIN_AN, _JD_BLINK_REPETITIONS(encoded));
#endif

    status_ctx_t *state = &status_ctx;

#if 0
    if (_JD_BLINK_DURATION(encoded) != JD_BLINK_DURATION_FAINT) {
        if (_JD_BLINK_COLOR(encoded) & 1)
            pin_pulse(2, 2);
        if (_JD_BLINK_COLOR(encoded) & 2)
            pin_pulse(3, 2);
        if (_JD_BLINK_COLOR(encoded) & 4)
            pin_pulse(4, 2);
    }
#endif

    for (unsigned i = 0; i < sizeof(state->queued_blinks); ++i) {
        if (state->queued_blinks[i] == encoded)
            return;
        if (state->queued_blinks[i] == 0) {
            if (_JD_BLINK_DURATION(encoded) != JD_BLINK_DURATION_FAINT || i == 0) {
                if (i == 0) {
                    state->blink_step = now;
                    state->blink_rep = 0;
                }
                state->queued_blinks[i] = encoded;
            }
            return;
        }
    }
}

void jd_glow(uint32_t glow) {
    status_ctx_t *state = &status_ctx;
    int ch = _JD_GLOW_CHANNEL(glow);
    uint8_t v = glow;
    if (state->glow_ch[ch] == v)
        return;
    state->glow_ch[ch] = v;
    re_glow(state);
}

#ifdef JD_DCFG
static uint16_t led_period;
static bool led_active_high;
static bool has_led;
static bool is_mono;
static bool is_rgbext;

__attribute__((weak)) void jd_rgbext_init(int type, uint8_t pin) {}
__attribute__((weak)) void jd_rgbext_set(uint8_t r, uint8_t g, uint8_t b) {}

void jd_rgb_init(void) {
    status_ctx_t *state = &status_ctx;

    has_led = true;
    is_mono = dcfg_get_bool("led.isMono");

    led_period = 256;

    uint8_t pin = dcfg_get_pin("led.pin");

    int ledType = dcfg_get_i32("led.type", 0);
    if (ledType) {
        for (int i = 0; i < 3; ++i) {
            channel_t *ch = &state->channels[i];
            ch->mult = 255;
        }
        is_rgbext = true;
        jd_rgbext_init(ledType, pin);
        jd_rgb_set(0, 0, 0);
        return;
    }

    uint8_t pin_r = dcfg_get_pin(dcfg_idx_key("led.rgb", 0, "pin"));

    if (pin_r == NO_PIN) {
        is_mono = true;
        if (pin == NO_PIN)
            has_led = false;
    }

    led_active_high = dcfg_get_bool("led.activeHigh");

    if (is_mono)
        for (int i = 0; i < 3; ++i) {
            channel_t *ch = &state->channels[i];
            ch->pin = pin;
            ch->mult = 255;
        }
    else
        for (int i = 0; i < 3; ++i) {
            channel_t *ch = &state->channels[i];
            ch->pin = dcfg_get_pin(dcfg_idx_key("led.rgb", i, "pin"));
            ch->pwm = jd_pwm_init(ch->pin, led_period, 0, 1);
            ch->mult = dcfg_get_i32(dcfg_idx_key("led.rgb", i, "mult"), 255);
        }
}

void jd_rgb_set(uint8_t r, uint8_t g, uint8_t b) {
    status_ctx_t *state = &status_ctx;

    if (is_rgbext) {
        jd_rgbext_set(r, g, b);
    } else if (!is_mono) {
        uint8_t v[] = {r, g, b};
        int sum = 0;
        for (int i = 0; i < 3; ++i) {
            channel_t *ch = &state->channels[i];
            int32_t c = v[i];
            sum += c;
            if (led_active_high) {
                if (c >= led_period)
                    c = led_period - 1;
            } else {
                c = (int)led_period - c;
                if (c < 0)
                    c = 0;
            }

            // set duty first, in case jd_pwm_enable() is not impl.
            jd_pwm_set_duty(ch->pwm, c);
            if (c == 0) {
                pin_set(ch->pin, !led_active_high);
                jd_pwm_enable(ch->pwm, 0);
            } else {
                jd_pwm_enable(ch->pwm, 1);
            }
        }

        if (sum == 0 && state->in_tim) {
            pwr_leave_tim();
            state->in_tim = 0;
        } else if (sum != 0 && !state->in_tim) {
            pwr_enter_tim();
            state->in_tim = 1;
        }
    } else {
        int fl = r + g + b;
        channel_t *ch = &state->channels[0];
        if (fl) {
            state->in_tim = 1;
            pin_set(ch->pin, led_active_high);
        } else {
            state->in_tim = 0;
            pin_set(ch->pin, !led_active_high);
        }
    }
}

bool jd_status_has_color(void) {
    return has_led && !is_mono;
}

#endif

#endif
