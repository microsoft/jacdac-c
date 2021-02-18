// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef __JD_ACCEL_H
#define __JD_ACCEL_H

typedef struct {
    void (*init)(void);
    void (*get_sample)(int32_t sample[3]);
} acc_api_t;

extern const acc_api_t acc_kxtj3;
extern const acc_api_t acc_qma7981;

#endif