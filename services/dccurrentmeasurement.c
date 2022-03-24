// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_sensor_api.h"
#include "jacdac/dist/c/dccurrentmeasurement.h"
struct srv_state {
    SENSOR_COMMON;
    char measurement_name[20];
    double measurement;
};

REG_DEFINITION(                                                    //
    dccurrentmeasurement_regs,                                     //
    REG_SENSOR_COMMON,                                             //
    REG_BYTES(JD_D_CCURRENT_MEASUREMENT_REG_MEASUREMENT_NAME, 20), //
    REG_BYTES(JD_D_CCURRENT_MEASUREMENT_REG_MEASUREMENT, 8),       //
)

void dccurrentmeasurement_process(srv_t *state) {
    sensor_process(state);
    void *tmp = sensor_get_reading(state);
    memcpy(&state->measurement, tmp, sizeof(state->measurement));
    sensor_process_simple(state, &state->measurement, sizeof(state->measurement));
}

void dccurrentmeasurement_handle_packet(srv_t *state, jd_packet_t *pkt) {
    int r = sensor_handle_packet(state, pkt);

    if (r == JD_D_CCURRENT_MEASUREMENT_REG_MEASUREMENT_NAME)
        service_handle_register(state, pkt, dccurrentmeasurement_regs);
}

SRV_DEF(dccurrentmeasurement, JD_SERVICE_CLASS_D_CCURRENT_MEASUREMENT);
void dccurrentmeasurement_init(const dccurrentmeasurement_params_t params) {
    SRV_ALLOC(dccurrentmeasurement);
    state->streaming_interval = 100;
    state->api = params.api;
    memset(state->measurement_name, 0, 20);
    memcpy(state->measurement_name, params.measurement_name, strlen(params.measurement_name));
}