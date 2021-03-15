// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_protocol.h"

void _jd_phys_start(void);

void jd_init() {
    jd_alloc_init();
    jd_tx_init();
    jd_rx_init();
    jd_services_init();
    // if we don't start it explicitly, it will only start on incoming packet
    _jd_phys_start();

#if JD_CONFIG_STATUS == 1
    jd_status_init();
#endif
    jd_status(JD_STATUS_STARTUP);
}