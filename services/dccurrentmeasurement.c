// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_sensor_api.h"
#include "jacdac/dist/c/dccurrentmeasurement.h"
struct srv_state {
    SENSOR_COMMON;
    double measurement;
    const char *measurement_name;
};

REG_DEFINITION(                                              //
    dccurrentmeasurement_regs,                               //
    REG_SENSOR_COMMON,                                       //
    REG_BYTES(JD_DC_CURRENT_MEASUREMENT_REG_MEASUREMENT, 8), //
)

void dccurrentmeasurement_process(srv_t *state) {
    void *tmp = sensor_get_reading(state);
    memcpy(&state->measurement, tmp, sizeof(state->measurement));
    sensor_process_simple(state, &state->measurement, sizeof(state->measurement));
}

void dccurrentmeasurement_handle_packet(srv_t *state, jd_packet_t *pkt) {
    if (service_handle_register(state, pkt, dccurrentmeasurement_regs) ||
        service_handle_string_register(pkt, JD_DC_CURRENT_MEASUREMENT_REG_MEASUREMENT_NAME,
                                       state->measurement_name))
        return;
    sensor_handle_packet_simple(state, pkt, &state->measurement, sizeof(state->measurement));
}

SRV_DEF(dccurrentmeasurement, JD_SERVICE_CLASS_DC_CURRENT_MEASUREMENT);
void dccurrentmeasurement_init(const dccurrentmeasurement_params_t *params) {
    SRV_ALLOC(dccurrentmeasurement);
    state->streaming_interval = 500;
    state->api = params->api;
    state->measurement_name = params->measurement_name;
}
