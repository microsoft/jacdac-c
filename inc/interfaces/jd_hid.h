// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef JD_HID_H
#define JD_HID_H

#include "jd_config.h"
#include "jd_physical.h"

#define JD_HID_MAX_KEYCODES 6

typedef struct {
    uint8_t modifier;
    uint8_t keycode[JD_HID_MAX_KEYCODES];
} jd_hid_keyboard_report_t;

typedef struct {
    uint8_t buttons;
    int8_t x, y;
    int8_t wheel, pan;
} jd_hid_mouse_report_t;

typedef struct {
    int8_t throttle0;
    int8_t throttle1;
    int8_t x0, y0;
    int8_t x1, y1;
    uint16_t buttons;
} jd_hid_gamepad_report_t;

void jd_hid_keyboard_set_report(const jd_hid_keyboard_report_t *report);
void jd_hid_mouse_move(const jd_hid_mouse_report_t *report);
void jd_hid_gamepad_set_report(const jd_hid_gamepad_report_t *report);

#endif
