// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_pins.h"
#include "jacdac/dist/c/motion.h"

#define SAMPLING_MS 128 // overkill anyways

struct srv_state {
    SENSOR_COMMON;
    uint32_t max_distance;
    uint16_t angle;

    uint8_t moving;
    uint8_t prev_moving;
    uint32_t nextSample;
    const motion_cfg_t *cfg;
};

REG_DEFINITION(                          //
    motion_regs,                         //
    REG_SENSOR_COMMON,                   //
    REG_U32(JD_MOTION_REG_MAX_DISTANCE), //
    REG_U16(JD_MOTION_REG_ANGLE),        //
)

static void update(srv_t *state) {
    state->moving = pin_get(state->cfg->pin) != state->cfg->inactive;

    if (state->moving != state->prev_moving) {
        state->prev_moving = state->moving;
        if (state->moving) {
            jd_send_event(state, JD_MOTION_EV_MOVEMENT);
        } else {
            jd_send_event(state, JD_EV_INACTIVE);
        }
    }
}

void motion_process(srv_t *state) {
    if (jd_should_sample(&state->nextSample, SAMPLING_MS * 1024)) {
        update(state);
    }
    sensor_process_simple(state, &state->moving, sizeof(state->moving));
}

void motion_handle_packet(srv_t *state, jd_packet_t *pkt) {
    if (service_handle_variant(pkt, state->cfg->variant))
        return;

    if (service_handle_register(state, pkt, motion_regs))
        return;

    sensor_handle_packet_simple(state, pkt, &state->moving, sizeof(state->moving));
}

SRV_DEF(motion, JD_SERVICE_CLASS_MOTION);

void motion_init(const motion_cfg_t *cfg) {
    SRV_ALLOC(motion);
    state->cfg = cfg;
    state->max_distance = cfg->max_distance;
    state->angle = cfg->angle;
    pin_setup_input(state->cfg->pin, PIN_PULL_NONE);
    update(state);
}

#if JD_DCFG
void motion_config(void) {
    motion_cfg_t *p = jd_alloc(sizeof(*p));
    p->pin = jd_srvcfg_pin("pin");
    if (jd_srvcfg_has_flag("activeLow"))
        p->inactive = true;
    p->angle = jd_srvcfg_u32("angle", 120);
    p->max_distance = (jd_srvcfg_u32("maxDistance", 1200) << 12) / 25;
    motion_init(p);
}
#endif
