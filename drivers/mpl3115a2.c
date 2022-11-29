#include "jd_drivers.h"

#define SAMPLING_MS 500

#ifndef MPL3115A2_ADDR
#define MPL3115A2_ADDR 0x60
#endif

#define MPL3115A2_REG_PRESSURE_MSB 0x01
#define MPL3115A2_REG_TEMP_MSB 0x04
#define MPL3115A2_WHOAMI 0x0C

#define MPL3115A2_REG_STATUS 0x00
#define MPL3115A2_REG_STATUS_TDR 0x02
#define MPL3115A2_REG_STATUS_PDR 0x04
#define MPL3115A2_REG_STATUS_PTDR 0x08

#define MPL3115A2_PT_DATA_CFG 0x13
#define MPL3115A2_PT_DATA_CFG_TDEFE 0x01
#define MPL3115A2_PT_DATA_CFG_PDEFE 0x02
#define MPL3115A2_PT_DATA_CFG_DREM 0x04

#define MPL3115A2_CTRL_REG1 0x26

#define MPL3115A2_CTRL_REG1_OST 0x02
#define MPL3115A2_CTRL_REG1_RST 0x04
#define MPL3115A2_CTRL_REG1_ALT 0x80

#define OVERSAMPLE_REG1(os) ((os & 7) << 3)
#define CONV_TIME(os) ((4 << os) + 2) // ms

#define CHOSEN_OVERSAMPLE 6 // 64x, 256ms

#define MPL3115A2_REG_STARTCONVERSION 0x12

typedef struct state {
    uint8_t inited;
    uint8_t read_issued;
    uint8_t ctrl1;
    env_reading_t pressure;
    env_reading_t temperature;
    uint32_t nextsample;
} ctx_t;
static ctx_t state;

static const int32_t pressure_error[] = {ERR_PRESSURE(200, 10), ERR_PRESSURE(500, 4),
                                         ERR_PRESSURE(1100, 4), ERR_END};

static const int32_t temperature_error[] = {ERR_TEMP(-40, 3), ERR_TEMP(85, 1), ERR_TEMP(85, 3),
                                            ERR_END};

static int read_reg(uint8_t reg) {
    int r = i2c_read_reg(MPL3115A2_ADDR, reg);
    if (r < 0)
        JD_PANIC();
    return r;
}

static void write_reg(uint8_t reg, uint8_t val) {
    int r = i2c_write_reg(MPL3115A2_ADDR, reg, val);
    if (r < 0)
        JD_PANIC();
}

static bool mpl3115a2_is_present(void) {
    return i2c_read_reg(MPL3115A2_ADDR, MPL3115A2_WHOAMI) == 0xC4;
}

static void mpl3115a2_init(void) {
    ctx_t *ctx = &state;
    if (ctx->inited)
        return;

    ctx->pressure.min_value = SCALE_PRESSURE(200);
    ctx->pressure.max_value = SCALE_PRESSURE(1100);
    ctx->temperature.min_value = SCALE_TEMP(-40);
    ctx->temperature.max_value = SCALE_TEMP(85);

    ctx->nextsample = now;

    ctx->inited = 1;
    i2c_init();

    if (read_reg(MPL3115A2_WHOAMI) != 0xC4)
        JD_PANIC();

    ctx->ctrl1 = OVERSAMPLE_REG1(CHOSEN_OVERSAMPLE);
    write_reg(MPL3115A2_CTRL_REG1, ctx->ctrl1);
    write_reg(MPL3115A2_PT_DATA_CFG, MPL3115A2_PT_DATA_CFG_TDEFE | MPL3115A2_PT_DATA_CFG_PDEFE |
                                         MPL3115A2_PT_DATA_CFG_DREM);
}

#define BOTH (MPL3115A2_REG_STATUS_PDR | MPL3115A2_REG_STATUS_TDR)
static void mpl3115a2_process(void) {
    ctx_t *ctx = &state;

    if (jd_should_sample_delay(&ctx->nextsample, 10000)) {
        if (!ctx->read_issued) {
            ctx->read_issued = 1;
            write_reg(MPL3115A2_CTRL_REG1, ctx->ctrl1 | MPL3115A2_CTRL_REG1_OST);
        } else {
            if ((read_reg(MPL3115A2_REG_STATUS) & BOTH) == BOTH) {
                uint8_t data[3];
                int r = i2c_read_reg_buf(MPL3115A2_ADDR, MPL3115A2_REG_PRESSURE_MSB, data,
                                         sizeof(data));
                if (r < 0)
                    JD_PANIC();

                uint32_t pressure = (data[0] << 16) | (data[1] << 8) | data[2];
                pressure = (pressure >> 4) * 64 / 25;

                r = i2c_read_reg_buf(MPL3115A2_ADDR, MPL3115A2_REG_TEMP_MSB, data, 2);
                if (r < 0)
                    JD_PANIC();

                int temp = (int16_t)((data[0] << 8) | data[1]);
                temp = (temp >> 4) << 6;

                ctx->read_issued = 0;
                ctx->nextsample = now + SAMPLING_MS * 1000;

                env_set_value(&ctx->pressure, pressure, pressure_error);
                env_set_value(&ctx->temperature, temp, temperature_error);

                ctx->inited = 2;
            }
        }
    }
}

static void *mpl3115a2_temperature(void) {
    ctx_t *ctx = &state;
    if (ctx->inited >= 2)
        return &ctx->temperature;
    return NULL;
}

static void *mpl3115a2_pressure(void) {
    ctx_t *ctx = &state;
    if (ctx->inited >= 2)
        return &ctx->pressure;
    return NULL;
}

const env_sensor_api_t temperature_mpl3115a2 = {
    .init = mpl3115a2_init,
    .process = mpl3115a2_process,
    .get_reading = mpl3115a2_temperature,
    .is_present = mpl3115a2_is_present,
};

const env_sensor_api_t pressure_mpl3115a2 = {
    .init = mpl3115a2_init,
    .process = mpl3115a2_process,
    .get_reading = mpl3115a2_pressure,
    .is_present = mpl3115a2_is_present,
};
