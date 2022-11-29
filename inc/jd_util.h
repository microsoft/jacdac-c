// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef JD_UTIL_H
#define JD_UTIL_H

#include "jd_physical.h"
#include <stdarg.h>

uint64_t jd_device_id(void);

uint32_t jd_random_around(uint32_t v);
uint32_t jd_random(void);
void jd_seed_random(uint32_t s);
uint32_t jd_hash_fnv1a(const void *data, unsigned len);
// CRC-16-CCITT polynomial 0x1021
uint16_t jd_crc16(const void *data, uint32_t size);
// CRC-32 ISO 3309 polynomial 0x04C11DB7
uint32_t jd_crc32(const void *data, uint32_t size);

// for SD card commands; includes stop bit
uint8_t jd_sd_crc7(const void *data, uint32_t size);
// for SD card data
uint16_t jd_sd_crc16(const void *data, uint32_t size);

int jd_shift_frame(jd_frame_t *frame);
void jd_reset_frame(jd_frame_t *frame);
void jd_pkt_set_broadcast(jd_packet_t *pkt, uint32_t service_class);
void *jd_push_in_frame(jd_frame_t *frame, unsigned service_num, unsigned service_cmd,
                       unsigned service_size);
// jd_should_sample() will try to keep the period sampling rate, when delayed
bool jd_should_sample(uint32_t *sample, uint32_t period);
// jd_should_sample_delay() will wait at least `period` until next sampling
bool jd_should_sample_delay(uint32_t *sample, uint32_t period);

// approx. a <= b modulo overflows
// checks if time from a to b is at most 0x7fff_ffff
static inline bool is_before(uint32_t a, uint32_t b) {
    return ((b - a) >> 31) == 0;
}

// check if given timestamp is already in the past, regardless of overflows on 'now'
// the moment has to be no more than ~2100 seconds in the past
static inline bool in_past(uint32_t moment) {
    extern uint32_t now;
    return is_before(moment, now);
}
static inline bool in_future(uint32_t moment) {
    extern uint32_t now;
    return is_before(now, moment);
}

#if JD_MS_TIMER
// this works up to around 24 days
static inline bool in_past_ms(uint32_t moment) {
    extern uint32_t now_ms;
    return is_before(moment, now_ms);
}
static inline bool in_future_ms(uint32_t moment) {
    extern uint32_t now_ms;
    return is_before(now_ms, moment);
}
bool jd_should_sample_ms(uint32_t *sample, uint32_t period);
#endif

// sizeof(dst) == len*2 + 1
void jd_to_hex(char *dst, const void *src, size_t len);
// sizeof(dst) >= strlen(dst)/2; returns length of dst
int jd_from_hex(void *dst, const char *src);

#if JD_ADVANCED_STRING
// buf is 64 bytes long
void jd_print_double(char *buf, double d, int numdigits);
#endif

// These allocate the resulting string
// often used with custom %-s format which will free them
#if JD_FREE_SUPPORTED
char *jd_vsprintf_a(const char *format, va_list ap);
__attribute__((format(printf, 1, 2))) char *jd_sprintf_a(const char *format, ...);
char *jd_to_hex_a(const void *src, size_t len);
char *jd_device_short_id_a(uint64_t long_id);
void *jd_from_hex_a(const char *src, unsigned *size);

char *jd_strdup(const char *s);
char *jd_concat_many(const char **parts);
char *jd_concat2(const char *a, const char *b);
char *jd_concat3(const char *a, const char *b, const char *c);
char *jd_urlencode(const char *src);
char *jd_json_escape(const char *str, unsigned sz);
jd_frame_t *jd_dup_frame(const jd_frame_t *frame);
#endif

#if JD_VERBOSE_ASSERT
__attribute__((noreturn)) void jd_assert_fail(const char *expr, const char *file, unsigned line,
                                              const char *funname);
__attribute__((noreturn)) void jd_panic_core(const char *file, unsigned line, const char *funname);
#else
#define jd_assert_fail(...) hw_panic()
#define jd_panic_core(...) hw_panic()
#endif

#if 1
#define JD_ASSERT(cond)                                                                            \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            jd_assert_fail(#cond, __FILE__, __LINE__, __func__);                                   \
        }                                                                                          \
    } while (0)
#else
#define JD_ASSERT(cond) ((void)0)
#endif

#define JD_PANIC() jd_panic_core(__FILE__, __LINE__, __func__)

// never compiled out
#define JD_CHK(call)                                                                               \
    do {                                                                                           \
        if ((call) != 0) {                                                                         \
            jd_assert_fail(#call, __FILE__, __LINE__, __func__);                                   \
        }                                                                                          \
    } while (0)

// jd_queue.c
typedef struct jd_queue *jd_queue_t;
jd_queue_t jd_queue_alloc(unsigned size);
int jd_queue_push(jd_queue_t q, jd_frame_t *pkt);
jd_frame_t *jd_queue_front(jd_queue_t q);
void jd_queue_shift(jd_queue_t q);
void jd_queue_test(void);
int jd_queue_will_fit(jd_queue_t q, unsigned size);

// jd_bqueue.c
typedef struct jd_bqueue *jd_bqueue_t;
jd_bqueue_t jd_bqueue_alloc(unsigned size);
unsigned jd_bqueue_occupied_bytes(jd_bqueue_t q);
unsigned jd_bqueue_free_bytes(jd_bqueue_t q);
// returns 0 on success, -1 when full
int jd_bqueue_push(jd_bqueue_t q, const void *data, unsigned len);
// returns 0 if `size` bytes was popped, and -1 when nothing was popped
int jd_bqueue_pop_atomic(jd_bqueue_t q, void *dst, unsigned size);
// returns number of bytes popped
unsigned jd_bqueue_pop_at_most(jd_bqueue_t q, void *dst, unsigned maxsize);
// returns -1 when empty
int jd_bqueue_pop_byte(jd_bqueue_t q);
// low-level, not thread-safe interface
unsigned jd_bqueue_available_cont_data(jd_bqueue_t q);
uint8_t *jd_bqueue_cont_data_ptr(jd_bqueue_t q);
void jd_bqueue_cont_data_advance(jd_bqueue_t q, unsigned sz);

void jd_utoa(unsigned k, char *s);
void jd_itoa(int n, char *s);
void jd_string_reverse(char *s);
int jd_vsprintf(char *dst, unsigned dstsize, const char *format, va_list ap);
__attribute__((format(printf, 3, 4))) int jd_sprintf(char *dst, unsigned dstsize,
                                                     const char *format, ...);
int jd_atoi(const char *s);

void jd_log_packet(jd_packet_t *pkt);
void jd_device_short_id(char short_id[5], uint64_t long_id);

#endif
