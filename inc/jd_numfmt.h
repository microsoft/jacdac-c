// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef JD_NUMFMT_H
#define JD_NUMFMT_H

#if JD_SHORT_FLOAT
typedef float jd_float_t;
#else
typedef double jd_float_t;
#endif

#define JD_NUMFMT_U8 0b0000
#define JD_NUMFMT_U16 0b0001
#define JD_NUMFMT_U32 0b0010
#define JD_NUMFMT_U64 0b0011
#define JD_NUMFMT_I8 0b0100
#define JD_NUMFMT_I16 0b0101
#define JD_NUMFMT_I32 0b0110
#define JD_NUMFMT_I64 0b0111
#define JD_NUMFMT_F8 0b1000  // not supported
#define JD_NUMFMT_F16 0b1001 // not supported
#define JD_NUMFMT_F32 0b1010
#define JD_NUMFMT_F64 0b1011

#define JD_NUMFMT(fmt, shift) (JACS_NUMFMT_##fmt | ((shift) << 4))

static inline int jd_numfmt_bytes(unsigned numfmt) {
    return (1 << (numfmt & 0b11));
}

static inline int jd_numfmt_is_plain_int(unsigned numfmt) {
    return (numfmt >> 3) == 0;
}

bool jd_numfmt_is_valid(unsigned fmt0);

jd_float_t jd_numfmt_read_float(const void *src, unsigned numfmt);
// this is optimized in some cases, otherwise falls back to jd_numfmt_read_float()
int32_t jd_numfmt_read_i32(const void *src, unsigned numfmt);

void jd_numfmt_write_float(void *dst, unsigned numfmt, jd_float_t v);
void jd_numfmt_write_i32(void *dst, unsigned numfmt, int32_t v);

// jd_shift_multiplier(10) = 1024
// jd_shift_multiplier(0) = 1
// jd_shift_multiplier(-10) = 1/1024
jd_float_t jd_shift_multiplier(int shift);

#endif // JD_NUMFMT_H