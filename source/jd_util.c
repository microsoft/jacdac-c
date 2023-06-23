// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_protocol.h"

#define ALIGN(x) (((x) + 3) & ~3)

static uint32_t seed;

// https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
uint32_t jd_hash_fnv1a(const void *data, unsigned len) {
    const uint8_t *d = (const uint8_t *)data;
    uint32_t h = 0x811c9dc5;
    while (len--)
        h = (h ^ *d++) * 0x1000193;
    return h;
}

#ifndef JD_PANIC
__attribute__((weak)) void JD_PANIC(void) {
    hw_panic();
}
#endif

__attribute__((weak)) uint64_t jd_device_id(void) {
    return hw_device_id();
}

void jd_seed_random(uint32_t s) {
    seed = (seed * 0x1000193) ^ s;
}

uint32_t jd_random(void) {
    if (seed == 0)
        jd_seed_random(13);

    // xorshift algorithm
    uint32_t x = seed;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    seed = x;
    return x;
}

// return v +/- 25% or so
uint32_t jd_random_around(uint32_t v) {
    uint32_t mask = 0xfffffff;
    while (mask > v)
        mask >>= 1;
    return (v - (mask >> 1)) + (jd_random() & mask);
}

// https://wiki.nicksoft.info/mcu:pic16:crc-16:home
uint16_t jd_crc16(const void *data0, uint32_t size) {
    const uint8_t *ptr = (const uint8_t *)data0;
    uint16_t crc = 0xffff;
    while (size--) {
        uint8_t data = *ptr++;
        uint8_t x = (crc >> 8) ^ data;
        x ^= x >> 4;
        crc = (crc << 8) ^ (x << 12) ^ (x << 5) ^ x;
    }
    return crc;
}

bool jd_frame_crc_ok(jd_frame_t *frame) {
    return jd_crc16((uint8_t *)frame + 2, JD_FRAME_SIZE(frame) - 2) == frame->crc;
}

void jd_compute_crc(jd_frame_t *frame) {
    frame->crc = jd_crc16((uint8_t *)frame + 2, JD_FRAME_SIZE(frame) - 2);
}

int jd_shift_frame(jd_frame_t *frame) {
    int psize = frame->size;
    jd_packet_t *pkt = (jd_packet_t *)frame;
    int oldsz = pkt->service_size + 4;
    if (ALIGN(oldsz) >= psize)
        return 0; // nothing to shift

    int ptr;
    if (frame->data[oldsz] == 0xff) {
        ptr = frame->data[oldsz + 1];
        if (ptr >= psize)
            return 0; // End-of-frame
        if (ptr <= oldsz) {
            return 0; // don't let it go back, must be some corruption
        }
    } else {
        ptr = ALIGN(oldsz);
    }

    // assume the first one got the ACK sorted
    frame->flags &= ~JD_FRAME_FLAG_ACK_REQUESTED;

    uint8_t *src = &frame->data[ptr];
    int newsz = *src + 4;
    if (ptr + newsz > psize) {
        return 0;
    }
    jd_word_move(frame->data, src, newsz + 3);
    // store ptr
    ptr += ALIGN(newsz);
    frame->data[newsz] = 0xff;
    frame->data[newsz + 1] = ptr;

    return 1;
}

void jd_pkt_set_broadcast(jd_packet_t *pkt, uint32_t service_class) {
    jd_frame_t *f = (jd_frame_t *)pkt;
    f->flags |= JD_FRAME_FLAG_BROADCAST;
    f->device_identifier = service_class | JD_DEVICE_IDENTIFIER_BROADCAST_MARK;
    pkt->service_index = JD_SERVICE_INDEX_BROADCAST;
}

void jd_reset_frame(jd_frame_t *frame) {
    frame->size = 0x00;
}

void *jd_push_in_frame(jd_frame_t *frame, unsigned service_num, unsigned service_cmd,
                       unsigned service_size) {
    if (service_num >> 8)
        JD_PANIC();
    if (service_cmd >> 16)
        JD_PANIC();
    uint8_t *dst = frame->data + frame->size;
    unsigned szLeft = frame->data + JD_SERIAL_PAYLOAD_SIZE - dst;
    if (service_size + 4 > szLeft)
        return NULL;
    *dst++ = service_size;
    *dst++ = service_num;
    *dst++ = service_cmd & 0xff;
    *dst++ = service_cmd >> 8;
    frame->size += ALIGN(service_size + 4);
    return dst;
}

#if JD_MS_TIMER
bool jd_should_sample_ms(uint32_t *sample, uint32_t period) {
    if (in_future_ms(*sample))
        return false;

    *sample += period;

    if (!in_future_ms(*sample))
        // we lost some samples
        *sample = now_ms + period;

    return true;
}
#endif

