// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_sensor_api.h"
#include "jacdac/dist/c/eco2.h"

struct srv_state {
    SENSOR_COMMON;
};

void env_sensor_process(srv_t *state) {
    sensor_process(state);
    if (sensor_should_stream(state)) {
        const env_reading_t *env = sensor_get_reading(state);
        if (env) {
            jd_send(state->service_index, JD_GET(JD_REG_READING), &env->value, sizeof(env->value));
            jd_send(state->service_index, JD_GET(JD_REG_READING_ERROR), &env->error,
                    sizeof(env->error));
        }
    }
}

int env_sensor_handle_packet(srv_t *state, jd_packet_t *pkt) {
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
#if 0
    // this has been removed
    case JD_GET(JD_E_CO2_REG_CONDITIONING_PERIOD):
        if (state->api->conditioning_period) {
            tmp = state->api->conditioning_period();
            goto send_it;
        } else {
            jd_send_not_implemented(pkt);
            return 0;
        }
#endif
    default:
        jd_send_not_implemented(pkt);
        return 0;
    }

    state->got_query = 1;

    const env_reading_t *env = sensor_get_reading(state);
    if (env == NULL)
        return 0;
    tmp = (&env->value)[off];

    // send_it:
    jd_send(pkt->service_index, pkt->service_command, &tmp, 4);
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