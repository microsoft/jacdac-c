// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef JD_IO_H
#define JD_IO_H

#include "jd_config.h"

void jd_led_set(int state);
void jd_led_blink(int us);
void jd_power_enable(int en);

#if JD_CONFIG_STATUS == 1
void jd_status_init(void);
void jd_status_process(void);
int jd_status_handle_packet(jd_packet_t *pkt);
void jd_status_set_ch(int ch, uint8_t v);
#endif

#define JD_BLINK_COLOR_OFF 0b000
#define JD_BLINK_COLOR_RED 0b100
#define JD_BLINK_COLOR_GREEN 0b010
#define JD_BLINK_COLOR_BLUE 0b001
#define JD_BLINK_COLOR_YELLOW 0b110
#define JD_BLINK_COLOR_MAGENTA 0b101
#define JD_BLINK_COLOR_CYAN 0b011
#define JD_BLINK_COLOR_WHITE 0b111

#define JD_BLINK_DURATION_FAINT 0
#define JD_BLINK_DURATION_FAST 1
#define JD_BLINK_DURATION_SLOW 2

#define JD_BLINK_REPEAT_1 1
#define JD_BLINK_REPEAT_2 2
#define JD_BLINK_REPEAT_3 3
#define JD_BLINK_REPEAT_4 4
#define JD_BLINK_REPEAT_5 5
#define JD_BLINK_REPEAT_6 6
#define JD_BLINK_REPEAT_7 7

#define JD_BLINK(num_rep, duration, color)                                                         \
    JD_BLINK_COLOR_##color | (JD_BLINK_REPEAT_##num_rep << 3) | (JD_BLINK_DURATION_##duration)

#define JD_BLINK_CONNECTED JD_BLINK(1, FAINT, GREEN)
#define JD_BLINK_IDENTIFY JD_BLINK(7, FAST, BLUE)
#define JD_BLINK_STARTUP JD_BLINK(3, FAST, GREEN)

void jd_blink(uint8_t encoded);

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

// (duration+1)*500ms
#define JD_GLOW_DURATION_HALF_SECOND 0
#define JD_GLOW_DURATION_ONE_SECOND 1
#define JD_GLOW_DURATION_ONE_HALF_SECOND 2
#define JD_GLOW_DURATION_TWO_SECOND 3

#define _JD_GLOW_COLOR(g) (((g) >> 0) & 3)
#define _JD_GLOW_CHANNEL(g) (((g) >> 4) & 3)
#define _JD_GLOW_GAP(g) (((g) >> 8) & 3)
#define _JD_GLOW_DURATION(g) (((g) >> 12) & 3)
#define _JD_GLOW_SPEED(g) ((((g) >> 16) & 3) * 64)

#define JD_GLOW(speed, duration, gap, channel, color)                                              \
    JD_BLINK_COLOR_##color | (JD_GLOW_##channel << 4) | (JD_GLOW_DURATION_##gap << 8) |            \
        (JD_GLOW_DURATION_##duration << 12) | (JD_GLOW_SPEED_##speed << 16)

#define JD_GLOW_OFF(channel) (JD_GLOW_##channel << 12)

#define JD_GLOW_BRAIN_CONNECTED JD_GLOW_OFF(CH_0)
#define JD_GLOW_BRAIN_DISCONNECTED JD_GLOW(SLOW, HALF_SECOND, ONE_SECOND, CH_0, RED)
#define JD_GLOW_UNKNOWN JD_GLOW(SLOW, HALF_SECOND, HALF_SECOND, CH_0, YELLOW)

#define JD_GLOW_PROTECT JD_GLOW_OFF(CH_3)

void jd_glow(uint32_t glow);

#endif
