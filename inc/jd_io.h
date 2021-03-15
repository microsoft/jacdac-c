// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include "jd_config.h"

void jd_led_set(int state);
void jd_led_blink(int us);
void jd_power_enable(int en);

#if JD_CONFIG_STATUS == 1
void jd_status_init(void);
void jd_status_process(void);
void jd_status_handle_packet(jd_packet_t *pkt);
#endif

#define JD_STATUS_OFF 0
#define JD_STATUS_STARTUP 1
#define JD_STATUS_CONNECTED 2
#define JD_STATUS_DISCONNECTED 3
#define JD_STATUS_IDENTIFY 4

// if disabled with JD_CONFIG_STATUS==0, the user has to provide their own impl.
void jd_status(int status);
