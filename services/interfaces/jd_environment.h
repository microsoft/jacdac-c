// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef __JD_ENVIRONMENT_H
#define __JD_ENVIRONMENT_H

#include "../jd_sensor.h"

const env_reading_t *th02_temperature(void);
const env_reading_t *th02_humidity(void);

const env_reading_t *shtc3_temperature(void);
const env_reading_t *shtc3_humidity(void);

#endif
