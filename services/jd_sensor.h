// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include "jd_protocol.h"

typedef void *(*get_reading_t)(void);

typedef struct {
    void (*init)(void);
    void (*process)(void);
    void (*sleep)(void);
    get_reading_t get_reading;
    // only present for some sensors:
    uint32_t (*conditioning_period)(void); // for eco2 and tvoc
    void (*set_temp_humidity)(int32_t temp, int32_t humidity); // temp/hum compensation
} sensor_api_t;

#define SENSOR_COMMON                                                                              \
    SRV_COMMON;                                                                                    \
    uint8_t streaming_samples;                                                                     \
    uint8_t got_query : 1;                                                                         \
    uint8_t inited : 1;                                                                            \
    uint8_t got_reading : 1;                                                                   \
    uint32_t streaming_interval;                                                                   \
    uint32_t next_streaming;                                                                       \
    const sensor_api_t *api

#define REG_SENSOR_COMMON REG_BYTES(JD_REG_PADDING, 20)

int sensor_handle_packet(srv_t *state, jd_packet_t *pkt);
int sensor_should_stream(srv_t *state);
int sensor_handle_packet_simple(srv_t *state, jd_packet_t *pkt, const void *sample,
                                uint32_t sample_size);
void sensor_process_simple(srv_t *state, const void *sample, uint32_t sample_size);

void sensor_process(srv_t *state);
void sensor_send_status(srv_t *state);
void *sensor_get_reading(srv_t *state);

// sync layout changes with env_sensor_handle_packet()
typedef struct {
    int32_t value;
    int32_t error;
    int32_t min_value;
    int32_t max_value;
} env_reading_t;

int env_sensor_handle_packet(srv_t *state, jd_packet_t *pkt);
void env_sensor_process(srv_t *state);

#define SCALE_TEMP(x) (int)((x)*1024.0 + 0.5)
#define SCALE_HUM(x) (int)((x)*1024.0 + 0.5)

#define ERR_HUM(a, b) SCALE_HUM(a), SCALE_HUM(b)
#define ERR_TEMP(a, b) SCALE_TEMP(a), SCALE_TEMP(b)
#define ERR_END -1, -1

int32_t env_extrapolate_error(int32_t value, const int32_t *error_table);
void env_set_value(env_reading_t *env, int32_t value, const int32_t *error_table);

// relative->absolute humidity conversion;
// all args are i22.10; temp is C, humidity %, result g/m3
int32_t env_absolute_humidity(int32_t temp, int32_t humidity);
