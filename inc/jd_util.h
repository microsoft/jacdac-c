// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include "jd_physical.h"

void jd_panic(void);
uint64_t jd_device_id(void);

uint32_t jd_random_around(uint32_t v);
uint32_t jd_random(void);
void jd_seed_random(uint32_t s);
uint32_t jd_hash_fnv1a(const void *data, unsigned len);
uint16_t jd_crc16(const void *data, uint32_t size);
int jd_shift_frame(jd_frame_t *frame);
void jd_reset_frame(jd_frame_t *frame);
void *jd_push_in_frame(jd_frame_t *frame, unsigned service_num, unsigned service_cmd,
                       unsigned service_size);
// jd_should_sample() will try to keep the period sampling rate, when delayed
bool jd_should_sample(uint32_t *sample, uint32_t period);
// jd_should_sample_delay() will wait at least `period` until next sampling
bool jd_should_sample_delay(uint32_t *sample, uint32_t period);

static inline bool is_before(uint32_t a, uint32_t b) {
    return ((b - a) >> 29) == 0;
}

// check if given timestamp is already in the past, regardless of overflows on 'now'
// the moment has to be no more than ~500 seconds in the past
static inline bool in_past(uint32_t moment) {
    extern uint32_t now;
    return ((now - moment) >> 29) == 0;
}
static inline bool in_future(uint32_t moment) {
    extern uint32_t now;
    return ((moment - now) >> 29) == 0;
}