// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_sensor_api.h"
#include "jacdac/dist/c/eco2.h"

struct srv_state {
    SENSOR_COMMON;
};

void eco2_process(srv_t *state) {
    env_sensor_process(state);
}

void eco2_handle_packet(srv_t *state, jd_packet_t *pkt) {
    env_sensor_handle_packet(state, pkt);
}

SRV_DEF(eco2, JD_SERVICE_CLASS_E_CO2);

void eco2_init(const env_sensor_api_t *api) {
    SRV_ALLOC(eco2);
    state->streaming_interval = 1000;
    state->api = api;
}
