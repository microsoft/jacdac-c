#include "jd_drivers.h"
#include "jd_util.h"

#ifdef TEMP_SHTC3

#define SAMPLING_MS 500
#define PRECISION 10

#ifndef SHTC3_ADDR
#define SHTC3_ADDR 0x70
#endif

#define SHTC3_MEASURE_NORMAL 0x7866
#define SHTC3_MEASURE_LOW_POWER 0x609C

#define SHTC3_SOFTRESET 0x805D
#define SHTC3_ID 0xEFC8
#define SHTC3_SLEEP 0xB098
#define SHTC3_WAKEUP 0x3517

typedef struct state {
    uint8_t inited;
    uint8_t read_issued;
    env_reading_t humidity;
    env_reading_t temperature;
    uint32_t nextsample;
} ctx_t;
static ctx_t state;

static const int32_t humidity_error[] = {ERR_HUM(0, 3.5), ERR_HUM(20, 2), ERR_HUM(80, 2),
                                         ERR_HUM(100, 3.5), ERR_END};

static const int32_t temperature_error[] = {ERR_TEMP(-40, 0.8), ERR_TEMP(5, 0.2), ERR_TEMP(60, 0.2),
                                            ERR_TEMP(125, 0.8), ERR_END};

static void send_cmd(uint16_t cmd) {
    if (i2c_write_reg16_buf(SHTC3_ADDR, cmd, NULL, 0))
        hw_panic();
}

static void wake(void) {
    send_cmd(SHTC3_WAKEUP);
    target_wait_us(300); // 200us seems minimum; play it safe with 300us
}

static void init(ctx_t *ctx) {
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
    int id = i2c_read_reg16(SHTC3_ADDR, SHTC3_ID);
    DMESG("SHTC3 id=%x", id);
    if (id <= 0)
        hw_panic();
    send_cmd(SHTC3_SLEEP);
}

static int weather_hw_process(ctx_t *ctx) {
    init(ctx);

    // the 20ms here is just for readings, we actually sample at SAMPLING_MS
    // the datasheet says max reading time is 12.1ms; give a little more time
    if (jd_should_sample(&ctx->nextsample, 20000)) {
        if (!ctx->read_issued) {
            ctx->read_issued = 1;
            wake();
            send_cmd(SHTC3_MEASURE_NORMAL);
        } else {
            uint8_t data[6];
            if (i2c_read_ex(SHTC3_ADDR, data, sizeof(data)))
                hw_panic();
            uint16_t temp = (data[0] << 8) | data[1];
            uint16_t hum = (data[3] << 8) | data[4];
            send_cmd(SHTC3_SLEEP);
            ctx->read_issued = 0;
            ctx->nextsample = now + SAMPLING_MS * 1000;
            env_set_value(&ctx->humidity, 100 * hum >> 6, humidity_error);
            env_set_value(&ctx->temperature, (175 * temp >> 6) - (45 << 10), temperature_error);
            ctx->inited = 2;
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

#endif // TEMP_SHTC3