bool jd_should_sample(uint32_t *sample, uint32_t period) {
    if (in_future(*sample))
        return false;

    *sample += period;

    if (!in_future(*sample))
        // we lost some samples
        *sample = now + period;

    return true;
}

bool jd_should_sample_delay(uint32_t *sample, uint32_t period) {
    if (in_future(*sample))
        return false;

    *sample = now + period;

    return true;
}

void jd_to_hex(char *dst, const void *src, size_t len) {
    const char *hex = "0123456789abcdef";
    const uint8_t *p = src;
    for (size_t i = 0; i < len; ++i) {
        dst[i * 2] = hex[p[i] >> 4];
        dst[i * 2 + 1] = hex[p[i] & 0xf];
    }
    dst[len * 2] = 0;
}

static int hexdig(char c) {
    if ('0' <= c && c <= '9')
        return c - '0';
    c |= 0x20;
    if ('a' <= c && c <= 'f')
        return c - 'a' + 10;
    return -1;
}

int jd_from_hex(void *dst, const char *src) {
    uint8_t *dp = dst;
    int prev = 0;
    for (;;) {
        while (*src && hexdig(*src) == -1)
            src++;
        if (!*src)
            break;
        int v = hexdig(*src++);
        if (prev == 0)
            prev = (v << 4) | 0x100; // make sure it's not zero
        else {
            if (dst)
                *dp++ = (prev | v) & 0xff;
            else
                dp++;
            prev = 0;
        }
    }
    return dp - (uint8_t *)dst;
}

void *jd_from_hex_a(const char *src, unsigned *size) {
    unsigned sz = strlen(src) / 2;
    void *r = jd_alloc(sz);
    *size = jd_from_hex(r, src);
    return r;
}

#if JD_VERBOSE_ASSERT
void jd_assert_fail(const char *expr, const char *file, unsigned line, const char *funname) {
    DMESG("assertion '%s' failed at %s:%d in %s", expr, file, line, funname);
    hw_panic();
}
void jd_panic_core(const char *file, unsigned line, const char *funname) {
    DMESG("JD_PANIC() at %s:%d in %s", file, line, funname);
    hw_panic();
}
#endif

/**
 * Performs an in buffer reverse of a given char array.
 *
 * @param s the string to reverse.
 */
void jd_string_reverse(char *s) {
    if (s == NULL)
        return;

    char *j;
    int c;

    j = s + strlen(s) - 1;

    while (s < j) {
        c = *s;
        *s++ = *j;
        *j-- = c;
    }
}

/**
 * Converts a given integer into a string representation.
 *
 * @param n The number to convert.
 *
 * @param s A pointer to the buffer where the resulting string will be stored.
 */
void jd_itoa(int n, char *s) {
    if (s == NULL)
        return;

    if (n < 0) {
        *s++ = '-';
        n = -n;
    }
    jd_utoa(n, s);
}

void jd_utoa(unsigned k, char *s) {
    char *s0 = s;

    if (s == NULL)
        return;

    // Calculate each character, starting with the LSB.
    do {
        *s++ = (k % 10) + '0';
    } while ((k /= 10) > 0);

    // Terminate the string.
    *s = '\0';

    // Flip the order.
    jd_string_reverse(s0);
}

int jd_atoi(const char *s) {
    int neg = 0;
    int r = 0;
    if (*s == '-') {
        neg = 1;
        s++;
    }
    for (;;) {
        int k = *s++ - '0';
        if (0 <= k && k <= 9) {
            r *= 10;
            r += k;
        } else {
            break;
        }
    }

    if (neg)
        return -r;
    else
        return r;
}

/**
 * Copy numwords/4*4 bytes to dst from src, left-to-right.
 * Special case of memmove()
 */
void jd_word_move(void *dst, const void *src, unsigned numbytes) {
    uint32_t *dp = dst;
    const uint32_t *sp = src;
    numbytes >>= 2;
    while (numbytes--)
        *dp++ = *sp++;
}

static void writeNum(char *buf, uintptr_t n, bool full) {
    int i = 0;
    int sh = sizeof(uintptr_t) * 8 - 4;
    while (sh >= 0) {
        int d = (n >> sh) & 0xf;
        if (full || d || sh == 0 || i) {
            buf[i++] = d > 9 ? 'A' + d - 10 : '0' + d;
        }
        sh -= 4;
    }
    buf[i] = 0;
}

