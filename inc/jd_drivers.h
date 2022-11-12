// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef JD_DRIVERS_H
#define JD_DRIVERS_H

#include "jd_service_framework.h"
#include "interfaces/jd_hw.h"
#include "services/interfaces/jd_sensor_api.h"
#include "services/interfaces/jd_pins.h"
#include "jd_util.h"

// This macros to be used when a single sensor implements two (or more) services
#define ENV_INIT_N(finit, fsleep, n)                                                               \
    static void init##n(void) {                                                                    \
        uint8_t s = init_status;                                                                   \
        init_status |= (1 << n);                                                                   \
        if (!s)                                                                                    \
            finit();                                                                               \
    }                                                                                              \
    static void sleep##n(void) {                                                                   \
        if (!init_status)                                                                          \
            return;                                                                                \
        init_status &= ~(1 << n);                                                                  \
        if (!init_status)                                                                          \
            fsleep();                                                                              \
    }

#define ENV_INIT_DUAL(init, sleep)                                                                 \
    static uint8_t init_status;                                                                    \
    ENV_INIT_N(init, sleep, 0);                                                                    \
    ENV_INIT_N(init, sleep, 1);

#define ENV_INIT_TRIPLE(init, sleep)                                                               \
    static uint8_t init_status;                                                                    \
    ENV_INIT_N(init, sleep, 0);                                                                    \
    ENV_INIT_N(init, sleep, 1);                                                                    \
    ENV_INIT_N(init, sleep, 2);

#define ENV_INIT_PTRS(n) .init = init##n, .sleep = sleep##n

#define ENV_DEFINE_GETTER(scd, val)                                                                \
    static void *scd##_##val(void) {                                                               \
        ctx_t *ctx = &state;                                                                       \
        if (ctx->inited >= 3)                                                                      \
            return &ctx->val;                                                                      \
        return NULL;                                                                               \
    }

#define ENV_GETTER(scd, val, idx)                                                                  \
    .process = scd##_process, .get_reading = scd##_##val, ENV_INIT_PTRS(idx)

// Used by various Sensirion sensors
uint8_t jd_sgp_crc8(const uint8_t *data, int len);
int jd_sgp_read_u16(uint8_t dev_addr, uint16_t regaddr, unsigned wait);

#endif
