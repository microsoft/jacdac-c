// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_sensor_api.h"
#include "jacdac/dist/c/tvoc.h"

struct srv_state {
    SENSOR_COMMON;
    const env_sensor_api_t *api;
};

void tvoc_process(srv_t *state) {
    env_sensor_process(state, state->api);
}

void tvoc_handle_packet(srv_t *state, jd_packet_t *pkt) {
    env_sensor_handle_packet(state, pkt, state->api);
}

SRV_DEF(tvoc, JD_SERVICE_CLASS_TVOC);

void tvoc_init(const env_sensor_api_t *api) {
    SRV_ALLOC(tvoc);
    state->streaming_interval = 1000;
    state->api = api;
}
