#include "jd_drivers.h"

#define SAMPLING_MS 300
#define PRECISION 10

#ifndef LTR390UV_ADDR
#define LTR390UV_ADDR 0x53
#endif

#define STATE_START 0
#define STATE_AMBIENT_ISSUED 1
#define STATE_UVI_ISSUED 2

typedef struct state {
    uint8_t inited;
    uint8_t state;
    uint8_t gain;
    env_reading_t uvi;
    env_reading_t ambient;
    uint32_t nextsample;
} ctx_t;
static ctx_t state;

#define LTR390UV_MAIN_CTRL 0x00
#define LTR390UV_MEAS_RATE 0x04
#define LTR390UV_GAIN 0x05
#define LTR390UV_PART_ID 0x06
#define LTR390UV_MAIN_STATUS 0x07
#define LTR390UV_ALSDATA 0x0D
#define LTR390UV_UVSDATA 0x10

static bool ltr390uv_is_present(void) {
    return (i2c_read_reg(LTR390UV_ADDR, LTR390UV_PART_ID) >> 4) == 0xB;
}

static void ltr390uv_init(void) {
    ctx_t *ctx = &state;
    if (ctx->inited)
        return;

    // TODO:
    ctx->uvi.min_value = SCALE_UVI(0);
    ctx->uvi.max_value = SCALE_UVI(20);
    ctx->ambient.min_value = SCALE_LUX(0);
    ctx->ambient.max_value = SCALE_LUX(100000);

    ctx->nextsample = now;

    ctx->inited = 1;
    i2c_init();

    DMESG("pres %d", ltr390uv_is_present());

    int id = i2c_read_reg(LTR390UV_ADDR, LTR390UV_PART_ID);
    DMESG("LTR390UV part=%x", id);
    if ((id >> 4) != 0xB)
        JD_PANIC();
    i2c_write_reg(LTR390UV_ADDR, LTR390UV_MEAS_RATE, 0x22); // 18bit/100ms

    ctx->gain = 3;
    i2c_write_reg(LTR390UV_ADDR, LTR390UV_GAIN, 0x01);
}

static bool data_ready(void) {
    int v = i2c_read_reg(LTR390UV_ADDR, LTR390UV_MAIN_STATUS);
    return (v & 0x08) != 0;
}

static uint32_t read_data(int addr) {
    uint8_t data[3];
    if (i2c_read_reg_buf(LTR390UV_ADDR, addr, data, 3))
        JD_PANIC();
    return data[0] | (data[1] << 8) | ((data[2] & 0xf) << 16);
}

static void ltr390uv_process(void) {
    ctx_t *ctx = &state;

    // the 9ms here is just for readings, we actually sample at SAMPLING_MS
    if (!jd_should_sample_delay(&ctx->nextsample, 9000))
        return;

    switch (ctx->state) {
    case STATE_START:
        i2c_write_reg(LTR390UV_ADDR, LTR390UV_MAIN_CTRL, 0x02); // ALS Mode, Enabled
        ctx->state = STATE_AMBIENT_ISSUED;
        break;
    case STATE_AMBIENT_ISSUED:
        if (data_ready()) {
            uint32_t v = read_data(LTR390UV_ALSDATA);
            ctx->ambient.value = (((v * 6) << 8) / (10 * ctx->gain)) << 2;
            ctx->ambient.error = ctx->ambient.value / 10;           // 10% per datasheet
            i2c_write_reg(LTR390UV_ADDR, LTR390UV_MAIN_CTRL, 0x0A); // UV Mode, Enabled
            ctx->state = STATE_UVI_ISSUED;
        }
        break;
    case STATE_UVI_ISSUED:
        if (data_ready()) {
            uint32_t v = read_data(LTR390UV_UVSDATA);
            ctx->uvi.value = (v << 11) / ctx->gain;
            ctx->uvi.error = ctx->uvi.value / 10;                   // assume same error as ALS
            i2c_write_reg(LTR390UV_ADDR, LTR390UV_MAIN_CTRL, 0x00); // Disabled
            ctx->state = STATE_START;
            ctx->inited = 2; // have both values
            ctx->nextsample = now + SAMPLING_MS * 1000;
        }
        break;
    }
}

static void *ltr390uv_ambient(void) {
    ctx_t *ctx = &state;
    if (ctx->inited >= 2)
        return &ctx->ambient;
    return NULL;
}

static void *ltr390uv_uvi(void) {
    ctx_t *ctx = &state;
    if (ctx->inited >= 2)
        return &ctx->uvi;
    return NULL;
}

const env_sensor_api_t illuminance_ltr390uv = {
    .init = ltr390uv_init,
    .process = ltr390uv_process,
    .get_reading = ltr390uv_ambient,
    .is_present = ltr390uv_is_present,
};

const env_sensor_api_t uvindex_ltr390uv = {
    .init = ltr390uv_init,
    .process = ltr390uv_process,
    .get_reading = ltr390uv_uvi,
    .is_present = ltr390uv_is_present,
};
