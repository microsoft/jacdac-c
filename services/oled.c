// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_oled.h"

struct srv_state {
    SRV_COMMON;
    uint8_t intensity;
    oled_state_t oled;
};

REG_DEFINITION(               //
    oled_regs,                //
    REG_SRV_COMMON,           //
    REG_U8(JD_REG_INTENSITY), //
)

void oled_process(srv_t *state) {}

void oled_handle_packet(srv_t *state, jd_packet_t *pkt) {
    if (service_handle_register(state, pkt, oled_regs)) {
    }
}

SRV_DEF(oled, JD_SERVICE_CLASS_MONO_DISPLAY);
void oled_init(void) {
    SRV_ALLOC(oled);
    oled_setup(&state->oled);
}
