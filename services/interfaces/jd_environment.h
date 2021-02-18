// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef __JD_ENVIRONMENT_H
#define __JD_ENVIRONMENT_H

#include "jd_sensor.h"

// sync layout changes with env_sensor_handle_packet()
typedef struct {
    int32_t value;
    int32_t error;
    int32_t min_value;
    int32_t max_value;
} env_reading_t;

typedef const env_reading_t *(*env_function_t)(void);

int env_sensor_handle_packet(srv_t *state, jd_packet_t *pkt, env_function_t fn);
void env_sensor_process(srv_t *state, env_function_t fn);

#define SCALE_TEMP(x) (int)((x)*1024.0 + 0.5)
#define SCALE_HUM(x) (int)((x)*1024.0 + 0.5)

#define ERR_HUM(a, b) SCALE_HUM(a), SCALE_HUM(b)
#define ERR_TEMP(a, b) SCALE_TEMP(a), SCALE_TEMP(b)
#define ERR_END -1, -1

int32_t env_extrapolate_error(int32_t value, const int32_t *error_table);
void env_set_value(env_reading_t *env, int32_t value, const int32_t *error_table);

const env_reading_t *th02_temperature(void);
const env_reading_t *th02_humidity(void);

const env_reading_t *shtc3_temperature(void);
const env_reading_t *shtc3_humidity(void);

#endif
