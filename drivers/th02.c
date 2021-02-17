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
    env_reading_t humidity;
    env_reading_t temperature;
    uint32_t nextsample;
} ctx_t;
static ctx_t state;

static const int32_t humidity_error[] = {ERR_HUM(0, 3), ERR_HUM(80, 3), ERR_HUM(100, 5.5), ERR_END};

static const int32_t temperature_error[] = {ERR_TEMP(-40, 3), ERR_TEMP(0, 0.5), ERR_TEMP(70, 0.5),
                                            ERR_TEMP(125, 1.2), ERR_END};

static int read_data(void) {
    uint8_t data[3];
    if (i2c_read_reg_buf(TH02_ADDR, TH02_STATUS, data, 3) < 0)
        return -1;
    if (data[0] & 1) {
        // DMESG("miss");
        return -1;
    }
    return (data[1] << 8) | data[2];
}

static int weather_hw_process(ctx_t *ctx) {
    if (!ctx->inited) {
        ctx->inited = 1;
        i2c_init();
        int id = i2c_read_reg(TH02_ADDR, TH02_ID);
        DMESG("TH02 id=%x", id);
        if (id < 0)
            hw_panic();
        ctx->temperature.min_value = SCALE_TEMP(-40);
        ctx->temperature.max_value = SCALE_TEMP(70);
        ctx->humidity.min_value = SCALE_TEMP(0);
        ctx->humidity.max_value = SCALE_TEMP(100);
    }

    // the 50ms here is just for readings, we actually sample at SAMPLING_MS
    if (jd_should_sample(&ctx->nextsample, 50000)) {
        if (ctx->in_temp) {
            int v = read_data();
            if (v >= 0) {
                ctx->in_temp = 0;
                env_set_value(&ctx->temperature, ((v << PRECISION) >> 7) - (50 << PRECISION),
                              temperature_error);
                ctx->in_humidity = 1;
                i2c_write_reg(TH02_ADDR, TH02_CONFIG, TH02_CFG_START);
            }
        } else if (ctx->in_humidity) {
            int v = read_data();
            if (v >= 0) {
                ctx->in_humidity = 0;
                env_set_value(&ctx->humidity, ((v << PRECISION) >> 8) - (24 << PRECISION),
                              humidity_error);
                ctx->nextsample = now + SAMPLING_MS * 1000;
                // DMESG("t=%dC h=%d%%", ctx->temp >> PRECISION, ctx->humidity >> PRECISION);
                ctx->inited = 2;
            }
        } else {
            ctx->in_temp = 1;
            i2c_write_reg(TH02_ADDR, TH02_CONFIG, TH02_CFG_START | TH02_CFG_TEMP);
        }
    }

    return ctx->inited >= 2;
}

const env_reading_t *env_temperature(void) {
    ctx_t *ctx = &state;
    if (weather_hw_process(ctx))
        return &ctx->temperature;
    return NULL;
}

const env_reading_t *env_humidity(void) {
    ctx_t *ctx = &state;
    if (weather_hw_process(ctx))
        return &ctx->humidity;
    return NULL;
}

#endif // TEMP_TH02