// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_sensor_api.h"
#include "jacdac/dist/c/humidity.h"

struct srv_state {
    SENSOR_COMMON;
};

void humidity_process(srv_t *state) {
    env_sensor_process(state);
}

void humidity_handle_packet(srv_t *state, jd_packet_t *pkt) {
    env_sensor_handle_packet(state, pkt);
}

SRV_DEF(humidity, JD_SERVICE_CLASS_HUMIDITY);
void humidity_init(const env_sensor_api_t *api) {
    SRV_ALLOC(humidity);
    state->streaming_interval = 1000;
    state->api = api;
}
