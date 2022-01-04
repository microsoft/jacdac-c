// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include "../jd_sensor.h"

typedef sensor_api_t accelerometer_api_t;
extern const accelerometer_api_t accelerometer_kxtj3;
extern const accelerometer_api_t accelerometer_kx023;
extern const accelerometer_api_t accelerometer_qma7981;
extern const accelerometer_api_t accelerometer_lsm6ds;

typedef sensor_api_t gyroscope_api_t;
extern const gyroscope_api_t gyroscope_lsm6ds;

typedef sensor_api_t color_api_t;

// TSC3471 sensor on color click (with RGB LED)
extern const color_api_t color_click;

typedef sensor_api_t env_sensor_api_t;

extern const env_sensor_api_t temperature_th02;
extern const env_sensor_api_t humidity_th02;

extern const env_sensor_api_t temperature_shtc3;
extern const env_sensor_api_t humidity_shtc3;

extern const env_sensor_api_t temperature_sht30;
extern const env_sensor_api_t humidity_sht30;

extern const env_sensor_api_t temperature_ds18b20;

extern const env_sensor_api_t temperature_max31855;
extern const env_sensor_api_t temperature_max6675;

// SGP30 sensor on airquality4 click
extern const env_sensor_api_t eco2_airquality4;
extern const env_sensor_api_t tvoc_airquality4;

extern const env_sensor_api_t illuminance_ltr390uv;
extern const env_sensor_api_t uvindex_ltr390uv;

extern const env_sensor_api_t pressure_cps122;
extern const env_sensor_api_t temperature_cps122;

extern const env_sensor_api_t temperature_mpl3115a2;
extern const env_sensor_api_t pressure_mpl3115a2;
