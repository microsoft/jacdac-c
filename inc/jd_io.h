// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef JD_IO_H
#define JD_IO_H

#include "jd_config.h"

void jd_power_enable(int en);

#if JD_CONFIG_STATUS == 1
void jd_status_init(void);
void jd_status_process(void);
int jd_status_handle_packet(jd_packet_t *pkt);
#endif

/*
 * LED is used to indicate status (glow) and events (blink).
 *
 * Events are queued (up to a small limit).
 *
 * Status has several channels. Services are meant to update their status frequently (say every
 * second). Status can be "off" or some color pattern. We display the highest-numbered non-off
 * channel.
 */

#define JD_BLINK_COLOR_OFF 0b000
#define JD_BLINK_COLOR_RED 0b001
#define JD_BLINK_COLOR_GREEN 0b010
#define JD_BLINK_COLOR_BLUE 0b100
#define JD_BLINK_COLOR_YELLOW 0b011
#define JD_BLINK_COLOR_MAGENTA 0b101
#define JD_BLINK_COLOR_CYAN 0b110
#define JD_BLINK_COLOR_WHITE 0b111

#define JD_BLINK_DURATION_FAINT 0
#define JD_BLINK_DURATION_FAST 1
#define JD_BLINK_DURATION_SLOW 2

#define JD_BLINK_REPEAT_0 0
#define JD_BLINK_REPEAT_1 1
#define JD_BLINK_REPEAT_2 2
#define JD_BLINK_REPEAT_3 3
#define JD_BLINK_REPEAT_4 4
#define JD_BLINK_REPEAT_5 5
#define JD_BLINK_REPEAT_6 6
#define JD_BLINK_REPEAT_7 7

#define _JD_CONCAT(x, y) x##y

#define _JD_BLINK_COLOR(encoded) (((encoded) >> 0) & 7)
#define _JD_BLINK_REPETITIONS(encoded) (((encoded) >> 3) & 7)
#define _JD_BLINK_DURATION(encoded) (((encoded) >> 12) & 3)

#define _JD_BLINK_PAIR(r, c)                                                                       \
    (_JD_CONCAT(JD_BLINK_COLOR_, c) | (_JD_CONCAT(JD_BLINK_REPEAT_, r) << 3))

static inline uint16_t _jd_blink_shift(uint16_t b) {
    return ((b >> 6) & 0x3f) | (b & 0xf000);
}

#define JD_BLINK(duration, r1, c1, r2, c2)                                                         \
    _JD_BLINK_PAIR(r1, c1) | (_JD_BLINK_PAIR(r2, c2) << 6) |                                       \
        (_JD_CONCAT(JD_BLINK_DURATION_, duration) << 12)

#define JD_BLINK_CONNECTED JD_BLINK(FAINT, 1, BLUE, 0, OFF)
#define JD_BLINK_IDENTIFY JD_BLINK(FAST, 7, BLUE, 0, OFF)
#define JD_BLINK_STARTUP JD_BLINK(FAST, 1, GREEN, 1, BLUE)

#define JD_BLINK_ERROR(c) JD_BLINK(FAST, 4, c, 0, OFF)
#define JD_BLINK_ERROR_LINE JD_BLINK_ERROR(YELLOW)
#define JD_BLINK_ERROR_OVF JD_BLINK_ERROR(MAGENTA)
#define JD_BLINK_ERROR_GENERAL JD_BLINK_ERROR(RED)

void jd_blink(uint16_t encoded);

// highest non-off channel wins
#define JD_GLOW_CH_0 0
#define JD_GLOW_CH_1 1
#define JD_GLOW_CH_2 2
#define JD_GLOW_CH_3 3

// speed*64
#define JD_GLOW_SPEED_INSTANT 0
#define JD_GLOW_SPEED_FAST 1
#define JD_GLOW_SPEED_SLOW 2
#define JD_GLOW_SPEED_VERY_SLOW 3

// (duration+1)*524ms
#define JD_GLOW_DURATION_HALF_SECOND 0
#define JD_GLOW_DURATION_ONE_SECOND 1
#define JD_GLOW_DURATION_ONE_HALF_SECOND 2
#define JD_GLOW_DURATION_TWO_SECOND 3

#define _JD_GLOW_COLOR(g) (((g) >> 0) & 3)
#define _JD_GLOW_CHANNEL(g) (((g) >> 4) & 3)
#define _JD_GLOW_GAP(g) (((((g) >> 8) & 3) + 1) * (512 << 10))
#define _JD_GLOW_DURATION(g) (((((g) >> 12) & 3) + 1) * (512 << 10))
#define _JD_GLOW_SPEED(g) ((((g) >> 16) & 3) * 64)

#define JD_GLOW(speed, duration, gap, channel, color)                                              \
    _JD_CONCAT(JD_BLINK_COLOR_, color) | (_JD_CONCAT(JD_GLOW_, channel) << 4) |                    \
        (_JD_CONCAT(JD_GLOW_DURATION_, gap) << 8) |                                                \
        (_JD_CONCAT(JD_GLOW_DURATION_, duration) << 12) |                                          \
        (_JD_CONCAT(JD_GLOW_SPEED_, speed) << 16)

#define JD_GLOW_OFF(channel) (_JD_CONCAT(JD_GLOW_, channel) << 12)
#define JD_GLOW_PROTECT JD_GLOW_OFF(CH_3)

#define JD_GLOW_BRAIN_CONNECTION_CH CH_1

#define JD_GLOW_BRAIN_CONNECTED JD_GLOW_OFF(JD_GLOW_BRAIN_CONNECTION_CH)
#define JD_GLOW_BRAIN_DISCONNECTED                                                                 \
    JD_GLOW(SLOW, HALF_SECOND, ONE_SECOND, JD_GLOW_BRAIN_CONNECTION_CH, RED)
#define JD_GLOW_UNKNOWN JD_GLOW(SLOW, HALF_SECOND, HALF_SECOND, JD_GLOW_BRAIN_CONNECTION_CH, YELLOW)

// cloud stuff
#define JD_GLOW_CLOUD_CONNECTION_CH CH_2
#define JD_GLOW_CLOUD_CONNECTING_TO_NETWORK                                                        \
    JD_GLOW(FAST, HALF_SECOND, HALF_SECOND, JD_GLOW_CLOUD_CONNECTION_CH, YELLOW)
#define JD_GLOW_CLOUD_CONNECTING_TO_CLOUD                                                          \
    JD_GLOW(FAST, HALF_SECOND, HALF_SECOND, JD_GLOW_CLOUD_CONNECTION_CH, GREEN)
#define JD_GLOW_CLOUD_CONNECTED_TO_CLOUD                                                           \
    JD_GLOW(FAST, TWO_SECOND, TWO_SECOND, JD_GLOW_CLOUD_CONNECTION_CH, GREEN)
#define JD_GLOW_CLOUD_NOT_CONNECTED_TO_CLOUD                                                       \
    JD_GLOW(FAST, TWO_SECOND, TWO_SECOND, JD_GLOW_CLOUD_CONNECTION_CH, YELLOW)

#define JD_BLINK_CLOUD_UPLOADED JD_BLINK(FAST, 1, BLUE, 0, OFF)
#define JD_BLINK_CLOUD_ERROR JD_BLINK_ERROR(BLUE)

void jd_glow(uint32_t glow);

#endif
