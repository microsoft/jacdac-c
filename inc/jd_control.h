// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include "jd_config.h"
#include "jd_physical.h"
#include "jd_services.h"

#include "jacdac/dist/c/_base.h"
#include "jacdac/dist/c/_sensor.h"
#include "jacdac/dist/c/control.h"


#define JD_SERVICE_NUMBER_CTRL 0x00
#define JD_SERVICE_NUMBER_MASK 0x3f
#define JD_SERVICE_NUMBER_CRC_ACK 0x3f
#define JD_SERVICE_NUMBER_STREAM 0x3e

#define JD_PIPE_COUNTER_MASK 0x001f
#define JD_PIPE_CLOSE_MASK 0x0020
#define JD_PIPE_METADATA_MASK 0x0040
#define JD_PIPE_PORT_SHIFT 7

#define JD_ADVERTISEMENT_0_COUNTER_MASK 0x0000000F
#define JD_ADVERTISEMENT_0_ACK_SUPPORTED 0x00000100

#define JD_GET(reg) (JD_CMD_GET_REGISTER | (reg))
#define JD_SET(reg) (JD_CMD_SET_REGISTER | (reg))

void jd_ctrl_init(void);
void jd_ctrl_process(srv_t *_state);
void jd_ctrl_handle_packet(srv_t *_state, jd_packet_t *pkt);
void dump_pkt(jd_packet_t *pkt, const char *msg);
