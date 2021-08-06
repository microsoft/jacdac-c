// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

typedef struct {
    void (*init)(void);
    void (*get_sample)(uint32_t sample[4]);
} color_api_t;

extern const color_api_t color_click;
