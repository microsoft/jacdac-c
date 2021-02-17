// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_protocol.h"
#include "interfaces/jd_sensor.h"
#include "interfaces/jd_environment.h"
#include "jacdac/dist/c/thermometer.h"

struct srv_state {
    SENSOR_COMMON;
};

void temp_process(srv_t *state) {
    env_sensor_process(state, env_temperature);
}

void temp_handle_packet(srv_t *state, jd_packet_t *pkt) {
    env_sensor_handle_packet(state, pkt, env_temperature);
}

SRV_DEF(temp, JD_SERVICE_CLASS_THERMOMETER);

void temp_init(void) {
    SRV_ALLOC(temp);
    state->streaming_interval = 1000;
    env_temperature(); // start the sensor
}