typedef struct {
    char *dst;
    char *dstend;
#if JD_ADVANCED_STRING
    unsigned ulen;
#endif
} printf_ctx_t;

static void write_n(printf_ctx_t *ctx, const char *src, int srclen) {
    int left = ctx->dstend - ctx->dst;
#if JD_ADVANCED_STRING
    for (int i = 0; i < srclen; ++i)
        if ((src[i] & 0xC0) != 0x80)
            ctx->ulen++;
#endif
    if (left > 0) {
        int srctrimmed = srclen >= left ? left - 1 : srclen;
        memcpy(ctx->dst, src, srctrimmed);
        ctx->dst[srctrimmed] = 0;
    }
    ctx->dst += srclen;
}

#define WRITEN(p, sz) write_n(&ctx, p, sz)

#if !JD_ADVANCED_STRING
static
#endif
    int
    jd_vsprintf_ext(char *dst, unsigned dstsize, const char *format, unsigned *ulen, va_list ap) {
    const char *end = format;
    printf_ctx_t ctx = {
        .dst = dst,
        .dstend = dst + dstsize,
    };
#if JD_ADVANCED_STRING
    char buf[64];
#else
    char buf[sizeof(uintptr_t) * 2 + 8];
#endif

    for (;;) {
        char c = *end++;
        if (c == 0 || c == '%') {
            if (format != end)
                WRITEN(format, end - format - 1);
            if (c == 0)
                break;

#if JD_ADVANCED_STRING
#if JD_LORA
            uint8_t fmtc = 0;
            while ('0' <= *end && *end <= '9')
                fmtc = *end++;
#endif

#if JD_FREE_SUPPORTED
            int do_free = 0;
            if (end[0] == '-' && end[1] == 's') {
                if (dst)
                    do_free = 1;
                end++;
            }
#endif

            c = *end++;
            buf[1] = 0;
            switch (c) {
            case 'c':
                buf[0] = va_arg(ap, int);
                break;
            case 'd':
                jd_itoa(va_arg(ap, int), buf);
                break;
            case 'u':
                jd_utoa(va_arg(ap, unsigned), buf);
                break;
            case 'x':
            case 'X':
                buf[0] = '0';
                buf[1] = 'x';
                writeNum(buf + 2, va_arg(ap, int), false);
#if JD_LORA
                if (c == 'X' && fmtc == '2') {
                    buf[0] = buf[8];
                    buf[1] = buf[9];
                    buf[2] = 0;
                }
#endif
                break;
            case 'p':
                buf[0] = '0';
                buf[1] = 'x';
                writeNum(buf + 2, va_arg(ap, uintptr_t), false);
                break;
            case 'f': {
                double f = va_arg(ap, double);
                jd_print_double(buf, f, 8);
                break;
            }
            case '*': {
                int len = va_arg(ap, int);
                uint8_t *d = va_arg(ap, void *);
                if (*end++ == 'p') {
                    while (len > 0) {
                        int ch = sizeof(buf) / 2 - 1;
                        if (len < ch)
                            ch = len;
                        jd_to_hex(buf, d, ch);
                        WRITEN(buf, ch * 2);
                        d += ch;
                        len -= ch;
                    }
                    buf[0] = 0;
                } else {
                    buf[0] = '?';
                }
                break;
            }
            case 's': {
                const char *val = va_arg(ap, const char *);
                if (!val)
                    val = "(null)";
                WRITEN(val, strlen(val));
                buf[0] = 0;
#if JD_FREE_SUPPORTED
                if (do_free)
                    jd_free((void *)val);
#endif
                break;
            }
            case '%':
                buf[0] = c;
                break;
            default:
                buf[0] = '?';
                break;
            }
#else
            uint32_t val = va_arg(ap, uint32_t);

            c = *end++;
            buf[1] = 0;
            switch (c) {
            case 'c':
                buf[0] = val;
                break;
            case 'd':
                jd_itoa(val, buf);
                break;
            case 'x':
            case 'p':
            case 'X':
                buf[0] = '0';
                buf[1] = 'x';
                writeNum(buf + 2, val, c != 'x');
                break;
            case 's':
                if ((void *)val == NULL)
                    val = (uint32_t) "(null)";
                WRITEN((char *)(void *)val, strlen((char *)(void *)val));
                buf[0] = 0;
                break;
            case '%':
                buf[0] = c;
                break;
            default:
                buf[0] = '?';
                break;
            }
#endif
            format = end;
            WRITEN(buf, strlen(buf));
        }
    }

#if JD_ADVANCED_STRING
    if (ulen)
        *ulen = ctx.ulen + 1;
#endif

    return ctx.dst - dst + 1;
}

