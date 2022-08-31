// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef __JD_DISP_H
#define __JD_DISP_H

#ifndef DISP_LIGHT_SENSE
#define DISP_LIGHT_SENSE 0
#endif

void disp_show(uint8_t *img);
void disp_refresh(void);
void disp_sleep(void);
#if DISP_LIGHT_SENSE
int disp_light_level(void);
void disp_set_dark_level(int v);
int disp_get_dark_level(void);
#else
void disp_set_brigthness(uint16_t v);
#endif

#endif