// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"

REG_DEFINITION(                         //
    sensor_regs,                        //
    REG_SRV_COMMON,                       //
    REG_U8(JD_REG_STREAMING_SAMPLES),   //
    REG_U32(JD_REG_STREAMING_INTERVAL), //
    REG_U32(JD_REG_PADDING),            // next_streaming not accessible
);

struct srv_state {
    SENSOR_COMMON;
};

int sensor_handle_packet(srv_t *state, jd_packet_t *pkt) {
    int r = service_handle_register(state, pkt, sensor_regs);
    switch (r) {
    case JD_REG_STREAMING_SAMPLES:
        if (state->streaming_samples) {
            if (state->streaming_interval == 0)
                state->streaming_interval = 100;
            state->next_streaming = now;
            state->got_query = 1;
        }
        break;
    case JD_REG_STREAMING_INTERVAL:
        if (state->streaming_interval < 20)
            state->streaming_interval = 20;
        if (state->streaming_interval > 100000)
            state->streaming_interval = 100000;
        break;
    }
    return r;
}

void sensor_process_simple(srv_t *state, const void *sample, uint32_t sample_size) {
    if (sensor_should_stream(state))
        jd_send(state->service_number, JD_GET(JD_REG_READING), sample, sample_size);
}

int sensor_handle_packet_simple(srv_t *state, jd_packet_t *pkt, const void *sample,
                                uint32_t sample_size) {
    int r = sensor_handle_packet(state, pkt);

    if (pkt->service_command == JD_GET(JD_REG_READING)) {
        state->got_query = 1;
        jd_send(pkt->service_number, pkt->service_command, sample, sample_size);
        r = -JD_REG_READING;
    }

    return r;
}

int sensor_should_stream(srv_t *state) {
    if (!state->streaming_samples)
        return false;
    if (jd_should_sample(&state->next_streaming, state->streaming_interval * 1000)) {
        state->streaming_samples--;
        return true;
    }
    return false;
}