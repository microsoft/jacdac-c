// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef JD_CONFIG_H
#define JD_CONFIG_H

#include "jd_user_config.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

// #define JD_DEBUG_MODE

#define JD_NOLOG(...) ((void)0)

// LOG(...) to be defined in particular .c files as either JD_LOG or JD_NOLOG

#ifdef JD_DEBUG_MODE
#define ERROR(msg, ...)                                                                            \
    do {                                                                                           \
        jd_debug_signal_error();                                                                   \
        JD_LOG("! " msg, ##__VA_ARGS__);                                                   \
    } while (0)
#else
#define ERROR(msg, ...)                                                                            \
    do {                                                                                           \
        JD_LOG("! " msg, ##__VA_ARGS__);                                                   \
    } while (0)
#endif

#if __WORDSIZE == 64
#define JD_64 1
#define JD_PTRSIZE 8
#elif __WORDSIZE == 32
#define JD_64 0
#define JD_PTRSIZE 4
#else
#error "can't determine pointer size"
#endif

#ifndef JD_CONFIG_TEMPERATURE
#define JD_CONFIG_TEMPERATURE 0
#endif

#ifndef JD_CONFIG_CONTROL_FLOOD
#define JD_CONFIG_CONTROL_FLOOD 1
#endif

#ifndef JD_CONFIG_WATCHDOG
#define JD_CONFIG_WATCHDOG 1
#endif

#ifndef JD_CONFIG_STATUS
#define JD_CONFIG_STATUS 1
#endif

#ifndef JD_CONFIG_IDENTIFY
#if JD_CONFIG_STATUS == 1
#define JD_CONFIG_IDENTIFY 0
#else
#define JD_CONFIG_IDENTIFY 1
#endif
#endif

#ifndef JD_CONFIG_DEV_SPEC_URL
#define JD_CONFIG_DEV_SPEC_URL 0
#endif

#define CONCAT_1(a, b) a##b
#define CONCAT_0(a, b) CONCAT_1(a, b)
#ifndef STATIC_ASSERT
#define STATIC_ASSERT(e) enum { CONCAT_0(_static_assert_, __LINE__) = 1 / ((e) ? 1 : 0) };
#endif

#ifndef JD_EVENT_QUEUE_SIZE
#define JD_EVENT_QUEUE_SIZE 128
#endif

#ifndef JD_TIM_OVERHEAD
#define JD_TIM_OVERHEAD 12
#endif

// this is timing overhead (in us) of starting transmission
// see set_tick_timer() for how to calibrate this
#ifndef JD_WR_OVERHEAD
#define JD_WR_OVERHEAD 8
#endif

#ifndef JD_CLIENT
#define JD_CLIENT 0
#endif

#endif