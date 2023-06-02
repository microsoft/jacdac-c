// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_lora.h"
// #include "jacdac/dist/c/lorawan.h"

#if JD_LORA

struct srv_state {
    SRV_COMMON;
    uint32_t next_tx;
};

#define DUTY_MS 3000

void lorawan_process(srv_t *state) {
    jd_lora_process();
    if (jd_should_sample(&state->next_tx, DUTY_MS * 1000)) {
        jd_lora_send(&now, 4);
    }
}

void lorawan_handle_packet(srv_t *state, jd_packet_t *pkt) {
    jd_send_not_implemented(pkt);
}

SRV_DEF(lorawan, 0x1953440b);

void lorawan_init(void) {
    SRV_ALLOC(lorawan);
    jd_lora_init();
}

#endif