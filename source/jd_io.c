// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_protocol.h"

#if JD_PHYSICAL

void jd_power_enable(int en) {
    power_pin_enable(en);
}

#endif

uint32_t jd_max_sleep;
void jd_set_max_sleep(uint32_t us)
{
    if (jd_max_sleep > us)
        jd_max_sleep = us;
}
