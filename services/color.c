// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_sensor_api.h"
#include "jacdac/dist/c/color.h"

struct srv_state {
    SENSOR_COMMON;
    const color_api_t *api;
    uint32_t sample_raw[4];
    uint16_t sample[3];
    uint32_t nextSample;
};

static void update(srv_t *state) {
    state->api->get_sample(state->sample_raw);
    uint32_t max = state->sample_raw[0];
    for (int i = 1; i < 3; ++i)
        if (state->sample_raw[i] > max)
            max = state->sample_raw[i];
    int d = (max >> 15);
    // int d = 1 << 16;
    // TODO update this when color service is updated to use 32 bits
    state->sample[0] = state->sample_raw[0] / d;
    state->sample[1] = state->sample_raw[1] / d;
    state->sample[2] = state->sample_raw[2] / d;
}

static void maybe_init(srv_t *state) {
    if (state->got_query && !state->inited) {
        state->inited = true;
        state->api->init();
    }
}

void color_process(srv_t *state) {
    maybe_init(state);

    if (jd_should_sample(&state->nextSample, 100000) && state->inited)
        update(state);

    sensor_process_simple(state, &state->sample, sizeof(state->sample));
}

void color_handle_packet(srv_t *state, jd_packet_t *pkt) {
    sensor_handle_packet_simple(state, pkt, &state->sample, sizeof(state->sample));
}

SRV_DEF(color, JD_SERVICE_CLASS_COLOR);

void color_init(const color_api_t *api) {
    SRV_ALLOC(color);
    state->api = api;
}
