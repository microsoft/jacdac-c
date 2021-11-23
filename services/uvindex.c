// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_sensor_api.h"
#include "jacdac/dist/c/uvindex.h"

struct srv_state {
    SENSOR_COMMON;
};

void uvindex_process(srv_t *state) {
    env_sensor_process(state);
}

void uvindex_handle_packet(srv_t *state, jd_packet_t *pkt) {
    env_sensor_handle_packet(state, pkt);
}

SRV_DEF(uvindex, JD_SERVICE_CLASS_UV_INDEX);

void uvindex_init(const env_sensor_api_t *api) {
    SRV_ALLOC(uvindex);
    state->streaming_interval = 500;
    state->api = api;
}
