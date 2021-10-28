// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_sensor_api.h"
#include "jacdac/dist/c/gyroscope.h"

struct srv_state {
    SENSOR_COMMON;
    jd_gyroscope_rotation_rates_t sample;
    uint32_t nextSample;
};

extern uint8_t gyroscope_pending;

void gyroscope_process(srv_t *state) {
#ifdef PIN_ACC_INT
    if (!gyroscope_pending && state->inited)
        return;
    gyroscope_pending = 0;
#else
    if (!jd_should_sample(&state->nextSample, 9500))
        return;
#endif

    sensor_process(state);
    void *tmp = sensor_get_reading(state);
    if (tmp) {
        memcpy(&state->sample.x, tmp, 3 * 4);
        gyroscope_data_transform(&state->sample.x);
    }

    sensor_process_simple(state, &state->sample, sizeof(state->sample));
}

void gyroscope_handle_packet(srv_t *state, jd_packet_t *pkt) {
    sensor_handle_packet_simple(state, pkt, &state->sample, sizeof(state->sample));
}

SRV_DEF(gyroscope, JD_SERVICE_CLASS_GYROSCOPE);

void gyroscope_init(const gyroscope_api_t *api) {
    SRV_ALLOC(gyroscope);
    state->api = api;
}
