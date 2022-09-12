// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_pins.h"
#include "interfaces/jd_adc.h"
#include "jacdac/dist/c/power.h"

#define LOG(msg, ...) DMESG("PWR: " msg, ##__VA_ARGS__)
// #define LOG JD_NOLOG

#define OVERLOAD_MS 2000 // how long to shut down the power for after overload, in ms

#define MS(v) ((v) << 10)

#if JD_RAW_FRAME

static uint8_t shutdown_frame[16] = {0x15, 0x59, 0x04, 0x05, 0x5A, 0xC9, 0xA4, 0x1F,
                                     0xAA, 0xAA, 0xAA, 0xAA, 0x00, 0x3D, 0x80, 0x00};

struct srv_state {
    SRV_COMMON;
    uint8_t allowed;
    uint8_t power_status;
    uint16_t max_power;
    uint16_t pulse_duration;
    uint32_t pulse_period;

    const power_config_t *cfg;
    uint8_t prev_power_status;
    uint8_t hw_watchdog_pulse_status;

    // timers
    uint32_t next_shutdown;
    uint32_t last_pulse;
    uint32_t next_leds;
    uint32_t power_switch_time;
    uint32_t power_on_complete;
    uint32_t re_enable;
    uint32_t hw_watchdog_pulse;
#ifdef PIN_PWR_LED_PULSE
    uint32_t led_pulse_on;
    uint32_t led_pulse_off;
#endif
};

REG_DEFINITION(                                   //
    power_regs,                                   //
    REG_SRV_COMMON,                               //
    REG_U8(JD_POWER_REG_ALLOWED),                 //
    REG_U8(JD_POWER_REG_POWER_STATUS),            //
    REG_U16(JD_POWER_REG_MAX_POWER),              //
    REG_U16(JD_POWER_REG_KEEP_ON_PULSE_DURATION), //
    REG_U32(JD_POWER_REG_KEEP_ON_PULSE_PERIOD),   //
)

#define HW_WATCHDOG() (state->cfg->en_active_high == 3)

static void set_limiter(srv_t *state) {
    int onoff = state->power_status == JD_POWER_POWER_STATUS_POWERING;
    LOG("lim: %d", onoff);

    if (HW_WATCHDOG()) {
        // do nothing
    } else if (state->cfg->en_active_high == 2) {
        if (onoff) {
            pin_set(state->cfg->pin_en, 0);
            pin_set(state->cfg->pin_fault, 1);
            pin_setup_output(state->cfg->pin_fault);
            target_wait_us(1000);
            pin_setup_input(state->cfg->pin_fault, PIN_PULL_UP);
        } else {
            pin_set(state->cfg->pin_en, 1);
        }
    } else {
        int val = state->cfg->en_active_high ? onoff : !onoff;
        pin_set(state->cfg->pin_en, val);
    }
}

#define GLOW_CH CH_0
static uint32_t glows[] = {
    [JD_POWER_POWER_STATUS_OVERLOAD] = JD_GLOW(VERY_SLOW_LOW, GLOW_CH, MAGENTA),
    [JD_POWER_POWER_STATUS_OVERPROVISION] = JD_GLOW_OFF(GLOW_CH),
    [JD_POWER_POWER_STATUS_DISALLOWED] = JD_GLOW_OFF(GLOW_CH),
#ifdef PIN_PWR_LED_PULSE
    [JD_POWER_POWER_STATUS_POWERING] = JD_GLOW_OFF(GLOW_CH),
#else
    [JD_POWER_POWER_STATUS_POWERING] = JD_GLOW(VERY_SLOW_LOW, GLOW_CH, GREEN),
#endif
};

static void set_leds(srv_t *state) {
    jd_glow(glows[state->power_status]);
}

static bool shutdown_pending(void) {
    return rawFrame != NULL || rawFrameSending;
}

static void send_shutdown(srv_t *state) {
    rawFrame = (jd_frame_t *)shutdown_frame;
    jd_packet_ready();
}

static int has_power(void) {
#ifdef PIN_PWR_DET
    pin_setup_input(PIN_PWR_DET, PIN_PULL_NONE);
    return pin_get(PIN_PWR_DET);
#else
    return 1;
#endif
}

