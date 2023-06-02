// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_protocol.h"

#if JD_PHYSICAL

void _jd_phys_start(void);

void jd_init(void) {
    jd_alloc_init();
    jd_tx_init();
    jd_rx_init();

#if JD_CONFIG_STATUS == 1
    jd_status_init();
#endif

    jd_services_init();
    // if we don't start it explicitly, it will only start on incoming packet
    _jd_phys_start();

    jd_blink(JD_BLINK_STARTUP);
}

#endif