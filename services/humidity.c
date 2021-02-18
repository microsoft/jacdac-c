// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_protocol.h"
#include "interfaces/jd_sensor.h"
#include "interfaces/jd_environment.h"
#include "jacdac/dist/c/humidity.h"

struct srv_state {
    SENSOR_COMMON;
    env_function_t read;
};

void humidity_process(srv_t *state) {
    env_sensor_process(state, state->read);
}

void humidity_handle_packet(srv_t *state, jd_packet_t *pkt) {
    env_sensor_handle_packet(state, pkt, state->read);
}

SRV_DEF(humidity, JD_SERVICE_CLASS_HUMIDITY);
void humidity_init(env_function_t read) {
    SRV_ALLOC(humidity);
    state->streaming_interval = 1000;
    state->read = read;
    read(); // start the sensor
}
