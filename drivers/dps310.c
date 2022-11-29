#include "jd_drivers.h"
#include <math.h>

#define SAMPLING_MS 250

#ifndef DPS310_ADDR
#define DPS310_ADDR 0x76
#endif

#define DPS310_PRS_B2 0x00
#define DPS310_TMP_B2 0x03
#define DPS310_PRS_CFG 0x06
#define DPS310_TMP_CFG 0x07
#define DPS310_MEAS_CFG 0x08
#define DPS310_CFG_REG 0x09
#define DPS310_RESET 0x0C
#define DPS310_PROD_ID 0x0D
#define DPS310_TMP_COEF_SRCE 0x28

// both support up to 7

#define RATE_BITS 2
#define RATE_PER_SECOND (1 << RATE_BITS)

static uint8_t dev_addr = DPS310_ADDR;

static const uint32_t scale_factor[] = {
    0x80000, 0x180000, 0x380000, 0x780000, 0x3e000, 0x7e000, 0xfe000, 0x1fe000,
};

#define OVERSAMPLE_BITS 3
#define OVERSAMPLE_VAL (1 << OVERSAMPLE_BITS)

typedef struct state {
    uint8_t inited;
    uint8_t read_issued;
    int c0;
    int c1;
    int c00;
    int c10;
    int c01;
    int c11;
    int c20;
    int c21;
    int c30;
    float raw_temp;
    env_reading_t pressure;
    env_reading_t temperature;
    uint32_t nextsample;
} ctx_t;
static ctx_t state;

static const int32_t pressure_error[] = {ERR_PRESSURE(300, 1), ERR_PRESSURE(1200, 1), ERR_END};

static const int32_t temperature_error[] = {ERR_TEMP(-40, 0.5), ERR_TEMP(85, 0.5), ERR_END};

static int twos_complement(int val, int bits) {
    if (val & (1 << (bits - 1)))
        return val - (1 << bits);
    else
        return val;
}
static void write_reg(uint8_t addr, uint8_t v) {
    i2c_write_reg(dev_addr, addr, v);
}
static uint8_t read_reg(uint8_t addr) {
    return i2c_read_reg(dev_addr, addr);
}
static int read24(uint8_t addr) {
    uint8_t tmp[3];
    i2c_read_reg_buf(dev_addr, addr, tmp, sizeof(tmp));
    int v = tmp[2] | (tmp[1] << 8) | (tmp[0] << 16);
    return twos_complement(v, 24);
}
static float read_scaled(uint8_t addr) {
    return read24(addr) / (float)scale_factor[OVERSAMPLE_BITS];
}
static bool meas_cfg_has_bit(int bit) {
    return (read_reg(DPS310_MEAS_CFG) & (1 << bit)) != 0;
}
static void wait_meas_cfg_bit(int bit) {
    while (!meas_cfg_has_bit(bit))
        target_wait_us(1000);
}

static bool dps310_is_present(void) {
    for (int i = 0x76; i <= 0x77; ++i) {
        int v = i2c_read_reg(i, DPS310_PROD_ID);
        if (v == 0x10) {
            dev_addr = i;
            return 1;
        }
    }
    return 0;
}

static void dps310_init(void) {
    ctx_t *ctx = &state;
    if (ctx->inited)
        return;

    ctx->pressure.min_value = SCALE_PRESSURE(300);
    ctx->pressure.max_value = SCALE_PRESSURE(1200);
    ctx->temperature.min_value = SCALE_TEMP(-40);
    ctx->temperature.max_value = SCALE_TEMP(85);

    ctx->nextsample = now;
    ctx->raw_temp = NAN;

    ctx->inited = 1;
    i2c_init();

    if (i2c_read_reg(dev_addr, DPS310_PROD_ID) != 0x10)
        JD_PANIC();

    write_reg(DPS310_RESET, 0x89);
    target_wait_us(10000);

    wait_meas_cfg_bit(6); // SENSOR_RDY

    //    https://github.com/Infineon/DPS310-Pressure-Sensor#temperature-measurement-issue
    write_reg(0x0e, 0xa5);
    write_reg(0x0f, 0x96);
    write_reg(0x62, 0x02);
    write_reg(0x0e, 0x00);
    write_reg(0x0f, 0x00);

    wait_meas_cfg_bit(7); // COEF_RDY

    uint8_t coeffs[18];
    i2c_read_reg_buf(dev_addr, 0x10, coeffs, sizeof(coeffs));

    ctx->c0 = twos_complement((coeffs[0] << 4) | ((coeffs[1] >> 4) & 0x0F), 12);
    ctx->c1 = twos_complement(((coeffs[1] & 0x0F) << 8) | coeffs[2], 12);
    ctx->c00 =
        twos_complement((coeffs[3] << 12) | (coeffs[4] << 4) | ((coeffs[5] >> 4) & 0x0F), 20);
    ctx->c10 = twos_complement(((coeffs[5] & 0x0F) << 16) | (coeffs[6] << 8) | coeffs[7], 20);
    ctx->c01 = twos_complement((coeffs[8] << 8) | coeffs[9], 16);
    ctx->c11 = twos_complement((coeffs[10] << 8) | coeffs[11], 16);
    ctx->c20 = twos_complement((coeffs[12] << 8) | coeffs[13], 16);
    ctx->c21 = twos_complement((coeffs[14] << 8) | coeffs[15], 16);
    ctx->c30 = twos_complement((coeffs[16] << 8) | coeffs[17], 16);

    uint8_t temp_src = read_reg(DPS310_TMP_COEF_SRCE) & 0x80;
    uint8_t rate = OVERSAMPLE_BITS | (RATE_BITS << 4);
    write_reg(DPS310_PRS_CFG, rate);
    write_reg(DPS310_TMP_CFG, rate | temp_src);

    // enable bg measure
    write_reg(DPS310_MEAS_CFG, 0b111);

    DMESG("DPS310 init complete");
}

static void dps310_sleep(void) {
    write_reg(DPS310_MEAS_CFG, 0);
}

static void dps310_process(void) {
    ctx_t *ctx = &state;
    if (jd_should_sample(&ctx->nextsample, SAMPLING_MS * 1000)) {
        if (meas_cfg_has_bit(5)) {
            float raw_temp = read_scaled(DPS310_TMP_B2);
            ctx->raw_temp = raw_temp;
            float temp = raw_temp * ctx->c1 + ctx->c0 * 0.5;
            int temp_i = (int)(temp * 1024 + 0.5);
            env_set_value(&ctx->temperature, temp_i, temperature_error);
        }

        if (!isnan(ctx->raw_temp) && meas_cfg_has_bit(4)) {
            float p_raw = read_scaled(DPS310_PRS_B2);
            float pascals = ctx->c00 + p_raw * (ctx->c10 + p_raw * (ctx->c20 + p_raw * ctx->c30)) +
                            ctx->raw_temp * (ctx->c01 + p_raw * (ctx->c11 + p_raw * ctx->c21));
            int pressure = (int)(pascals * 0.01 * 1024 + 0.5);
            env_set_value(&ctx->pressure, pressure, pressure_error);
            ctx->inited = 3;
        }
    }
}

ENV_INIT_DUAL(dps310_init, dps310_sleep);

ENV_DEFINE_GETTER(dps310, temperature)
ENV_DEFINE_GETTER(dps310, pressure)

#define FUNS(val, idx) ENV_GETTER(dps310, val, idx), .is_present = dps310_is_present

const env_sensor_api_t temperature_dps310 = {FUNS(temperature, 0)};
const env_sensor_api_t pressure_dps310 = {FUNS(pressure, 1)};
