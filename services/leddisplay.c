// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_pixel.h"
#include "interfaces/jd_hw_pwr.h"
#include "jacdac/dist/c/led.h"

#define DEFAULT_INTENSITY 15

// #define LOG JD_LOG
#define LOG JD_NOLOG

#define PX_WORDS(NUM_PIXELS) (((NUM_PIXELS)*3 + 3) / 4)

typedef union {
    struct {
        uint8_t b;
        uint8_t g;
        uint8_t r;
        uint8_t _a; // not really used
    };
    uint32_t val;
} RGB;

REG_DEFINITION(                           //
    leddisplay_regs,                      //
    REG_SRV_COMMON,                       //
    REG_U8(JD_LED_REG_BRIGHTNESS),        //
    REG_U8(JD_LED_REG_ACTUAL_BRIGHTNESS), //
    REG_U8(JD_LED_REG_VARIANT),           //
    REG_U16(JD_LED_REG_NUM_PIXELS),       //
    REG_U16(JD_LED_REG_MAX_POWER),        //
)

struct srv_state {
    SRV_COMMON;

    uint8_t requested_intensity;
    uint8_t intensity;
    uint8_t variant;
    uint16_t numpixels;
    uint16_t maxpower;

    // end of registers
    uint8_t leddisplay_type;
    uint32_t auto_refresh;
    uint8_t pxbuffer[JD_SERIAL_PAYLOAD_SIZE];
    volatile uint8_t in_tx;
    volatile uint8_t dirty;
    uint8_t needs_clear;
};

static srv_t *state_;

#include "led_internal.h"

void leddisplay_process(srv_t *state) {
    if (in_past(state->auto_refresh))
        state->dirty = 1;

    if (!is_enabled(state) && !state->needs_clear)
        return;

    if (state->dirty && !state->in_tx) {
        state->dirty = 0;
        state->auto_refresh = now + (64 << 10);
        if (!state->needs_clear &&
            is_empty((uint32_t *)state->pxbuffer, PX_WORDS(state->numpixels))) {
            jd_power_enable(0);
            return;
        } else {
            jd_power_enable(1);
        }
        state->in_tx = 1;
        state->needs_clear = 0;
        pwr_enter_pll();
        limit_intensity(state);
        px_tx(state->pxbuffer, state->numpixels * 3, state->intensity, tx_done);
    }
}

static void handle_set(srv_t *state, jd_packet_t *pkt) {
    int sz = pkt->service_size;
    if (sz > state->numpixels * 3)
        sz = state->numpixels * 3;
    memcpy(state->pxbuffer, pkt->data, sz);
    state->dirty = 1;
}

void leddisplay_handle_packet(srv_t *state, jd_packet_t *pkt) {
    LOG("cmd: %x", pkt->service_command);

    switch (pkt->service_command) {
    case JD_GET(JD_LED_REG_PIXELS):
        jd_send(pkt->service_index, pkt->service_command, state->pxbuffer, state->numpixels * 3);
        break;
    case JD_SET(JD_LED_REG_PIXELS):
        handle_set(state, pkt);
        break;
    default:
        switch (service_handle_register_final(state, pkt, leddisplay_regs)) {
        case JD_LED_REG_BRIGHTNESS:
            state->intensity = state->requested_intensity;
            break;
        }
        break;
    }

    if (is_empty((uint32_t *)state->pxbuffer, PX_WORDS(state->numpixels)) || !is_enabled(state))
        state->needs_clear = 1;
}

SRV_DEF(leddisplay, JD_SERVICE_CLASS_LED);
void leddisplay_init(uint8_t leddisplay_type, uint32_t num_pixels, uint32_t default_max_power,
                     uint8_t variant) {
    SRV_ALLOC(leddisplay);
    if (num_pixels * 3 > JD_SERIAL_PAYLOAD_SIZE)
        JD_PANIC();
    state_ = state; // there is global singleton state
    state->leddisplay_type = leddisplay_type;
    state->numpixels = num_pixels;
    state->maxpower = default_max_power;
    state->variant = variant;
    state->intensity = state->requested_intensity = DEFAULT_INTENSITY;

    px_alloc();
    px_init(state->leddisplay_type);

    // clear out display
    state->in_tx = 1;
    pwr_enter_pll();
    px_tx(state->pxbuffer, state->numpixels * 3, state->intensity, tx_done);
}
