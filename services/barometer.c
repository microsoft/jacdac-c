// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_sensor_api.h"
#include "jacdac/dist/c/airpressure.h"

struct srv_state {
    SENSOR_COMMON;
};

void barometer_process(srv_t *state) {
    env_sensor_process(state);
}

void barometer_handle_packet(srv_t *state, jd_packet_t *pkt) {
    env_sensor_handle_packet(state, pkt);
}

SRV_DEF(barometer, JD_SERVICE_CLASS_AIR_PRESSURE);

void barometer_init(const env_sensor_api_t *api) {
    SRV_ALLOC(barometer);
    state->streaming_interval = 1000;
    state->api = api;
}