void power_process(srv_t *state) {
    if (!has_power()) {
        if (state->power_status == JD_POWER_POWER_STATUS_POWERING) {
            LOG("no power");
            state->power_status = JD_POWER_POWER_STATUS_DISALLOWED; // TODO - use some other state
        }
    } else {
        if (state->allowed && state->power_status == JD_POWER_POWER_STATUS_DISALLOWED) {
            LOG("power returned");
            state->power_status = JD_POWER_POWER_STATUS_POWERING;
        }
    }

#ifdef PIN_PWR_LED_PULSE
    if (jd_should_sample(&state->led_pulse_on, 2 << 20)) {
        state->led_pulse_off = now + (50 << 10);
        if (state->power_status == JD_POWER_POWER_STATUS_POWERING) {
            pin_set(PIN_PWR_LED_PULSE, 0);
            pin_setup_output(PIN_PWR_LED_PULSE);
        }
    }
    if (in_past(state->led_pulse_off)) {
        pin_setup_analog_input(PIN_PWR_LED_PULSE);
        state->led_pulse_off = now + (50 << 20);
    }
#endif

    if (HW_WATCHDOG() && jd_should_sample(&state->hw_watchdog_pulse, 9500)) {
        if (state->power_status == JD_POWER_POWER_STATUS_OVERLOAD ||
            state->power_status == JD_POWER_POWER_STATUS_POWERING) {
            pin_set(state->cfg->pin_en, state->hw_watchdog_pulse_status);
            state->hw_watchdog_pulse_status = !state->hw_watchdog_pulse_status;
        } else {
            pin_set(state->cfg->pin_en, 0);
        }
    }

    if (jd_should_sample(&state->next_leds, 256 * 1024)) {
        set_leds(state);
    }

    if (state->pulse_period && state->pulse_duration) {
        uint32_t pulse_delta = now - state->last_pulse;
        if (pulse_delta > state->pulse_period * 1000) {
            pulse_delta = 0;
            state->last_pulse = now;
        }
        pin_set(state->cfg->pin_pulse, pulse_delta < state->pulse_duration * 1000);
    }

    if (HW_WATCHDOG() && state->power_status == JD_POWER_POWER_STATUS_OVERLOAD &&
        pin_get(state->cfg->pin_fault))
        state->power_status = JD_POWER_POWER_STATUS_POWERING;

    if (state->power_status == JD_POWER_POWER_STATUS_POWERING) {
        if (state->prev_power_status == JD_POWER_POWER_STATUS_POWERING) {
            // already in steady state
            if (!pin_get(state->cfg->pin_fault)) {
                // fault!
                state->power_status = JD_POWER_POWER_STATUS_OVERLOAD;
                state->re_enable = now + jd_random_around(OVERLOAD_MS * 1000);
                set_limiter(state);
            }
        } else {
            if (!state->power_on_complete) {
                set_limiter(state);
                int ign = state->cfg->fault_ignore_ms;
                state->power_on_complete = now + MS(ign ? ign : 16); // some time to ramp up
                if (!state->power_on_complete)                       // unlikely wrap-around
                    state->power_on_complete = 1;
                return;
            }

            if (in_future(state->power_on_complete))
                return; // still waiting for it to ramp up
        }
    } else {
        if (state->prev_power_status != state->power_status)
            set_limiter(state);
    }

    state->power_on_complete = 0;

    if (state->prev_power_status != state->power_status) {
        LOG("status %d -> %d", state->prev_power_status, state->power_status);
        state->prev_power_status = state->power_status;
        jd_send_event_ext(state, JD_POWER_EV_POWER_STATUS_CHANGED, &state->power_status,
                          sizeof(state->power_status));
    }

    if (in_past(state->next_shutdown)) {
        state->next_shutdown = now + jd_random_around(MS(512));

        if (state->power_status == JD_POWER_POWER_STATUS_OVERLOAD ||
            state->power_status == JD_POWER_POWER_STATUS_POWERING) {
            send_shutdown(state);
        }
        return;
    }

    if (!state->allowed)
        return;

    if ((state->power_status != JD_POWER_POWER_STATUS_POWERING && in_past(state->re_enable)) ||
        (state->power_status == JD_POWER_POWER_STATUS_OVERPROVISION &&
         in_past(state->power_switch_time))) {
        state->re_enable = now + MS(128 * 1024); // very distant future
        state->power_status = JD_POWER_POWER_STATUS_POWERING;
        state->next_shutdown = now;
        return;
    }
}

