// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"

REG_DEFINITION(                         //
    sensor_regs,                        //
    REG_SRV_COMMON,                     //
    REG_U8(JD_REG_STREAMING_SAMPLES),   //
    REG_U32(JD_REG_STREAMING_INTERVAL), //
);

struct srv_state {
    SENSOR_COMMON;
};

// this really should return jd_system_status_code_t but that structure is also 32 bits and starts
// with the status code enum, so for simplicity we just use uint32_t
static uint32_t get_status_code(srv_t *state) {
    if (!state->jd_inited)
        return JD_STATUS_CODES_SLEEPING;
    if (!state->got_reading)
        return JD_STATUS_CODES_INITIALIZING;
    return JD_STATUS_CODES_READY;
}

void sensor_send_status(srv_t *state) {
    uint32_t payload = get_status_code(state);
    // DMESG("status: s=%d st=%d", state->service_index, payload);
    jd_send_event_ext(state, JD_EV_STATUS_CODE_CHANGED, &payload, sizeof(payload));
}

static int respond_ranges(srv_t *state) {
    const sensor_range_t *r = state->api->ranges;
    if (!r)
        return 0;
    while (r->range)
        r++;
    int len = r - state->api->ranges;
    uint32_t buf[len];
    r = state->api->ranges;
    for (int i = 0; i < len; ++i)
        buf[i] = r[i].range;
    jd_send(state->service_index, JD_GET(JD_REG_SUPPORTED_RANGES), buf, len * sizeof(uint32_t));
    return -JD_REG_SUPPORTED_RANGES;
}

int sensor_handle_packet(srv_t *state, jd_packet_t *pkt) {
    switch (pkt->service_command) {
    case JD_GET(JD_REG_INTENSITY):
        jd_respond_u8(pkt, state->jd_inited ? 1 : 0);
        return -JD_REG_INTENSITY;
    case JD_GET(JD_REG_STATUS_CODE):
        jd_respond_u32(pkt, get_status_code(state));
        return -JD_REG_STATUS_CODE;
    case JD_GET(JD_REG_READING_RANGE):
        if (!state->api->get_range)
            return 0;
        jd_respond_u32(pkt, state->api->get_range());
        return -JD_REG_READING_RANGE;
    case JD_SET(JD_REG_READING_RANGE):
        if (!state->api->set_range)
            return 0;
        state->api->set_range(*(uint32_t *)pkt->data);
        return JD_REG_READING_RANGE;
    case JD_GET(JD_REG_SUPPORTED_RANGES):
        return respond_ranges(state);
    case JD_SET(JD_REG_INTENSITY):
        if (pkt->data[0]) {
            // this will make it initialize soon
            state->got_query = 1;
        } else {
            state->got_query = 0;
            state->streaming_samples = 0;
            // if sensor supports sleep and was already initialized, put it to sleep
            if (state->api && state->api->sleep && state->jd_inited) {
                state->got_reading = 0;
                state->jd_inited = 0;
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
        JD_PANIC();
    if (!state->jd_inited)
        return NULL;
    void *r = state->api->get_reading();
    if (r && !state->got_reading) {
        state->got_reading = 1;
        sensor_send_status(state);
    }
    return r;
}

const sensor_range_t *sensor_lookup_range(const sensor_range_t *ranges, int32_t requested) {
    while (ranges->range) {
        if (ranges->range >= requested)
            return ranges;
        ranges++;
    }
    return ranges - 1; // return maximum possible one
}

bool sensor_maybe_init(srv_t *state) {
    if (state->got_query && !state->jd_inited) {
        state->got_reading = 0;
        state->jd_inited = 1;
        if (state->api && state->api->init) {
            sensor_send_status(state);
            state->api->init();
        }
        if (!state->api || state->api->get_reading())
            state->got_reading = 1;
        sensor_send_status(state);
        return true;
    }
    return false;
}

void sensor_process(srv_t *state) {
    sensor_maybe_init(state);
    if (state->jd_inited && state->api && state->api->process)
        state->api->process();
}

void sensor_process_simple(srv_t *state, const void *sample, uint32_t sample_size) {
    sensor_process(state);
    if (sensor_should_stream(state))
        jd_send(state->service_index, JD_GET(JD_REG_READING), sample, sample_size);
}

int sensor_handle_packet_simple(srv_t *state, jd_packet_t *pkt, const void *sample,
                                uint32_t sample_size) {
    int r = sensor_handle_packet(state, pkt);

    if (pkt->service_command == JD_GET(JD_REG_READING)) {
        state->got_query = 1;
        jd_send(pkt->service_index, pkt->service_command, sample, sample_size);
        r = -JD_REG_READING;
    }

    if (r == 0)
        jd_send_not_implemented(pkt);

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

bool sensor_should_send_threshold_event(uint32_t *block, uint32_t debounce_ms, bool cond_ok) {
    if (*block == 0 || in_past(*block)) {
        if (cond_ok) {
            if (*block == 0) {
                *block = now + (debounce_ms << 10);
                return 1;
            } else {
                *block = now;
            }
        } else {
            *block = 0;
        }
    }
    return 0;
}
