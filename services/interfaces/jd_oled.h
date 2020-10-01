// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef __JD_OLED_H
#define __JD_OLED_H

#define OLED_WIDTH 64
#define OLED_HEIGHT 48

typedef struct oled_state {
    uint8_t databuf[OLED_WIDTH * OLED_HEIGHT / 8];
} oled_state_t;

void oled_set_pixel(oled_state_t *ctx, int x, int y);
void oled_setup(oled_state_t *ctx);
void oled_flush(oled_state_t *ctx);

#endif