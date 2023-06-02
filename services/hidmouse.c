// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_hid.h"
#include "jacdac/dist/c/hidmouse.h"

#if JD_HID

#define STEP_MS 20
#define NUM_BUTTONS 3
#define NUM_AXIS 4

#define BTN_OFF 0

#define BTN_CLICK_BEG 1
#define BTN_CLICK_OFF (1 + (100 / STEP_MS))

#define BTN_DBL_CLICK_BEG 50
#define BTN_DBL_CLICK_PAUSE (BTN_DBL_CLICK_BEG + (100 / STEP_MS))
#define BTN_DBL_CLICK_PAUSE_END (BTN_DBL_CLICK_BEG + (250 / STEP_MS))
#define BTN_DBL_CLICK_END (BTN_DBL_CLICK_BEG + (350 / STEP_MS))

#define BTN_ON 0xff

static const uint8_t state_map[] = {
    [JD_HID_MOUSE_BUTTON_EVENT_UP] = BTN_OFF,
    [JD_HID_MOUSE_BUTTON_EVENT_DOWN] = BTN_ON,
    [JD_HID_MOUSE_BUTTON_EVENT_CLICK] = BTN_CLICK_BEG,
    [JD_HID_MOUSE_BUTTON_EVENT_DOUBLE_CLICK] = BTN_DBL_CLICK_BEG,
};

static inline bool is_btn_on(uint8_t b) {
    return b != BTN_OFF && !(BTN_DBL_CLICK_PAUSE <= b && b < BTN_DBL_CLICK_PAUSE_END);
}

static inline bool is_btn_final(uint8_t b) {
    return b == BTN_CLICK_OFF || b == BTN_DBL_CLICK_END || b == BTN_OFF;
}

static int next_btn_state(uint8_t b) {
    if (is_btn_final(b))
        return BTN_OFF;
    if (b == BTN_ON)
        return b;
    return b + 1;
}

typedef struct {
    uint8_t state;
} button_t;

typedef struct {
    int32_t target;
    int8_t step;
} axis_t;

struct srv_state {
    SRV_COMMON;

    axis_t axis[NUM_AXIS]; // x, y, wheel, pan
    button_t buttons[NUM_BUTTONS];
    bool last_zero;
    uint32_t next_tick;
};

REG_DEFINITION(     //
    hidmouse_regs,  //
    REG_SRV_COMMON, //
)

static inline int myabs(int v) {
    return v < 0 ? -v : v;
}

static void compute_step(axis_t *ax, int16_t d, uint16_t time) {
    ax->target += d;
    if (time == 0)
        time = 1;
    int per_step = (STEP_MS * ax->target) / time;
    if (per_step < -127)
        ax->step = -127;
    else if (per_step > 127)
        ax->step = 127;
    else
        ax->step = per_step;
}

static int8_t current_step(axis_t *ax) {
    int step = ax->step;
    if (myabs(ax->target) < myabs(step))
        step = ax->target;
    ax->target -= step;
    return step;
}

static bool is_zero_report(jd_hid_mouse_report_t *r) {
    uint8_t *p = (void *)r;
    for (unsigned i = 0; i < sizeof(*r); ++i)
        if (p[i] != 0)
            return 0;
    return 1;
}

void hidmouse_process(srv_t *state) {
    if (!jd_should_sample_delay(&state->next_tick, STEP_MS * 1000))
        return;

    jd_hid_mouse_report_t r;
    memset(&r, 0, sizeof(r));

    for (int i = 0; i < NUM_BUTTONS; ++i) {
        button_t *b = &state->buttons[i];
        if (is_btn_on(b->state))
            r.buttons |= 1 << i;
        b->state = next_btn_state(b->state);
    }

    r.x = current_step(&state->axis[0]);
    r.y = current_step(&state->axis[1]);
    r.wheel = current_step(&state->axis[2]);
    r.pan = current_step(&state->axis[3]);

    bool is_zero = is_zero_report(&r);
    if (is_zero && state->last_zero)
        return;
    state->last_zero = is_zero;
    jd_hid_mouse_move(&r);
}

void hidmouse_handle_packet(srv_t *state, jd_packet_t *pkt) {
    switch (pkt->service_command) {
    case JD_HID_MOUSE_CMD_MOVE:
        if (pkt->service_size >= sizeof(jd_hid_mouse_move_t)) {
            jd_hid_mouse_move_t *d = (void *)pkt->data;
            compute_step(&state->axis[0], d->dx, d->time);
            compute_step(&state->axis[1], d->dy, d->time);
        }
        break;

    case JD_HID_MOUSE_CMD_WHEEL:
        if (pkt->service_size >= sizeof(jd_hid_mouse_wheel_t)) {
            jd_hid_mouse_wheel_t *d = (void *)pkt->data;
            compute_step(&state->axis[2], d->dy, d->time);
        }
        break;

    case JD_HID_MOUSE_CMD_SET_BUTTON:
        if (pkt->service_size >= sizeof(jd_hid_mouse_set_button_t) - 1) {
            jd_hid_mouse_set_button_t *d = (void *)pkt->data;
            for (int i = 0; i < NUM_BUTTONS; ++i) {
                button_t *b = &state->buttons[i];
                if (d->buttons & (1 << i)) {
                    if (d->ev < sizeof(state_map) / sizeof(state_map[0]))
                        b->state = state_map[d->ev];
                }
            }
        }
        break;

    default:
        service_handle_register_final(state, pkt, hidmouse_regs);
        break;
    }
}

SRV_DEF(hidmouse, JD_SERVICE_CLASS_HID_MOUSE);
void hidmouse_init(void) {
    SRV_ALLOC(hidmouse);
}

#endif