#include "jd_drivers.h"

#define SAMPLING_MS 500
#define PRECISION 10

#ifndef SHT30_ADDR
#define SHT30_ADDR 0x44
#endif

#define SHT30_MEASURE_HIGH_REP 0x2400 // no clock stretching
#define SHT30_SOFTRESET 0x30A2
#define SHT30_STATUS 0xF32D

typedef struct state {
    uint8_t inited;
    uint8_t read_issued;
    env_reading_t humidity;
    env_reading_t temperature;
    uint32_t nextsample;
} ctx_t;
static ctx_t state;

static const int32_t humidity_error[] = {ERR_HUM(0, 4), ERR_HUM(20, 2), ERR_HUM(80, 2),
                                         ERR_HUM(100, 4), ERR_END};

static const int32_t temperature_error[] = {ERR_TEMP(-40, 0.6), ERR_TEMP(0, 0.2), ERR_TEMP(65, 0.2),
                                            ERR_TEMP(125, 0.6), ERR_END};

static void send_cmd(uint16_t cmd) {
    if (i2c_write_reg16_buf(SHT30_ADDR, cmd, NULL, 0))
        hw_panic();
}

static void wake(void) {
    // nothing to do on this chip
}

static void sht30_init(void) {
    ctx_t *ctx = &state;
    if (ctx->inited)
        return;

    ctx->humidity.min_value = SCALE_HUM(0);
    ctx->humidity.max_value = SCALE_HUM(100);
    ctx->temperature.min_value = SCALE_TEMP(-40);
    ctx->temperature.max_value = SCALE_TEMP(125);

    ctx->nextsample = now;

    ctx->inited = 1;
    i2c_init();
    wake();
    int id = i2c_read_reg16(SHT30_ADDR, SHT30_STATUS);
    DMESG("SHT30 status=%x", id);
    if (id < 0)
        hw_panic();
}

static void sht30_process(void) {
    ctx_t *ctx = &state;

    // the 20ms here is just for readings, we actually sample at SAMPLING_MS
    // the datasheet says max reading time is 15.5ms; give a little more time
    if (jd_should_sample_delay(&ctx->nextsample, 20000)) {
        if (!ctx->read_issued) {
            ctx->read_issued = 1;
            wake();
            send_cmd(SHT30_MEASURE_HIGH_REP);
        } else {
            uint8_t data[6];
            if (i2c_read_ex(SHT30_ADDR, data, sizeof(data)))
                hw_panic();
            uint16_t temp = (data[0] << 8) | data[1];
            uint16_t hum = (data[3] << 8) | data[4];
            ctx->read_issued = 0;
            ctx->nextsample = now + SAMPLING_MS * 1000;
            env_set_value(&ctx->humidity, 100 * hum >> 6, humidity_error);
            env_set_value(&ctx->temperature, (175 * temp >> 6) - (45 << 10), temperature_error);
            ctx->inited = 2;
        }
    }
}

static void *sht30_temperature(void) {
    ctx_t *ctx = &state;
    if (ctx->inited >= 2)
        return &ctx->temperature;
    return NULL;
}

static void *sht30_humidity(void) {
    ctx_t *ctx = &state;
    if (ctx->inited >= 2)
        return &ctx->humidity;
    return NULL;
}

const env_sensor_api_t temperature_sht30 = {
    .init = sht30_init,
    .process = sht30_process,
    .get_reading = sht30_temperature,
};

const env_sensor_api_t humidity_sht30 = {
    .init = sht30_init,
    .process = sht30_process,
    .get_reading = sht30_humidity,
};
