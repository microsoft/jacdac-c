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

#if JD_DCFG
void jd_rgbext_init(int type, uint8_t pin);
void jd_rgbext_set(uint8_t r, uint8_t g, uint8_t b);
bool jd_status_has_color(void);
#else
static inline bool jd_status_has_color(void) {
#if defined(PIN_LED_R) || defined(LED_SET_RGB)
    return true;
#else
    return false;
#endif
}
#endif
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
#define JD_BLINK_DURATION_VERY_SLOW 3

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
#define _JD_BLINK_DURATION(encoded) (((encoded) >> 6) & 3)

#define JD_BLINK(duration, r, c)                                                                   \
    (_JD_CONCAT(JD_BLINK_COLOR_, c) | (_JD_CONCAT(JD_BLINK_REPEAT_, r) << 3) |                     \
     (_JD_CONCAT(JD_BLINK_DURATION_, duration) << 6))

#define JD_BLINK_INFO(c) JD_BLINK(SLOW, 1, c)
#define JD_BLINK_ERROR(c) JD_BLINK(FAST, 1, c)

#define JD_BLINK_CONNECTED JD_BLINK(FAINT, 1, GREEN)
#define JD_BLINK_IDENTIFY JD_BLINK(FAST, 7, BLUE)
#define JD_BLINK_STARTUP JD_BLINK(VERY_SLOW, 1, GREEN)

#define JD_BLINK_ERROR_LINE JD_BLINK_ERROR(YELLOW)
#define JD_BLINK_ERROR_OVF JD_BLINK_ERROR(MAGENTA)
#define JD_BLINK_ERROR_GENERAL JD_BLINK_ERROR(RED)

extern uint8_t jd_connected_blink;

void jd_blink(uint8_t encoded);

// has to be 4 or 8
#define JD_GLOW_CHANNELS 4

// highest non-off channel wins
#define JD_GLOW_CH_0 0
#define JD_GLOW_CH_1 1
#define JD_GLOW_CH_2 2
#define JD_GLOW_CH_3 3

#define JD_GLOW_SPEED_OFF 0
#define JD_GLOW_SPEED_ON 1
#define JD_GLOW_SPEED_VERY_SLOW 2
#define JD_GLOW_SPEED_SLOW 3
#define JD_GLOW_SPEED_FAST 4
#define JD_GLOW_SPEED_VERY_SLOW_LOW 5
#define JD_GLOW_SPEED_SLOW_BLINK 6
#define JD_GLOW_SPEED_FAST_BLINK 7

#define _JD_GLOW_COLOR(g) (((g) >> 0) & 7)
#define _JD_GLOW_SPEED(g) (((g) >> 3) & 7)
#define _JD_GLOW_CHANNEL(g) (((g) >> 8) & (JD_GLOW_CHANNELS - 1))

#define JD_GLOW(speed, channel, color)                                                             \
    (_JD_CONCAT(JD_BLINK_COLOR_, color) | (_JD_CONCAT(JD_GLOW_SPEED_, speed) << 3) |               \
     (_JD_CONCAT(JD_GLOW_, channel) << 8))

#define JD_GLOW_OFF(channel) (_JD_CONCAT(JD_GLOW_, channel) << 8)

#define JD_GLOW_BRAIN_CONNECTION_CH CH_1

#define JD_GLOW_BRAIN_CONNECTED JD_GLOW_OFF(JD_GLOW_BRAIN_CONNECTION_CH)
#define JD_GLOW_BRAIN_DISCONNECTED JD_GLOW(VERY_SLOW, JD_GLOW_BRAIN_CONNECTION_CH, RED)
#define JD_GLOW_UNKNOWN JD_GLOW(SLOW, JD_GLOW_BRAIN_CONNECTION_CH, YELLOW)

// cloud stuff
#define JD_GLOW_CLOUD_CONNECTION_CH CH_2
#define JD_GLOW_CLOUD_CONNECTING_TO_NETWORK JD_GLOW(SLOW, JD_GLOW_CLOUD_CONNECTION_CH, YELLOW)
#define JD_GLOW_CLOUD_CONNECTING_TO_CLOUD JD_GLOW(FAST, JD_GLOW_CLOUD_CONNECTION_CH, YELLOW)
#define JD_GLOW_CLOUD_CONNECTED_TO_CLOUD JD_GLOW(VERY_SLOW_LOW, JD_GLOW_CLOUD_CONNECTION_CH, BLUE)
#define JD_GLOW_CLOUD_NOT_CONNECTED_TO_CLOUD                                                       \
    JD_GLOW(VERY_SLOW_LOW, JD_GLOW_CLOUD_CONNECTION_CH, RED)

#define JD_BLINK_CLOUD_UPLOADED JD_BLINK_INFO(BLUE)
#define JD_BLINK_CLOUD_ERROR JD_BLINK_ERROR(RED)

void jd_glow(uint32_t glow);

#endif
