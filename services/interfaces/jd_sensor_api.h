// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include "../jd_sensor.h"

typedef struct {
    void (*init)(void);
    void (*get_sample)(int32_t sample[3]);
    void (*sleep)(void);
} accelerometer_api_t;

extern const accelerometer_api_t accelerometer_kxtj3;
extern const accelerometer_api_t accelerometer_kx023;
extern const accelerometer_api_t accelerometer_qma7981;

typedef struct {
    void (*init)(void);
    void (*get_sample)(uint32_t sample[4]);
} color_api_t;

extern const color_api_t color_click;

const env_reading_t *th02_temperature(void);
const env_reading_t *th02_humidity(void);

const env_reading_t *shtc3_temperature(void);
const env_reading_t *shtc3_humidity(void);
