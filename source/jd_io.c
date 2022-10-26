// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_protocol.h"

#if JD_PHYSICAL

void jd_power_enable(int en) {
    power_pin_enable(en);
}

#endif