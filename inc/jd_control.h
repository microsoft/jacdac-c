// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef JD_CONTROL_H
#define JD_CONTROL_H

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

static inline bool jd_is_command(jd_packet_t *pkt) {
    return (pkt->flags & JD_FRAME_FLAG_COMMAND) != 0;
}

static inline bool jd_is_report(jd_packet_t *pkt) {
    return !jd_is_command(pkt);
}

static inline bool jd_is_event(jd_packet_t *pkt) {
    return jd_is_report(pkt) && pkt->service_index <= JD_SERVICE_INDEX_MAX_NORMAL &&
           (pkt->service_command & JD_CMD_EVENT_MASK) != 0;
}

static inline int jd_event_code(jd_packet_t *pkt) {
    return jd_is_event(pkt) ? (pkt->service_command & JD_CMD_EVENT_CODE_MASK) : -1;
}

static inline bool jd_is_register_get(jd_packet_t *pkt) {
    return pkt->service_index <= JD_SERVICE_INDEX_MAX_NORMAL && JD_IS_GET(pkt->service_command);
}

#endif
