// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef JD_IO_H
#define JD_IO_H

#include "jd_config.h"

void jd_led_set(int state);
void jd_led_blink(int us);
void jd_power_enable(int en);

#if JD_CONFIG_STATUS == 1
void jd_status_init(void);
void jd_status_process(void);
int jd_status_handle_packet(jd_packet_t *pkt);
void jd_status_set_ch(int ch, uint8_t v);
#endif

// sync with jd_status_animations[]
#define JD_STATUS_OFF 0
#define JD_STATUS_STARTUP 1
#define JD_STATUS_CONNECTED 2
#define JD_STATUS_DISCONNECTED 3
#define JD_STATUS_IDENTIFY 4
#define JD_STATUS_UNKNOWN_STATE 5

// if disabled with JD_CONFIG_STATUS==0, the user has to provide their own impl.
void jd_status(int status);

#endif
