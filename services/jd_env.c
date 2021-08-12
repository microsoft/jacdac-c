// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_sensor_api.h"
#include "jacdac/dist/c/eco2.h"

struct srv_state {
    SENSOR_COMMON;
};

// this really should return jd_system_status_code_t but that structure is also 32 bits and starts
// with the status code enum, so for simplicity we just use uint32_t
static uint32_t get_status_code(srv_t *state) {
    if (!state->inited)
        return JD_STATUS_CODES_SLEEPING;
    if (!state->got_env_reading)
        return JD_STATUS_CODES_INITIALIZING;
    return JD_STATUS_CODES_READY;
}

static void send_status(srv_t *state) {
    uint32_t payload = get_status_code(state);
    // DMESG("status: s=%d st=%d", state->service_number, payload);
    jd_send_event_ext(state, JD_EV_STATUS_CODE_CHANGED, &payload, sizeof(payload));
}

static void got_reading(srv_t *state) {
    if (!state->got_env_reading) {
        state->got_env_reading = 1;
        send_status(state);
    }
}

static void maybe_init(srv_t *state, const env_sensor_api_t *api) {
    if (!state->inited) {
        state->got_env_reading = 0;
        if (api->init)
            api->init();
        state->inited = true;
        send_status(state);
    }
}

void env_sensor_process(srv_t *state, const env_sensor_api_t *api) {
    if (!state->got_query)
        return;
    maybe_init(state, api);
    if (api->process)
        api->process();

    if (sensor_should_stream(state)) {
        const env_reading_t *env = api->get_reading();
        if (env) {
            got_reading(state);
            jd_send(state->service_number, JD_GET(JD_REG_READING), &env->value, sizeof(env->value));
        }
    }
}

int env_sensor_handle_packet(srv_t *state, jd_packet_t *pkt, const env_sensor_api_t *api) {
    int off;
    int32_t tmp;

    int r = sensor_handle_packet(state, pkt);
    if (r)
        return r;

    switch (pkt->service_command) {
    case JD_GET(JD_REG_READING):
        off = 0;
        break;
    case JD_GET(JD_REG_READING_ERROR):
        off = 1;
        break;
    case JD_GET(JD_REG_MIN_READING):
        off = 2;
        break;
    case JD_GET(JD_REG_MAX_READING):
        off = 3;
        break;
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
            if (api->sleep && state->inited) {
                state->got_env_reading = 0;
                state->inited = 0;
                api->sleep();
                send_status(state);
            }
        }
        return JD_REG_INTENSITY;
    case JD_GET(JD_E_CO2_REG_CONDITIONING_PERIOD):
        if (api->conditioning_period) {
            tmp = api->conditioning_period();
            goto send_it;
        } else {
            return 0;
        }
    default:
        return 0;
    }

    state->got_query = 1;
    if (!state->inited)
        return 0;

    const env_reading_t *env = api->get_reading();
    if (env == NULL)
        return 0;
    got_reading(state);
    tmp = (&env->value)[off];

send_it:
    jd_send(pkt->service_number, pkt->service_command, &tmp, 4);
    return -(pkt->service_command & 0xfff);
}

void env_set_value(env_reading_t *env, int32_t value, const int32_t *error_table) {
    env->value = value;
    env->error = env_extrapolate_error(value, error_table);
}

int32_t env_extrapolate_error(int32_t value, const int32_t *error_table) {
    if (value < error_table[0])
        return error_table[1];
    int32_t prev = error_table[0];
    int idx = 2;
    for (;;) {
        if (error_table[idx] == -1 && error_table[idx + 1] == -1)
            return error_table[idx - 1];
        int32_t curr = error_table[idx];
        if (value <= curr) {
            int32_t size = curr - prev;
            int32_t pos = value - prev;
            int32_t e0 = error_table[idx - 1];
            int32_t e1 = error_table[idx + 1];
            return (pos * (e1 - e0) / size) + e0;
        }
        prev = curr;
        idx += 2;
    }
}

#define ABSHUM(t, h) (int)(t * (1 << 10)), (int)(h * (1 << 10))
static const int32_t abs_hum[] = {
    ABSHUM(-25, 0.6), ABSHUM(-20, 0.9), ABSHUM(-15, 1.6), ABSHUM(-10, 2.3), ABSHUM(-5, 3.4),
    ABSHUM(0, 4.8),   ABSHUM(5, 6.8),   ABSHUM(10, 9.4),  ABSHUM(15, 12.8), ABSHUM(20, 17.3),
    ABSHUM(25, 23),   ABSHUM(30, 30.4), ABSHUM(35, 39.6), ABSHUM(40, 51.1), ABSHUM(45, 65.4),
    ABSHUM(50, 83),   ERR_END,
};

// result is i22.10 g/m3
int32_t env_absolute_humidity(int32_t temp, int32_t humidity) {
    int32_t maxval = env_extrapolate_error(temp, abs_hum);
    return (maxval * humidity / 100) >> 10;
}