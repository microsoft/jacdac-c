// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_sensor_api.h"
#include "jacdac/dist/c/color.h"

struct srv_state {
    SENSOR_COMMON;
    uint16_t sample[3];
    uint32_t nextSample;
};

static void update(srv_t *state) {
    uint32_t *sample_raw = sensor_get_reading(state);
    if (!sample_raw)
        return;
    uint32_t max = sample_raw[0];
    for (int i = 1; i < 3; ++i)
        if (sample_raw[i] > max)
            max = sample_raw[i];
    int d = (max >> 15);
    // int d = 1 << 16;
    // TODO update this when color service is updated to use 32 bits
    state->sample[0] = sample_raw[0] / d;
    state->sample[1] = sample_raw[1] / d;
    state->sample[2] = sample_raw[2] / d;
}

void color_process(srv_t *state) {
    // we'll only update once inited, but make sure we update immediately after init
    if (!state->jd_inited)
        state->nextSample = now;

    sensor_process_simple(state, &state->sample, sizeof(state->sample));

    if (jd_should_sample(&state->nextSample, 100000) && state->jd_inited)
        update(state);
}

void color_handle_packet(srv_t *state, jd_packet_t *pkt) {
    sensor_handle_packet_simple(state, pkt, &state->sample, sizeof(state->sample));
}

SRV_DEF(color, JD_SERVICE_CLASS_COLOR);

void color_init(const color_api_t *api) {
    SRV_ALLOC(color);
    state->api = api;
}
