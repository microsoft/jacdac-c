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

// 255 minus size of the serial header, rounded down to 4
#define JD_SERIAL_PAYLOAD_SIZE 236
#define JD_SERIAL_FULL_HEADER_SIZE 16
// the COMMAND flag signifies that the device_identifier is the recipient
// (i.e., it's a command for the peripheral); the bit clear means device_identifier is the source
// (i.e., it's a report from peripheral or a broadcast message)
#define JD_FRAME_FLAG_COMMAND 0x01
// an ACK should be issued with CRC of this frame upon reception
#define JD_FRAME_FLAG_ACK_REQUESTED 0x02
// the device_identifier contains target service class number
#define JD_FRAME_FLAG_IDENTIFIER_IS_SERVICE_CLASS 0x04
// set on frames not received from the JD-wire
#define JD_FRAME_FLAG_LOOPBACK 0x80

#define JD_FRAME_SIZE(pkt) ((pkt)->size + 12)

#define JD_NOLOG(...) ((void)0)

// LOG(...) to be defined in particular .c files as either JD_LOG or JD_NOLOG

#ifdef JD_DEBUG_MODE
#define ERROR(msg, ...)                                                                            \
    do {                                                                                           \
        jd_debug_signal_error();                                                                   \
        JD_LOG("JD-ERROR: " msg, ##__VA_ARGS__);                                                   \
    } while (0)
#else
#define ERROR(msg, ...)                                                                            \
    do {                                                                                           \
        JD_LOG("JD-ERROR: " msg, ##__VA_ARGS__);                                                   \
    } while (0)
#endif

#ifndef JD_CONFIG_TEMPERATURE
#define JD_CONFIG_TEMPERATURE 0
#endif

#define CONCAT_1(a, b) a##b
#define CONCAT_0(a, b) CONCAT_1(a, b)
#ifndef STATIC_ASSERT
#define STATIC_ASSERT(e) enum { CONCAT_0(_static_assert_, __LINE__) = 1 / ((e) ? 1 : 0) };
#endif

#endif