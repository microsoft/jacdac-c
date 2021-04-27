// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef __JD_ACCEL_H
#define __JD_ACCEL_H

typedef struct {
    void (*init)(void);
    void (*get_sample)(int32_t sample[3]);
    void (*sleep)(void);
} accelerometer_api_t;

extern const accelerometer_api_t accelerometer_kxtj3;
extern const accelerometer_api_t accelerometer_kx023;
extern const accelerometer_api_t accelerometer_qma7981;

#endif