int jd_vsprintf(char *dst, unsigned dstsize, const char *format, va_list ap) {
    return jd_vsprintf_ext(dst, dstsize, format, NULL, ap);
}

int jd_sprintf(char *dst, unsigned dstsize, const char *format, ...) {
    va_list arg;
    va_start(arg, format);
    int r = jd_vsprintf(dst, dstsize, format, arg);
    va_end(arg);
    return r;
}

// https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
// this is not jd_hash_fnv1a()!
static uint32_t hash_fnv1(const void *data, unsigned len) {
    const uint8_t *d = (const uint8_t *)data;
    uint32_t h = 0x811c9dc5;
    while (len--) {
        h = (h * 0x1000193) ^ *d++;
    }
    return h;
}

static uint32_t mkcd_hash(const void *buf, size_t length, int bits) {
    if (bits < 1) {
        return 0;
    }

    uint32_t h = hash_fnv1(buf, length);

    if (bits >= 32) {
        return h;
    } else {
        return (h ^ (h >> bits)) & ((1 << bits) - 1);
    }
}

void jd_device_short_id(char short_id[5], uint64_t long_id) {
    uint32_t h = mkcd_hash(&long_id, sizeof(long_id), 30);
    short_id[0] = 'A' + (h % 26);
    short_id[1] = 'A' + ((h / 26) % 26);
    short_id[2] = '0' + ((h / (26 * 26)) % 10);
    short_id[3] = '0' + ((h / (26 * 26 * 10)) % 10);
    short_id[4] = 0;
}

void jd_log_packet(jd_packet_t *pkt) {
    char shortid[5];
    char tmpbuf[65];
    unsigned sz = pkt->service_size;
    if (sz > (sizeof(tmpbuf) >> 1))
        sz = (sizeof(tmpbuf) >> 1);
    jd_to_hex(tmpbuf, pkt->data, sz);
    jd_device_short_id(shortid, pkt->device_identifier);
    DMESG("PKT %s %s/%d cmd=%x sz=%d %s%s", pkt->flags & JD_FRAME_FLAG_COMMAND ? "TO" : "FROM",
          shortid, pkt->service_index, pkt->service_command, pkt->service_size, tmpbuf,
          sz == pkt->service_size ? "" : "...");
}

#if JD_ADVANCED_STRING
#include <math.h>

#define NUMBER double

#define p10(v) __builtin_powi(10, v)

static const uint64_t pows[] = {
    1LL,           10LL,           100LL,           1000LL,           10000LL,
    100000LL,      1000000LL,      10000000LL,      100000000LL,      1000000000LL,
    10000000000LL, 100000000000LL, 1000000000000LL, 10000000000000LL, 100000000000000LL,
};

// The basic idea is we convert d to a 64 bit integer with numdigits
// digits, and then print it out, putting dot in the right place.

void jd_print_double(char *buf, NUMBER d, int numdigits) {
    if (d < 0) {
        *buf++ = '-';
        d = -d;
    }

    if (isnan(d)) {
        strcpy(buf, "NaN");
        return;
    }

    if (isinf(d)) {
        strcpy(buf, "inf");
        return;
    }

    if (d < 2.3e-308) {
        *buf++ = '0';
        *buf++ = 0;
        return;
    }

    if (numdigits < 4)
        numdigits = 4;
    int maxdigits = sizeof(NUMBER) == 4 ? 8 : 15;
    if (numdigits > maxdigits)
        numdigits = maxdigits;

    int pw = sizeof(NUMBER) == 4 ? (int)log10f(d) : (int)log10(d);
    int e = 1;

    // if outside 1e-6 -- 1e21 range, we use the e-notation
    if (d < 1e-6 || d > 1e21) {
        // normalize number to 1.XYZ, save e, and reset pw
        if (pw < 0)
            d *= p10(-pw);
        else
            d /= p10(pw);
        e = pw;
        pw = 0;
    }

    int trailingZ = 0;
    int dotAfter = pw + 1; // at which position the dot should be in the number

    uint64_t dd;

    if (pw >= numdigits) {
        // we're going to print out the whole number anyways, so ignore the requested precision if
        // too small
        numdigits = pw + 1;
        if (numdigits > maxdigits)
            numdigits = maxdigits;
    }

    // normalize number to be integer with exactly numdigits digits
    if (pw >= numdigits) {
        // if the number is larger than numdigits, we need trailing zeroes
        trailingZ = pw - numdigits + 1;
        dd = (uint64_t)(d / p10(trailingZ) + 0.5);
    } else {
        dd = (uint64_t)(d * p10(numdigits - pw - 1) + 0.5);
    }

    // if number is less than 1, we need 0.00...00 at the beginning
    if (dotAfter < 1) {
        *buf++ = '0';
        *buf++ = '.';
        int n = -dotAfter;
        while (n--)
            *buf++ = '0';
    }

    // now print out the actual number
    for (int i = numdigits - 1; i >= 0; i--) {
        uint64_t q = pows[i];
        // this may be faster than fp-division and fmod(); or maybe not
        // anyways, it works
        int k = '0';
        while (dd >= q) {
            dd -= q;
            k++;
        }
        *buf++ = k;
        // if we're after dot, and what's left is zeroes, stop
        if (dd == 0 && (numdigits - i) >= dotAfter)
            break;
        // print the dot, if we arrived at it
        if ((numdigits - i) == dotAfter)
            *buf++ = '.';
    }

    // print out remaining trailing zeroes if any
    while (trailingZ-- > 0)
        *buf++ = '0';

    // if we used e-notation, handle that
    if (e != 1) {
        *buf++ = 'e';
        if (e > 0)
            *buf++ = '+';
        jd_itoa(e, buf);
    } else {
        *buf = 0;
    }
}
#endif

