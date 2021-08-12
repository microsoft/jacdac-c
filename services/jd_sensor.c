// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"

REG_DEFINITION(                         //
    sensor_regs,                        //
    REG_SRV_COMMON,                     //
    REG_U8(JD_REG_STREAMING_SAMPLES),   //
    REG_U32(JD_REG_STREAMING_INTERVAL), //
    REG_U32(JD_REG_PADDING),            // next_streaming not accessible
);

struct srv_state {
    SENSOR_COMMON;
};

// this really should return jd_system_status_code_t but that structure is also 32 bits and starts
// with the status code enum, so for simplicity we just use uint32_t
static uint32_t get_status_code(srv_t *state) {
    if (!state->inited)
        return JD_STATUS_CODES_SLEEPING;
    if (!state->got_reading)
        return JD_STATUS_CODES_INITIALIZING;
    return JD_STATUS_CODES_READY;
}

void sensor_send_status(srv_t *state) {
    uint32_t payload = get_status_code(state);
    // DMESG("status: s=%d st=%d", state->service_number, payload);
    jd_send_event_ext(state, JD_EV_STATUS_CODE_CHANGED, &payload, sizeof(payload));
}

int sensor_handle_packet(srv_t *state, jd_packet_t *pkt) {
    switch (pkt->service_command) {
    case JD_GET(JD_REG_INTENSITY):
        jd_respond_u8(pkt, state->inited ? 1 : 0);
        return -JD_REG_INTENSITY;
    case JD_GET(JD_REG_STATUS_CODE):
        jd_respond_u32(pkt, get_status_code(state));
        return -JD_REG_STATUS_CODE;
    case JD_SET(JD_REG_INTENSITY):
        if (pkt->data[0]) {
            // this will make it initialize soon
            state->got_query = 1;
        } else {
            state->got_query = 0;
            state->streaming_samples = 0;
            // if sensor supports sleep and was already initialized, put it to sleep
            if (state->api && state->api->sleep && state->inited) {
                state->got_reading = 0;
                state->inited = 0;
                state->api->sleep();
                sensor_send_status(state);
            }
        }
        return JD_REG_INTENSITY;
    }

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

void *sensor_get_reading(srv_t *state) {
    if (!state->api)
        jd_panic();
    if (!state->inited)
        return NULL;
    void *r = state->api->get_reading();
    if (r && !state->got_reading) {
        state->got_reading = 1;
        sensor_send_status(state);
    }
    return r;
}

static void maybe_init(srv_t *state) {
    if (!state->inited) {
        state->got_reading = 0;
        state->inited = 1;
        if (state->api && state->api->init) {
            sensor_send_status(state);
            state->api->init();
        }
        if (!state->api || state->api->get_reading())
            state->got_reading = 1;
        sensor_send_status(state);
    }
}

void sensor_process(srv_t *state) {
    if (!state->got_query)
        return;
    maybe_init(state);
    if (state->api && state->api->process)
        state->api->process();
}

void sensor_process_simple(srv_t *state, const void *sample, uint32_t sample_size) {
    sensor_process(state);
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