// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_lora.h"
// #include "jacdac/dist/c/lorawan.h"

#if JD_LORA

struct srv_state {
    SRV_COMMON;
};

void lorawan_process(srv_t *state) {
    jd_lora_process();
}

void lorawan_handle_packet(srv_t *state, jd_packet_t *pkt) {
    jd_send_not_implemented(pkt);
}

SRV_DEF(lorawan, 0x1953440b);

void lorawan_init() {
    SRV_ALLOC(lorawan);
    jd_lora_init();
}

#endif