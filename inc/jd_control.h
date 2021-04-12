// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include "jd_config.h"
#include "jd_physical.h"
#include "jd_service_framework.h"

#include "jacdac/dist/c/_base.h"
#include "jacdac/dist/c/_system.h"
#include "jacdac/dist/c/_sensor.h"
#include "jacdac/dist/c/control.h"

#define JD_ADVERTISEMENT_0_COUNTER_MASK 0x0000000F

#define JD_ADVERTISEMENT_0_STATUS_LIGHT_MASK 0x00000030
#define JD_ADVERTISEMENT_0_STATUS_LIGHT_NONE 0x00000000 // or status_light cmd not supported
#define JD_ADVERTISEMENT_0_STATUS_LIGHT_MONO 0x00000010
#define JD_ADVERTISEMENT_0_STATUS_LIGHT_RGB_NO_FADE 0x00000020
#define JD_ADVERTISEMENT_0_STATUS_LIGHT_RGB_FADE 0x00000030

#define JD_ADVERTISEMENT_0_ACK_SUPPORTED 0x00000100
#define JD_ADVERTISEMENT_0_IDENTIFIER_IS_SERVICE_CLASS_SUPPORTED                                   \
    0x00000200                                         // "broadcast" frame flag
#define JD_ADVERTISEMENT_0_FRAMES_SUPPORTED 0x00000400 // can have multiple packets per frame
#define JD_ADVERTISEMENT_0_IS_CLIENT 0x00000800        // typically a brain

#define JD_GET(reg) (JD_CMD_GET_REGISTER | (reg))
#define JD_SET(reg) (JD_CMD_SET_REGISTER | (reg))

void jd_ctrl_init(void);
void jd_ctrl_process(srv_t *_state);
void jd_ctrl_handle_packet(srv_t *_state, jd_packet_t *pkt);
void dump_pkt(jd_packet_t *pkt, const char *msg);
