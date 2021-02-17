// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_protocol.h"
#include "interfaces/jd_sensor.h"
#include "interfaces/jd_environment.h"
#include "jacdac/dist/c/humidity.h"

struct srv_state {
    SENSOR_COMMON;
};

void humidity_process(srv_t *state) {
    env_sensor_process(state, env_humidity);
}

void humidity_handle_packet(srv_t *state, jd_packet_t *pkt) {
    env_sensor_handle_packet(state, pkt, env_humidity);
}

SRV_DEF(humidity, JD_SERVICE_CLASS_HUMIDITY);
void humidity_init(void) {
    SRV_ALLOC(humidity);
    state->streaming_interval = 1000;
}
