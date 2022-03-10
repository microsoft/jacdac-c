// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_sensor_api.h"
#include "jacdac/dist/c/illuminance.h"

struct srv_state {
    SENSOR_COMMON;
};

void illuminance_process(srv_t *state) {
    env_sensor_process(state);
}

void illuminance_handle_packet(srv_t *state, jd_packet_t *pkt) {
    env_sensor_handle_packet(state, pkt);
}

SRV_DEF(illuminance, JD_SERVICE_CLASS_ILLUMINANCE);

void illuminance_init(const env_sensor_api_t *api) {
    SRV_ALLOC(illuminance);
    state->streaming_interval = 500;
    state->api = api;
}
