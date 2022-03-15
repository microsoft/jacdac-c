// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef JD_UTIL_H
#define JD_UTIL_H

#include "jd_physical.h"

__attribute__((noreturn)) void jd_panic(void);
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

// sizeof(dst) == len*2 + 1
void jd_to_hex(char *dst, const void *src, size_t len);

#define JD_ASSERT(cond)                                                                            \
    do {                                                                                           \
        if (!(cond))                                                                               \
            jd_panic();                                                                            \
    } while (0)


// jd_queue.c
typedef struct _queue *jd_queue_t;
jd_queue_t jd_queue_alloc(unsigned size);
int jd_queue_push(jd_queue_t q, jd_frame_t *pkt);
jd_frame_t *jd_queue_front(jd_queue_t q);
void jd_queue_shift(jd_queue_t q);
void jd_queue_test(void);
int jd_queue_will_fit(jd_queue_t q, unsigned size);

#endif
