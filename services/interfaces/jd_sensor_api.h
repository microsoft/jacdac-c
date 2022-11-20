// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include "../jd_sensor.h"

typedef sensor_api_t accelerometer_api_t;
extern const accelerometer_api_t accelerometer_kxtj3;
extern const accelerometer_api_t accelerometer_kx023;
extern const accelerometer_api_t accelerometer_qma7981;
extern const accelerometer_api_t accelerometer_lsm6ds;

typedef sensor_api_t captouch_api_t;
extern const captouch_api_t captouch_cap1298;

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
extern const env_sensor_api_t eco2_sgp30;
extern const env_sensor_api_t tvoc_sgp30;

extern const env_sensor_api_t tvoc_sgp30;
extern const env_sensor_api_t eco2_sgp30;

extern const env_sensor_api_t illuminance_ltr390uv;
extern const env_sensor_api_t uvindex_ltr390uv;

extern const env_sensor_api_t pressure_cps122;
extern const env_sensor_api_t temperature_cps122;

extern const env_sensor_api_t pressure_dps310;
extern const env_sensor_api_t temperature_dps310;

extern const env_sensor_api_t temperature_mpl3115a2;
extern const env_sensor_api_t pressure_mpl3115a2;

extern const env_sensor_api_t ethanol_sgpc3;
extern const env_sensor_api_t tvoc_sgpc3;

extern const env_sensor_api_t pressure_lps33hwtr;
extern const env_sensor_api_t temperature_lps33hwtr;

extern const env_sensor_api_t co2_scd40;
extern const env_sensor_api_t temperature_scd40;
extern const env_sensor_api_t humidity_scd40;

// I2C scanning
extern const accelerometer_api_t *i2c_accelerometers[];
extern const gyroscope_api_t *i2c_gyro[];
extern const captouch_api_t *i2c_captouch[];
extern const env_sensor_api_t *i2c_temperature[];
extern const env_sensor_api_t *i2c_pressure[];
extern const env_sensor_api_t *i2c_humidity[];
extern const env_sensor_api_t *i2c_co2[];
extern const env_sensor_api_t *i2c_tvoc[];
extern const env_sensor_api_t *i2c_illuminance[];
extern const env_sensor_api_t *i2c_uvindex[];

int jd_scan_i2c(const char *label, const sensor_api_t **apis, void (*init)(const sensor_api_t *));

int jd_scan_accelerometers(void);
int jd_scan_gyroscopes(void);
int jd_scan_temperature(void);
int jd_scan_pressure(void);
int jd_scan_humidity(void);
int jd_scan_co2(void);
int jd_scan_tvoc(void);
int jd_scan_uvindex(void);
int jd_scan_illuminance(void);

int jd_scan_all(void);
