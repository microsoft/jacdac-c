#include "jd_drivers.h"

// doesn't seem to be working - I keep getting 0/0xffff reading from it
#define SAMPLING_MS 2000
#define PRECISION 10

#ifndef SGPC3_ADDR
#define SGPC3_ADDR 0x58
#endif

#define sgpc3_tvoc_init_continuous 0x20ae
#define sgpc3_tvoc_init_0 0x2089
#define sgpc3_set_absolute_humidity 0x2061
#define sgpc3_get_feature_set_version 0x202f
#define sgpc3_measure_tvoc_and_raw 0x2046

#define ST_IDLE 0
#define ST_HUM_NEEDED 1
#define ST_HUM_ISSUED 2
#define ST_READ_ISSUED 3

typedef struct state {
    uint8_t inited;
    uint8_t state;
    uint8_t hum_comp[3];
    env_reading_t ethanol;
    env_reading_t tvoc;
    uint32_t nextsample;
} ctx_t;
static ctx_t state;

uint8_t jd_sgp_crc8(const uint8_t *data, int len) {
    uint8_t res = 0xff;
    while (len--) {
        res ^= *data++;
        for (int i = 0; i < 8; ++i)
            if (res & 0x80)
                res = (res << 1) ^ 0x31;
            else
                res = (res << 1);
    }
    return res;
}

static void send_cmd(uint16_t cmd) {
    if (i2c_write_reg16_buf(SGPC3_ADDR, cmd, NULL, 0))
        hw_panic();
}

static void sgpc3_init(void) {
    ctx_t *ctx = &state;
    if (ctx->inited)
        return;

    ctx->ethanol.min_value = SCALE_TVOC(0);
    ctx->ethanol.max_value = SCALE_TVOC(1000); // typical max is 30
    ctx->tvoc.min_value = SCALE_TVOC(0);
    ctx->tvoc.max_value = SCALE_TVOC(60000);

    ctx->inited = 1;
    i2c_init();

    send_cmd(sgpc3_get_feature_set_version);
    target_wait_us(10000);

    uint8_t data[2];
    if (i2c_read_ex(SGPC3_ADDR, data, sizeof(data)))
        hw_panic();
    DMESG("SGPC3 id=%x %x", data[0], data[1]);

    // ctx->nextsample = now + 16 * 1000 * 1000;
    ctx->nextsample = now + 184 * 1000 * 1000;
    send_cmd(sgpc3_tvoc_init_continuous);
    // send_cmd(sgpc3_tvoc_init_0);
}

static void sgpc3_process(void) {
    ctx_t *ctx = &state;

    if (jd_should_sample_delay(&ctx->nextsample, 50 * 1000)) {
        switch (ctx->state) {
        case ST_HUM_NEEDED:
            // don't set it before fully initialized
            if (ctx->inited >= 3)
                if (i2c_write_reg16_buf(SGPC3_ADDR, sgpc3_set_absolute_humidity, ctx->hum_comp, 3))
                    hw_panic();
            ctx->state = ST_HUM_ISSUED;
            break;

        case ST_IDLE:
        case ST_HUM_ISSUED:
            send_cmd(sgpc3_measure_tvoc_and_raw);
            ctx->state = ST_READ_ISSUED;
            break;

        case ST_READ_ISSUED: {
            uint8_t data[6];
            if (i2c_read_ex(SGPC3_ADDR, data, sizeof(data)))
                hw_panic();
            DMESG("rd: %x %x %x %x %x %x", data[0], data[1], data[2], data[3], data[4], data[5]);
            ctx->state = ST_IDLE;

            if (ctx->inited == 1) {
                ctx->nextsample = now + 20 * 1000 * 1000;
                ctx->inited = 2;
            } else {
                ctx->nextsample = now + SAMPLING_MS * 1000;
                ctx->inited = 3;
            }

            int tvoc = (data[0] << 8) | data[1];
            int ethanol = (data[3] << 8) | data[4];

            DMESG("TV %d %d", tvoc, ethanol);

            // assume 15% error
            ctx->ethanol.value = ethanol << 10;
            ctx->ethanol.error = ethanol * 153;
            ctx->tvoc.value = tvoc << 10;
            ctx->tvoc.error = tvoc * 153;

            break;
        }

        default:
            hw_panic();
        }
    }
}

static void *sgpc3_tvoc(void) {
    ctx_t *ctx = &state;
    if (ctx->inited >= 3)
        return &ctx->tvoc;
    return NULL;
}

static void *sgpc3_ethanol(void) {
    ctx_t *ctx = &state;
    if (ctx->inited >= 3)
        return &ctx->ethanol;
    return NULL;
}

static uint32_t sgpc3_conditioning_period(void) {
    // none when sensor off for less than 5 min
    // 16s when sensor off for 5 min - 1h
    // 64s when sensor off for 1-24h
    // 184s when longer
    return 184 + 20;
}

static void sgpc3_set_temp_humidity(int32_t temp, int32_t humidity) {
    ctx_t *ctx = &state;
    uint16_t scaled = env_absolute_humidity(temp, humidity) >> 2;
    ctx->hum_comp[0] = scaled >> 8;
    ctx->hum_comp[1] = scaled & 0xff;
    ctx->hum_comp[2] = jd_sgp_crc8(ctx->hum_comp, 2);
    if (ctx->state == ST_IDLE)
        ctx->state = ST_HUM_NEEDED;
}

const env_sensor_api_t tvoc_sgpc3 = {
    .init = sgpc3_init,
    .process = sgpc3_process,
    .get_reading = sgpc3_tvoc,
    .conditioning_period = sgpc3_conditioning_period,
    .set_temp_humidity = sgpc3_set_temp_humidity,
};

const env_sensor_api_t ethanol_sgpc3 = {
    .init = sgpc3_init,
    .process = sgpc3_process,
    .get_reading = sgpc3_ethanol,
    .conditioning_period = sgpc3_conditioning_period,
    .set_temp_humidity = sgpc3_set_temp_humidity,
};