void power_handle_packet(srv_t *state, jd_packet_t *pkt) {
    switch (pkt->service_command) {
    case JD_SET(JD_POWER_REG_MAX_POWER):
        return; // ignore writing to max_power
    case JD_POWER_CMD_SHUTDOWN:
        if (pkt->flags & JD_FRAME_FLAG_LOOPBACK) // doesn't actually happen...
            return;                              // our own!
        if ((state->power_status == JD_POWER_POWER_STATUS_POWERING ||
             state->power_status == JD_POWER_POWER_STATUS_OVERLOAD) &&
            !shutdown_pending() && state->power_on_complete == 0) {
            state->power_status = JD_POWER_POWER_STATUS_OVERPROVISION;
            state->power_switch_time = now + jd_random_around(MS(8 * 1024)); // around 8s
        }
        // note that we'll keep getting shutdown commands - if we're already over-provisioned
        // we'll just keep shifting the re_enable timer forward, so it never triggers
        // Once the shutdown packets stop, we won't wait for power_switch_time, but only
        // for the re_enable
        state->re_enable = now + MS(1024) + jd_random_around(MS(256));
        return;
    }

    if (state->cfg->pin_pulse == NO_PIN) {
        // disable keep_on* regs when no pulse pin
        if (jd_block_register(pkt, JD_POWER_REG_KEEP_ON_PULSE_DURATION) ||
            jd_block_register(pkt, JD_POWER_REG_KEEP_ON_PULSE_PERIOD))
            return;
    }

    switch (service_handle_register_final(state, pkt, power_regs)) {
    case JD_POWER_REG_ALLOWED:
        LOG("allowed=%d", state->allowed);
        state->power_status =
            state->allowed ? JD_POWER_POWER_STATUS_POWERING : JD_POWER_POWER_STATUS_DISALLOWED;
        break;
    case JD_POWER_REG_KEEP_ON_PULSE_PERIOD:
    case JD_POWER_REG_KEEP_ON_PULSE_DURATION:
        if (state->pulse_period && state->pulse_duration) {
            // assuming 22R in 0805, we get around 1W or power dissipation, but can withstand only
            // 1/8W continuos so we limit duty cycle to 10%, and make sure it doesn't stay on for
            // more than 1s
            if (state->pulse_duration > 1000)
                state->pulse_duration = 1000;
            if (state->pulse_period < state->pulse_duration * 10)
                state->pulse_period = state->pulse_duration * 10;
        }
        break;
    }
}

SRV_DEF(power, JD_SERVICE_CLASS_POWER);
void power_init(const power_config_t *cfg) {
    SRV_ALLOC(power);
    state->cfg = cfg;

    pin_setup_input(cfg->pin_fault, HW_WATCHDOG() ? PIN_PULL_NONE : PIN_PULL_UP);
    pin_setup_output(cfg->pin_en);
    pin_set(cfg->pin_pulse, 1);
    pin_setup_output(cfg->pin_pulse);

    state->power_status = JD_POWER_POWER_STATUS_POWERING;
    set_limiter(state);

    state->next_shutdown = now + jd_random_around(MS(256));

    state->allowed = 1;
    state->max_power = 900;

    // These should be reasonable defaults.
    // Only 1 out of 14 batteries tested wouldn't work with these settings.
    // 0.6/20s is 7mA (at 22R), so ~6 weeks on 10000mAh battery (mAh are quoted for 3.7V not 5V).
    // These can be tuned by the user for better battery life.
    // Note that the power_process() above at 1kHz takes about 1mA on its own.
    if (cfg->pin_pulse != NO_PIN) {
        state->pulse_duration = 600;
        state->pulse_period = 20000;
        state->last_pulse = now - state->pulse_duration * 1000;
    }

    if (cfg->en_active_high < 2)
        tim_max_sleep = 1000; // wakeup at 1ms not usual 10ms to get faster over-current reaction
}

#endif