#if JD_FREE_SUPPORTED
char *jd_vsprintf_a(const char *format, va_list ap) {
    va_list ap2;
    va_copy(ap2, ap);
    int len = jd_vsprintf(NULL, 0, format, ap);
    char *r = jd_alloc(len);
    jd_vsprintf(r, len, format, ap2);
    va_end(ap2);
    return r;
}

char *jd_sprintf_a(const char *format, ...) {
    va_list arg;
    va_start(arg, format);
    char *r = jd_vsprintf_a(format, arg);
    va_end(arg);
    return r;
}

char *jd_to_hex_a(const void *src, size_t len) {
    char *r = jd_alloc(len * 2 + 1);
    jd_to_hex(r, src, len);
    return r;
}

char *jd_device_short_id_a(uint64_t long_id) {
    char *r = jd_alloc(5);
    jd_device_short_id(r, long_id);
    return r;
}

static int urlencode_core(char *dst, const char *src) {
    int len = 0;
    while (*src) {
        uint8_t c = *src++;
        if (('0' <= c && c <= '9') || ('a' <= (c | 0x20) && (c | 0x20) <= 'z') ||
            (c == '-' || c == '.' || c == '_' || c == '~')) {
            if (dst)
                *dst++ = c;
            len++;
        } else {
            if (dst) {
                *dst++ = '%';
                jd_to_hex(dst, &c, 1);
                dst += 2;
            }
            len += 3;
        }
    }
    if (dst)
        *dst++ = 0;
    return len + 1;
}

char *jd_urlencode(const char *src) {
    int len = urlencode_core(NULL, src);
    char *r = jd_alloc(len);
    urlencode_core(r, src);
    return r;
}

char *jd_concat_many(const char **parts) {
    int len = 0;
    for (int i = 0; parts[i]; ++i)
        len += strlen(parts[i]);
    char *r = jd_alloc(len + 1);
    len = 0;

    for (int i = 0; parts[i]; ++i) {
        int k = strlen(parts[i]);
        memcpy(r + len, parts[i], k);
        len += k;
    }
    r[len] = 0;
    return r;
}

char *jd_concat3(const char *a, const char *b, const char *c) {
    const char *arr[] = {a, b, c, NULL};
    return jd_concat_many(arr);
}

char *jd_concat2(const char *a, const char *b) {
    return jd_concat3(a, b, NULL);
}

char *jd_strdup(const char *a) {
    return jd_concat3(a, NULL, NULL);
}

jd_frame_t *jd_dup_frame(const jd_frame_t *frame) {
    int sz = JD_FRAME_SIZE(frame);
    jd_frame_t *r = jd_alloc(sz);
    memcpy(r, frame, sz);
    return r;
}

void *jd_memdup(const void *src, unsigned size) {
    if (size == 0)
        return jd_alloc(1);
    void *r = jd_alloc(size);
    memcpy(r, src, size);
    return r;
}

bool jd_ends_with(const char *s, const char *suff) {
    if (s == NULL)
        return false;
    if (suff == NULL)
        return true;
    unsigned slen = strlen(s);
    unsigned sufflen = strlen(suff);
    return slen >= sufflen && strcmp(s + slen - sufflen, suff) == 0;
}

bool jd_starts_with(const char *s, const char *pref) {
    if (s == NULL)
        return false;
    if (pref == NULL)
        return true;
    unsigned preflen = strlen(pref);
    return memcmp(s, pref, preflen) == 0;
}

#endif
