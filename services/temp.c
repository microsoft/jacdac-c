// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_protocol.h"
#include "interfaces/jd_sensor.h"
#include "interfaces/jd_environment.h"
#include "jacdac/dist/c/temperature.h"

struct srv_state {
    SENSOR_COMMON;
};

void temp_process(srv_t *state) {
    uint32_t temp = hw_temp();
    sensor_process_simple(state, &temp, sizeof(temp));
}

void temp_handle_packet(srv_t *state, jd_packet_t *pkt) {
    uint32_t temp = hw_temp();
    sensor_handle_packet_simple(state, pkt, &temp, sizeof(temp));
}

SRV_DEF(temp, JD_SERVICE_CLASS_TEMPERATURE);

void temp_init(void) {
    SRV_ALLOC(temp);
    state->streaming_interval = 1000;
}
