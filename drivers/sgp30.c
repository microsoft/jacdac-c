#include "jd_drivers.h"

// doesn't seem to be working - I keep getting 0/0xffff reading from it
#define SAMPLING_MS 2000

#ifndef SGP30_ADDR
#define SGP30_ADDR 0x58
#endif

#define sgp30_tvoc_init_continuous 0x2003
#define sgp30_set_absolute_humidity 0x2061
#define sgp30_get_feature_set_version 0x202f
#define sgp30_measure_air_quality 0x2008

#define ST_IDLE 0
#define ST_HUM_NEEDED 1
#define ST_HUM_ISSUED 2
#define ST_READ_ISSUED 3

#define WORD(off) ((data[off] << 8) | data[(off) + 1])

typedef struct state {
    uint8_t inited;
    uint8_t state;
    uint8_t hum_comp[3];
    env_reading_t eco2;
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

int jd_sgp_read_u16(uint8_t dev_addr, uint16_t regaddr, unsigned wait) {
    uint8_t buf[3];
    int r;
    if (wait == 0)
        r = i2c_read_reg16_buf(dev_addr, regaddr, buf, sizeof(buf));
    else {
        r = i2c_write_reg16_buf(dev_addr, regaddr, NULL, 0);
        if (r == 0) {
            target_wait_us(wait);
            r = i2c_read_ex(dev_addr, buf, sizeof(buf));
        }
    }
    if (r == 0 && jd_sgp_crc8(buf, 2) == buf[2]) {
        return (buf[0] << 8) | (buf[1]);
    }
    return -1;
}

static void send_cmd(uint16_t cmd) {
    if (i2c_write_reg16_buf(SGP30_ADDR, cmd, NULL, 0))
        JD_PANIC();
}

static bool sgp30_is_present(void) {
    i2c_init();
    return (jd_sgp_read_u16(SGP30_ADDR, sgp30_get_feature_set_version, 2000) & 0xff00) == 0x0000;
}

static void sgp30_init(void) {
    ctx_t *ctx = &state;
    if (ctx->inited)
        return;

    ctx->eco2.min_value = 400 << 10;
    ctx->eco2.max_value = 60000 << 10;
    ctx->tvoc.min_value = 0 << 10;
    ctx->tvoc.max_value = 60000 << 10;

    ctx->inited = 1;
    i2c_init();
    // DMESG("SGP30 pres=%x", sgp30_is_present());

    int id = jd_sgp_read_u16(SGP30_ADDR, sgp30_get_feature_set_version, 2000);
    if (id < 0)
        JD_PANIC();

    DMESG("SGP30 id=%x", id);

    ctx->nextsample = now + (1 << 20);
    send_cmd(sgp30_tvoc_init_continuous);
}

static void sgp30_process(void) {
    ctx_t *ctx = &state;

    if (jd_should_sample_delay(&ctx->nextsample, 50 * 1000)) {
        switch (ctx->state) {
        case ST_HUM_NEEDED:
            // don't set it before fully initialized
            if (ctx->inited >= 3)
                if (i2c_write_reg16_buf(SGP30_ADDR, sgp30_set_absolute_humidity, ctx->hum_comp, 3))
                    JD_PANIC();
            ctx->state = ST_HUM_ISSUED;
            break;

        case ST_IDLE:
        case ST_HUM_ISSUED:
            send_cmd(sgp30_measure_air_quality);
            ctx->state = ST_READ_ISSUED;
            break;

        case ST_READ_ISSUED: {
            uint8_t data[6];
            if (i2c_read_ex(SGP30_ADDR, data, sizeof(data)))
                JD_PANIC();
            ctx->state = ST_IDLE;

            if (ctx->inited == 1) {
                // give it 2s to start up
                ctx->nextsample = now + (2 << 20);
                ctx->inited = 2;
            } else {
                ctx->nextsample = now + SAMPLING_MS * 1000;
                ctx->inited = 3;
            }

            int eco2 = WORD(0);
            int tvoc = WORD(3);

            // assume 15% error
            ctx->eco2.value = eco2 << 10;
            ctx->eco2.error = eco2 * 153;
            ctx->tvoc.value = tvoc << 10;
            ctx->tvoc.error = tvoc * 153;

            break;
        }

        default:
            JD_PANIC();
        }
    }
}

static void sgp30_sleep(void) {
    // this is "general call" to register 0x06
    i2c_write_reg_buf(0x00, 0x06, NULL, 0);
}

static uint32_t sgp30_conditioning_period(void) {
    return 15;
}

static void sgp30_set_temp_humidity(int32_t temp, int32_t humidity) {
    ctx_t *ctx = &state;
    uint16_t scaled = env_absolute_humidity(temp, humidity) >> 2;
    ctx->hum_comp[0] = scaled >> 8;
    ctx->hum_comp[1] = scaled & 0xff;
    ctx->hum_comp[2] = jd_sgp_crc8(ctx->hum_comp, 2);
    if (ctx->state == ST_IDLE)
        ctx->state = ST_HUM_NEEDED;
}

ENV_INIT_DUAL(sgp30_init, sgp30_sleep);

ENV_DEFINE_GETTER(sgp30, eco2)
ENV_DEFINE_GETTER(sgp30, tvoc)

#define FUNS(val, idx)                                                                             \
    ENV_GETTER(sgp30, val, idx), .conditioning_period = sgp30_conditioning_period,                 \
                                 .set_temp_humidity = sgp30_set_temp_humidity,                     \
                                 .is_present = sgp30_is_present

const env_sensor_api_t tvoc_sgp30 = {FUNS(tvoc, 0)};
const env_sensor_api_t eco2_sgp30 = {FUNS(eco2, 1)};
