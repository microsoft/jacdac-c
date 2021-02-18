// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_protocol.h"
#include "interfaces/jd_sensor.h"
#include "interfaces/jd_environment.h"
#include "jacdac/dist/c/thermometer.h"

struct srv_state {
    SENSOR_COMMON;
    env_function_t read;
};

void temp_process(srv_t *state) {
    env_sensor_process(state, state->read);
}

void temp_handle_packet(srv_t *state, jd_packet_t *pkt) {
    env_sensor_handle_packet(state, pkt, state->read);
}

SRV_DEF(temp, JD_SERVICE_CLASS_THERMOMETER);

void temp_init(env_function_t read) {
    SRV_ALLOC(temp);
    state->streaming_interval = 1000;
    state->read = read;
    read(); // start the sensor
}
