// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_environment.h"
#include "jacdac/dist/c/thermometer.h"

struct srv_state {
    SENSOR_COMMON;
    env_function_t read;
};

void thermometer_process(srv_t *state) {
    env_sensor_process(state, state->read);
}

void thermometer_handle_packet(srv_t *state, jd_packet_t *pkt) {
    env_sensor_handle_packet(state, pkt, state->read);
}

SRV_DEF(thermometer, JD_SERVICE_CLASS_THERMOMETER);

void thermometer_init(env_function_t read) {
    SRV_ALLOC(thermometer);
    state->streaming_interval = 1000;
    state->read = read;
    read(); // start the sensor
}
