#include "jd_drivers.h"
#include "jd_util.h"

#ifdef TEMP_TH02

#define SAMPLING_MS 500
#define PRECISION 10

#ifndef TH02_ADDR
#define TH02_ADDR 0x40
#endif

#define TH02_CFG_FAST (1 << 5)
#define TH02_CFG_TEMP (1 << 4)
#define TH02_CFG_HEAT (1 << 1)
#define TH02_CFG_START (1 << 0)

#define TH02_STATUS 0x00
#define TH02_DATA_H 0x01
#define TH02_DATA_L 0x02
#define TH02_CONFIG 0x03
#define TH02_ID 0x11

typedef struct state {
    uint8_t inited;
    uint8_t in_temp;
    uint8_t in_humidity;
    uint32_t humidity;
    uint32_t temp;
    uint32_t nextsample;
} ctx_t;
static ctx_t state;

static int read_data(void) {
    uint8_t data[3];
    if (i2c_read_reg_buf(TH02_ADDR, TH02_STATUS, data, 3) < 0)
        return -1;
    if (data[0] & 1) {
        //DMESG("miss");
        return -1;
    }
    return (data[1] << 8) | data[2];
}

static void weather_hw_process(void) {
    ctx_t *ctx = &state;
    if (!ctx->inited) {
        ctx->inited = 1;
        i2c_init();
        int id = i2c_read_reg(TH02_ADDR, TH02_ID);
        DMESG("TH02 id=%x", id);
        if (id < 0)
            hw_panic();
    }

    // the 50ms here is just for readings, we actually sample at SAMPLING_MS
    if (jd_should_sample(&ctx->nextsample, 50000)) {
        if (ctx->in_temp) {
            int v = read_data();
            if (v >= 0) {
                ctx->in_temp = 0;
                ctx->temp = ((v << PRECISION) >> 7) - (50 << PRECISION);
                ctx->in_humidity = 1;
                i2c_write_reg(TH02_ADDR, TH02_CONFIG, TH02_CFG_START);
            }
        } else if (ctx->in_humidity) {
            int v = read_data();
            if (v >= 0) {
                ctx->in_humidity = 0;
                ctx->humidity = ((v << PRECISION) >> 8) - (24 << PRECISION);
                ctx->nextsample = now + SAMPLING_MS * 1000;
                //DMESG("t=%dC h=%d%%", ctx->temp >> PRECISION, ctx->humidity >> PRECISION);
            }
        } else {
            ctx->in_temp = 1;
            i2c_write_reg(TH02_ADDR, TH02_CONFIG, TH02_CFG_START | TH02_CFG_TEMP);
        }
    }
}

uint32_t hw_temp(void) {
    weather_hw_process();
    return state.temp;
}

uint32_t hw_humidity(void) {
    weather_hw_process();
    return state.humidity;
}

#endif // TEMP_TH02