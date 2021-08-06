// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_sensor_api.h"

struct srv_state {
    SENSOR_COMMON;
};

void env_sensor_process(srv_t *state, const env_sensor_api_t *api) {
    if (api->process)
        api->process();

    if (sensor_should_stream(state)) {
        const env_reading_t *env = api->get_reading();
        if (env)
            jd_send(state->service_number, JD_GET(JD_REG_READING), &env->value, sizeof(env->value));
    }
}

int env_sensor_handle_packet(srv_t *state, jd_packet_t *pkt, const env_sensor_api_t *api) {
    int off;

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
    default:
        return 0;
    }

    const env_reading_t *env = api->get_reading();
    if (env == NULL)
        return 0;

    jd_send(pkt->service_number, pkt->service_command, &env->value + off, 4);

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
