// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_hid.h"
#include "jacdac/dist/c/hidkeyboard.h"

#if JD_HID

#define STEP_MS 20

struct srv_state {
    SRV_COMMON;

    uint8_t steps_left[JD_HID_MAX_KEYCODES];
    jd_hid_keyboard_report_t prev_r;
    jd_hid_keyboard_report_t r;

    uint16_t num_skips;
    uint32_t next_tick;
};

REG_DEFINITION(       //
    hidkeyboard_regs, //
    REG_SRV_COMMON,   //
)

static bool is_empty(srv_t *state) {
    for (int i = 0; i < JD_HID_MAX_KEYCODES; ++i) {
        if (state->r.keycode[i])
            return 0;
    }
    return 1;
}

void hidkeyboard_process(srv_t *state) {
    if (!jd_should_sample_delay(&state->next_tick, STEP_MS * 1000))
        return;

    if (is_empty(state))
        state->r.modifier = 0;

    if (state->num_skips > (2000 / STEP_MS) ||
        memcmp(&state->r, &state->prev_r, sizeof(state->r)) != 0) {
        memcpy(&state->prev_r, &state->r, sizeof(state->r));
        jd_hid_keyboard_set_report(&state->r);
        state->num_skips = 0;
    } else {
        state->num_skips++;
    }

    for (int i = 0; i < JD_HID_MAX_KEYCODES; ++i) {
        if (state->steps_left[i] && --state->steps_left[i] == 0)
            state->r.keycode[i] = 0;
    }
}

static int index_of_key(srv_t *state, int k) {
    for (int i = 0; i < JD_HID_MAX_KEYCODES; ++i) {
        if (state->r.keycode[i] == k)
            return i;
    }
    return -1;
}

void hidkeyboard_handle_packet(srv_t *state, jd_packet_t *pkt) {
    switch (pkt->service_command) {
    case JD_HID_KEYBOARD_CMD_CLEAR:
        memset(&state->r, 0, sizeof(state->r));
        break;

    case JD_HID_KEYBOARD_CMD_KEY:
        if (pkt->service_size >= sizeof(jd_hid_keyboard_key_t)) {
            jd_hid_keyboard_key_t *d = (void *)pkt->data;
            state->r.modifier = d->modifiers;
            if (d->action <= JD_HID_KEYBOARD_ACTION_DOWN && 0 < d->selector && d->selector < 0xE0) {
                if (d->action == JD_HID_KEYBOARD_ACTION_UP) {
                    int idx = index_of_key(state, d->selector);
                    if (idx >= 0)
                        state->r.keycode[idx] = 0;
                } else {
                    int idx = index_of_key(state, d->selector);
                    if (idx < 0)
                        idx = index_of_key(state, 0);
                    if (idx < 0)
                        idx = 0; // overwrite first
                    state->r.keycode[idx] = d->selector;
                    if (d->action == JD_HID_KEYBOARD_ACTION_DOWN)
                        state->steps_left[idx] = 0; // "infinity"
                    else
                        state->steps_left[idx] = 100 / STEP_MS;
                }
            }
        }
        break;

    default:
        service_handle_register_final(state, pkt, hidkeyboard_regs);
        break;
    }
}

SRV_DEF(hidkeyboard, JD_SERVICE_CLASS_HID_KEYBOARD);
void hidkeyboard_init(void) {
    SRV_ALLOC(hidkeyboard);
    state->num_skips = 0xffff; // force send
}

#endif