// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_protocol.h"

void jd_led_set(int state)
{
    led_set(state);
}

void jd_led_blink(int us)
{
    led_blink(us);
}

void jd_power_enable(int en)
{
    power_pin_enable(en);
}