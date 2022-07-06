// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_hid.h"
#include "jacdac/dist/c/hidjoystick.h"

#define STEP_MS 20

// two analog (triggers - z, rz), 16 normal, 4 directions
#define NUM_BUTTONS (2 + 16 + 4)
#define NUM_AXIS 4
#define BTN_ANALOG (0x3)

struct srv_state {
    SRV_COMMON;

    uint32_t button_analog;
    uint8_t button_count;
    uint8_t axis_count;

    jd_hid_gamepad_report_t prev_r;
    jd_hid_gamepad_report_t r;

    uint16_t num_skips;
    uint32_t next_tick;
};

REG_DEFINITION(                                  //
    hidjoystick_regs,                            //
    REG_SRV_COMMON,                              //
    REG_U32(JD_HID_JOYSTICK_REG_BUTTONS_ANALOG), //
    REG_U8(JD_HID_JOYSTICK_REG_BUTTON_COUNT),    //
    REG_U8(JD_HID_JOYSTICK_REG_AXIS_COUNT),      //
)

void hidjoystick_process(srv_t *state) {
    if (!jd_should_sample_delay(&state->next_tick, STEP_MS * 1000))
        return;

    if (state->num_skips > (2000 / STEP_MS) ||
        memcmp(&state->r, &state->prev_r, sizeof(state->r)) != 0) {
        memcpy(&state->prev_r, &state->r, sizeof(state->r));
        jd_hid_gamepad_set_report(&state->r);
        state->num_skips = 0;
    } else {
        state->num_skips++;
    }
}

void hidjoystick_handle_packet(srv_t *state, jd_packet_t *pkt) {
    switch (pkt->service_command) {
    case JD_HID_JOYSTICK_CMD_SET_AXIS: {
        int8_t *ax = &state->r.x;
        int16_t *d = (int16_t *)pkt->data;
        for (int i = 0; i < NUM_AXIS; i++) {
            if (i * 2 + 1 < pkt->service_size)
                ax[i] = d[i] >> 8;
            else
                ax[i] = 0;
        }
    } break;

    case JD_HID_JOYSTICK_CMD_SET_BUTTONS:
        state->r.buttons = 0;
        state->r.hat = 0;
        state->r.z = 0;
        state->r.rz = 0;
        for (int i = 0; i < pkt->service_size; ++i) {
            if (i == 0)
                state->r.z = pkt->data[0] >> 1;
            else if (i == 1)
                state->r.rz = pkt->data[0] >> 1;
            else if (pkt->data[i]) {
                i -= 2;
                if (i < 16)
                    state->r.buttons |= (1 << i);
                else {
                    i -= 16;
                    if (i < 4)
                        state->r.hat |= (1 << i);
                }
            }
        }
        break;

    default:
        service_handle_register_final(state, pkt, hidjoystick_regs);
        break;
    }
}

SRV_DEF(hidjoystick, JD_SERVICE_CLASS_HID_JOYSTICK);
void hidjoystick_init() {
    SRV_ALLOC(hidjoystick);
    state->button_analog = BTN_ANALOG;
    state->button_count = NUM_BUTTONS;
    state->axis_count = NUM_AXIS;
    state->num_skips = 0xffff; // force send
}
