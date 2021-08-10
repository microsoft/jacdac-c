// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include "../jd_sensor.h"

typedef struct {
    void (*init)(void);
    void (*process)(void);
    void (*sleep)(void);
    void (*get_sample)(void *sample);
} sensor_api_t;

typedef void (*get_sample_t)(void *);

struct env_sensor_api {
    void (*init)(void);
    void (*process)(void);
    void (*sleep)(void);
    const env_reading_t *(*get_reading)(void);
    uint32_t (*conditioning_period)(void);
};

typedef sensor_api_t accelerometer_api_t;
extern const accelerometer_api_t accelerometer_kxtj3;
extern const accelerometer_api_t accelerometer_kx023;
extern const accelerometer_api_t accelerometer_qma7981;

typedef sensor_api_t color_api_t;

// TSC3471 sensor on color click (with RGB LED)
extern const color_api_t color_click;

extern const env_sensor_api_t temperature_th02;
extern const env_sensor_api_t humidity_th02;

extern const env_sensor_api_t temperature_shtc3;
extern const env_sensor_api_t humidity_shtc3;

// SG30 sensor on airquality4 click
extern const env_sensor_api_t eco2_airquality4;
extern const env_sensor_api_t tvoc_airquality4;

