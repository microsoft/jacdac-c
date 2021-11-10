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

#define JD_GET(reg) (JD_CMD_GET_REGISTER | (reg))
#define JD_SET(reg) (JD_CMD_SET_REGISTER | (reg))

#define JD_IS_GET(cmd) (((cmd) >> 12) == (JD_CMD_GET_REGISTER >> 12))
#define JD_IS_SET(cmd) (((cmd) >> 12) == (JD_CMD_SET_REGISTER >> 12))
#define JD_REG_CODE(cmd) ((cmd)&0xfff)

void jd_ctrl_init(void);
void jd_ctrl_process(srv_t *_state);
void jd_ctrl_handle_packet(srv_t *_state, jd_packet_t *pkt);
void dump_pkt(jd_packet_t *pkt, const char *msg);
