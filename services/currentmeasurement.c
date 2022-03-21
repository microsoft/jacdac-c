// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_sensor_api.h"
#include "jacdac/dist/c/currentmeasurement.h"
struct srv_state {
    SENSOR_COMMON;
    char measurement_name[20];
    float measurement[2];
};

REG_DEFINITION(                                    //
    currentmeasurement_regs,                                    //
    REG_SENSOR_COMMON,                                //
    REG_BYTES(JD_CURRENT_MEASUREMENT_REG_MEASUREMENT_NAME, 20),                //
    REG_BYTES(JD_CURRENT_MEASUREMENT_REG_MEASUREMENT, 8),                //
)

void currentmeasurement_process(srv_t *state) {
    sensor_process(state);
    void *tmp = sensor_get_reading(state);
    memcpy(state->measurement, tmp, sizeof(state->measurement));
    sensor_process_simple(state, state->measurement, sizeof(state->measurement));
}

void currentmeasurement_handle_packet(srv_t *state, jd_packet_t *pkt) {
    int r = sensor_handle_packet(state, pkt);

    if (r == JD_CURRENT_MEASUREMENT_REG_MEASUREMENT_NAME)
        service_handle_register(state, pkt, currentmeasurement_regs);
}

SRV_DEF(currentmeasurement, JD_SERVICE_CLASS_CURRENT_MEASUREMENT);
void currentmeasurement_init(const currentmeasurement_params_t params) {
    SRV_ALLOC(currentmeasurement);
    state->streaming_interval = 100;
    state->api = params.api;
    memset(state->measurement_name, 0, 20);
    memcpy(state->measurement_name, params.measurement_name, strlen(params.measurement_name));
}