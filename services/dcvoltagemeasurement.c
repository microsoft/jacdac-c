// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_sensor_api.h"
#include "jacdac/dist/c/dcvoltagemeasurement.h"
struct srv_state {
    SENSOR_COMMON;
    uint8_t measurement_type;
    char measurement_name[20];
    double measurement;
};

REG_DEFINITION(                                                    //
    dcvoltagemeasurement_regs,                                     //
    REG_SENSOR_COMMON,                                             //
    REG_U8(JD_D_CVOLTAGE_MEASUREMENT_REG_MEASUREMENT_TYPE),        //
    REG_BYTES(JD_D_CVOLTAGE_MEASUREMENT_REG_MEASUREMENT_NAME, 20), //
    REG_BYTES(JD_D_CVOLTAGE_MEASUREMENT_REG_MEASUREMENT, 8),       //
)

void dcvoltagemeasurement_process(srv_t *state) {
    sensor_process(state);
    void *tmp = sensor_get_reading(state);
    memcpy(&state->measurement, tmp, sizeof(state->measurement));
    sensor_process_simple(state, &state->measurement, sizeof(state->measurement));
}

void dcvoltagemeasurement_handle_packet(srv_t *state, jd_packet_t *pkt) {
    int r = sensor_handle_packet(state, pkt);

    if (r == JD_D_CVOLTAGE_MEASUREMENT_REG_MEASUREMENT_TYPE ||
        r == JD_D_CVOLTAGE_MEASUREMENT_REG_MEASUREMENT_NAME)
        service_handle_register(state, pkt, dcvoltagemeasurement_regs);
}

SRV_DEF(dcvoltagemeasurement, JD_SERVICE_CLASS_D_CVOLTAGE_MEASUREMENT);
void dcvoltagemeasurement_init(const dcvoltagemeasurement_params_t params) {
    SRV_ALLOC(dcvoltagemeasurement);
    state->streaming_interval = 100;
    state->api = params.api;
    state->measurement_type = params.measurement_type;
    memset(state->measurement_name, 0, 20);
    memcpy(state->measurement_name, params.measurement_name, strlen(params.measurement_name));
}