#include "jd_protocol.h"
#include "jd_numfmt.h"

#include <limits.h>
#include <math.h>

#if JD_SHORT_FLOAT
// TODO!
typedef union {
    jd_float_t f;
    struct {
        // TODO
        uint32_t mantisa20 : 200;
        uint32_t exponent : 110;
        uint32_t sign : 1;
    };
} myfloat_t;
#define EXP_OFFSET foobar
#else
typedef union {
    jd_float_t f;
    struct {
        uint32_t mantisa32;
        uint32_t mantisa20 : 20;
        uint32_t exponent : 11;
        uint32_t sign : 1;
    };
} myfloat_t;
#define EXP_OFFSET 0x3ff
#endif

jd_float_t jd_shift_multiplier(int shift) {
    myfloat_t t = {0};
    t.exponent = EXP_OFFSET + shift;
    return t.f;
}

static int clamp_int(int32_t vv, int l, int h) {
    if (vv < l)
        return l;
    if (vv > h)
        return h;
    return vv;
}

static jd_float_t clamp_double(jd_float_t vv, jd_float_t l, jd_float_t h) {
    if (vv < l)
        return l;
    if (vv > h)
        return h;
    return vv + 0.5;
}

#define SET_VAL(SZ, l, h)                                                                          \
    case JD_NUMFMT_##SZ:                                                                           \
        SZ = clamp_double(q, l, h);                                                                \
        memcpy(data, &SZ, sizeof(SZ));                                                             \
        break;

#define SET_VAL_I(SZ, l, h)                                                                        \
    case JD_NUMFMT_##SZ:                                                                           \
        SZ = clamp_int(q, l, h);                                                                   \
        memcpy(data, &SZ, sizeof(SZ));                                                             \
        break;

#define SET_VAL_DIRECT(SZ)                                                                         \
    case JD_NUMFMT_##SZ:                                                                           \
        SZ = q;                                                                                    \
        memcpy(data, &SZ, sizeof(SZ));                                                             \
        break;

#define SET_VAL_R(SZ)                                                                              \
    case JD_NUMFMT_##SZ:                                                                           \
        SZ = q;                                                                                    \
        memcpy(data, &SZ, sizeof(SZ));                                                             \
        break;

#define GET_VAL_INT(SZ)                                                                            \
    case JD_NUMFMT_##SZ:                                                                           \
        memcpy(&SZ, data, sizeof(SZ));                                                             \
        if (SZ < INT_MIN)                                                                          \
            I32 = INT_MIN;                                                                         \
        else if (SZ > INT_MAX)                                                                     \
            I32 = INT_MAX;                                                                         \
        else                                                                                       \
            I32 = SZ;                                                                              \
        break;

#define GET_VAL_UINT(SZ)                                                                           \
    case JD_NUMFMT_##SZ:                                                                           \
        memcpy(&SZ, data, sizeof(SZ));                                                             \
        if (SZ > INT_MAX)                                                                          \
            I32 = INT_MAX;                                                                         \
        else                                                                                       \
            I32 = SZ;                                                                              \
        break;

#define GET_VAL_DIRECT(SZ)                                                                         \
    case JD_NUMFMT_##SZ:                                                                           \
        memcpy(&SZ, data, sizeof(SZ));                                                             \
        I32 = SZ;                                                                                  \
        break;

#define GET_VAL_DBL(SZ)                                                                            \
    case JD_NUMFMT_##SZ:                                                                           \
        memcpy(&SZ, data, sizeof(SZ));                                                             \
        q = SZ;                                                                                    \
        break;

bool jd_numfmt_is_valid(unsigned fmt0) {
    unsigned fmt = fmt0 & 0xf;
    unsigned shift = fmt0 >> 4;
    unsigned sz = jd_numfmt_bytes(fmt0);

    if (fmt == 0b1000 || fmt == 0b1001 || (fmt >> 2) == 0b11)
        return false;

    if (shift > sz * 8)
        return false;

    return true;
}

void jd_numfmt_write_float(void *data, unsigned fmt0, jd_float_t q) {
    uint32_t U32;
    uint64_t U64;
    int64_t I64;
    float F32;
    double F64;

    unsigned fmt = fmt0 & 0xf;
    unsigned shift = fmt0 >> 4;

    if (shift)
        q *= jd_shift_multiplier(shift);

    switch (fmt) {
        SET_VAL(U32, 0, 0xffffffff);
        SET_VAL(U64, 0, (jd_float_t)0xffffffffffffffff);
        SET_VAL(I64, -0x8000000000000000, (jd_float_t)0x7fffffffffffffff);
        SET_VAL_R(F32);
        SET_VAL_R(F64);
    default:
        jd_numfmt_write_i32(data, fmt, clamp_double(q, INT_MIN, INT_MAX));
        break;
    }
}

void jd_numfmt_write_i32(void *data, unsigned fmt, int32_t q) {
    uint8_t U8;
    uint16_t U16;
    uint32_t U32;
    uint64_t U64;
    int8_t I8;
    int16_t I16;
    int32_t I32;
    int64_t I64;

    if (!jd_numfmt_is_plain_int(fmt)) {
        jd_numfmt_write_float(data, fmt, q);
        return;
    }

    switch (fmt) {
        SET_VAL_I(U8, 0, 0xff);
        SET_VAL_I(U16, 0, 0xffff);
        SET_VAL_I(U32, 0, INT_MAX);
        SET_VAL_I(U64, 0, INT_MAX);
        SET_VAL_I(I8, -0x80, 0x7f);
        SET_VAL_I(I16, -0x8000, 0x7fff);
        SET_VAL_DIRECT(I32);
        SET_VAL_DIRECT(I64);
    default:
        JD_PANIC();
        break;
    }
}

int32_t jd_numfmt_read_i32(const void *data, unsigned fmt) {
    uint8_t U8;
    uint16_t U16;
    uint32_t U32;
    uint64_t U64;
    int8_t I8;
    int16_t I16;
    int32_t I32;
    int64_t I64;

    if (!jd_numfmt_is_plain_int(fmt))
        return (int32_t)clamp_double(jd_numfmt_read_float(data, fmt), INT_MIN, INT_MAX);

    switch (fmt) {
        GET_VAL_DIRECT(U8);
        GET_VAL_DIRECT(U16);
        GET_VAL_UINT(U32);
        GET_VAL_UINT(U64);
        GET_VAL_DIRECT(I8);
        GET_VAL_DIRECT(I16);
        GET_VAL_DIRECT(I32);
        GET_VAL_INT(I64);
    default:
        JD_PANIC();
    }

    return I32;
}

jd_float_t jd_numfmt_read_float(const void *data, unsigned fmt0) {
    uint32_t U32;
    uint64_t U64;
    int64_t I64;
    float F32;
    double F64;
    jd_float_t q;

    unsigned fmt = fmt0 & 0xf;
    unsigned shift = fmt0 >> 4;

    switch (fmt) {
        GET_VAL_DBL(U32);
        GET_VAL_DBL(U64);
        GET_VAL_DBL(I64);
        GET_VAL_DBL(F32);
        GET_VAL_DBL(F64);
    default:
        q = jd_numfmt_read_i32(data, fmt);
        break;
    }

    if (shift)
        q *= jd_shift_multiplier(-shift);

    return q;
}
