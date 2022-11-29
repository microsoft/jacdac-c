#include "jd_drivers.h"

// TODO this doesn't seem to work

#define SAMPLING_MS 500
#define PRECISION 10

#ifndef CPS122_ADDR
#define CPS122_ADDR 0x6D
#endif

#define CPS122_CTRL 0x30
#define CPS122_MR 0x0A
#define CPS122_GD 0x06

typedef struct state {
    uint8_t inited;
    uint8_t read_issued;
    env_reading_t pressure;
    env_reading_t temperature;
    uint32_t nextsample;
} ctx_t;
static ctx_t state;

static const int32_t pressure_error[] = {ERR_PRESSURE(300, 1), ERR_PRESSURE(1200, 1), ERR_END};

static const int32_t temperature_error[] = {ERR_TEMP(-40, 1.0), ERR_TEMP(85, 1.0), ERR_END};

static void send_cmd(uint16_t cmd) {
    if (i2c_write_reg_buf(CPS122_ADDR, cmd, NULL, 0))
        JD_PANIC();
}

static void cps122_init(void) {
    ctx_t *ctx = &state;
    if (ctx->inited)
        return;

    ctx->pressure.min_value = SCALE_PRESSURE(300);
    ctx->pressure.max_value = SCALE_PRESSURE(1200);
    ctx->temperature.min_value = SCALE_TEMP(-40);
    ctx->temperature.max_value = SCALE_TEMP(85);

    ctx->nextsample = now;

    ctx->inited = 1;
    i2c_init();
#if 0
    uint8_t buf[5];
    int id = i2c_read_reg_buf(CPS122_ADDR, CPS122_GD, buf, sizeof(buf));
    if (id < 0)
        JD_PANIC();
#endif
}

static void cps122_process(void) {
    ctx_t *ctx = &state;

    // the 10ms here is just for readings, we actually sample at SAMPLING_MS
    // the datasheet says max reading time is ~5ms; give a little more time
    if (jd_should_sample_delay(&ctx->nextsample, 10000)) {
        if (!ctx->read_issued) {
            ctx->read_issued = 1;
            send_cmd(CPS122_MR);
        } else {
            uint8_t data[5];
            int r = i2c_read_reg_buf(CPS122_ADDR, CPS122_GD, data, sizeof(data));
            if (r < 0)
                JD_PANIC();

            int16_t rtemp = (data[3] << 8) | data[4];
            int32_t temp = rtemp << 2;

            uint32_t pressure = (data[0] << 16) | (data[1] << 8) | data[2];
            pressure = (pressure * 4) / 25;

            ctx->read_issued = 0;
            ctx->nextsample = now + SAMPLING_MS * 1000;

            env_set_value(&ctx->pressure, pressure, pressure_error);
            env_set_value(&ctx->temperature, temp, temperature_error);
            ctx->inited = 2;
        }
    }
}

static void *cps122_temperature(void) {
    ctx_t *ctx = &state;
    if (ctx->inited >= 2)
        return &ctx->temperature;
    return NULL;
}

static void *cps122_pressure(void) {
    ctx_t *ctx = &state;
    if (ctx->inited >= 2)
        return &ctx->pressure;
    return NULL;
}

const env_sensor_api_t temperature_cps122 = {
    .init = cps122_init,
    .process = cps122_process,
    .get_reading = cps122_temperature,
};

const env_sensor_api_t pressure_cps122 = {
    .init = cps122_init,
    .process = cps122_process,
    .get_reading = cps122_pressure,
};
