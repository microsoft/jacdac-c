// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_sensor_api.h"
#include "jacdac/dist/c/analogmeasurement.h"
struct srv_state {
    SENSOR_COMMON;
    uint8_t measurement_type;
    char measurement_name[20];
    float measurement[2];
};

REG_DEFINITION(                                    //
    analogmeasurement_regs,                                    //
    REG_SENSOR_COMMON,                                //
    REG_U8(JD_ANALOG_MEASUREMENT_REG_MEASUREMENT_TYPE),                   //
    REG_BYTES(JD_ANALOG_MEASUREMENT_REG_MEASUREMENT_NAME, 20),                //
    REG_BYTES(JD_ANALOG_MEASUREMENT_REG_MEASUREMENT, 8),                //
)

void analogmeasurement_process(srv_t *state) {
    sensor_process(state);
    void *tmp = sensor_get_reading(state);
    memcpy(state->measurement, tmp, sizeof(state->measurement));
    sensor_process_simple(state, state->measurement, sizeof(state->measurement));
}

void analogmeasurement_handle_packet(srv_t *state, jd_packet_t *pkt) {
    int r = sensor_handle_packet(state, pkt);

    if (r == JD_ANALOG_MEASUREMENT_REG_MEASUREMENT_TYPE || JD_ANALOG_MEASUREMENT_REG_MEASUREMENT_NAME)
        service_handle_register(state, pkt, analogmeasurement_regs);
}

SRV_DEF(analogmeasurement, JD_SERVICE_CLASS_ANALOG_MEASUREMENT);
void analogmeasurement_init(const analogmeasurement_params_t params) {
    SRV_ALLOC(analogmeasurement);
    state->streaming_interval = 100;
    state->api = params.api;
    state->measurement_type = params.measurement_type;
    memset(state->measurement_name, 0, 20);
    memcpy(state->measurement_name, params.measurement_name, strlen(params.measurement_name));
}