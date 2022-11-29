#include "jd_drivers.h"

#ifndef LPS33HW_I2C_ADDR
#define LPS33HW_I2C_ADDR 0x5D // LPS33HW default i2c address
#endif

#define LPS33HW_DEFAULT_CTRL_REG1 0x22
#define LPS33HW_DEFAULT_CTRL_REG2 0x10

#define LPS33HW_ID 0xB1

#define LPS33HW_WHO_AM_I 0x0F  // Chip ID
#define LPS33HW_CTRL_REG1 0x10 // Control register 1
#define LPS33HW_CTRL_REG2 0x11 // Control register 2
#define LPS33HW_CTRL_REG3 0x12 // Control register 3

#define LPS33HW_INTERRUPT_CFG 0x0B // Interrupt configuration register
#define LPS33HW_THS_P_L 0x0C       // Threshold pressure low byte
#define LPS33HW_THS_P_H 0x0D       // Threshold pressure high byte
#define LPS33HW_FIFO_CTRL 0x14     // FIFO Control register
#define LPS33HW_REF_P_XL 0x15      // Reference pressure low byte
#define LPS33HW_REF_P_L 0x16       // Reference pressure mid byte
#define LPS33HW_REF_P_H 0x17       // Reference pressure high byte
#define LPS33HW_RPDS_L 0x18        // Offset pressure low byte
#define LPS33HW_RPDS_H 0x19        // Offset pressure high byte
#define LPS33HW_RES_CONF 0x1A      // Low power mode configuration
#define LPS33HW_INT_SOURCE 0x25    // Interrupt source
#define LPS33HW_FIFO_STATUS 0x26   // FIFO Status
#define LPS33HW_STATUS 0x27        // Status register
#define LPS33HW_PRESS_OUT_XL 0x28  // Pressure low byte
#define LPS33HW_PRESS_OUT_L 0x29   // Pressure mid byte
#define LPS33HW_PRESS_OUT_H 0x2A   // Pressure high byte
#define LPS33HW_TEMP_OUT_L 0x2B    // Temperature low byte
#define LPS33HW_TEMP_OUT_H 0x2C    // Temperature high byte
#define LPS33HW_LPFP_RES 0x33      // Low pass filter reset

#define SAMPLING_MS 100 // 10 hz

static uint8_t lps33hw_addr = LPS33HW_I2C_ADDR;

typedef struct state {
    uint8_t inited;
    uint8_t ctrl1;
    env_reading_t pressure;
    env_reading_t temperature;
    uint32_t nextsample;
} ctx_t;
static ctx_t state;

static const int32_t pressure_error[] = {ERR_PRESSURE(260, 1), ERR_PRESSURE(1260, 1), ERR_END};

static const int32_t temperature_error[] = {ERR_TEMP(-40, 1.0), ERR_TEMP(85, 1.0), ERR_END};

typedef enum {
    RATE_ONE_SHOT = 0x00,
    RATE_1_HZ = 0x01,  /** 1 hz  **/
    RATE_10_HZ = 0x02, /** 10 hz  **/
    RATE_25_HZ = 0x03, /** 25 hz  **/
    RATE_50_HZ = 0x04, /** 50 hz  **/
    RATE_75_HZ = 0x05  /** 75 hz  **/
} LPS33HW_DataRate;

typedef enum {
    LowPassFilter_Off = 0x00,
    LowPassFilter_ODR9 = 0x02,
    LowPassFilter_ODR20 = 0x03
} LPS33HW_LowPassFilter;

static void writeReg(uint8_t reg, uint8_t val) {
    i2c_write_reg(lps33hw_addr, reg, val);
}

static void readData(uint8_t reg, uint8_t *dst, int len) {
    int r = i2c_read_reg_buf(lps33hw_addr, reg, dst, len);
    if (r < 0)
        JD_PANIC();
}

static int readReg(uint8_t reg) {
    uint8_t r = 0;
    readData(reg, &r, 1);
    return r;
}

__attribute__((unused)) void lps33hwtr_reset(void) {

    ctx_t *ctx = &state;
    ctx->ctrl1 = readReg(LPS33HW_CTRL_REG1);
    // Reset and reboot
    // BOOT|FIFO_EN|STOP_ON_FTH|IF_ADD_INC|I2C_DIS|[SWRESET]|0|ONE_SHOT
    writeReg(LPS33HW_CTRL_REG2, LPS33HW_DEFAULT_CTRL_REG2 | 0x04);
    writeReg(LPS33HW_CTRL_REG1, ctx->ctrl1);
}

static void lps33hwtr_set_rate(LPS33HW_DataRate new_rate) {
    ctx_t *ctx = &state;
    // 0|[ODR2]|[ODR1]|[ODR0]|EN_LPFP|LPFP_CFG|BDU|SIM
    ctx->ctrl1 &= ~(0x70);                  // 01110000
    ctx->ctrl1 |= ((uint8_t)new_rate << 4); // 0XYZ0000 LSB

    writeReg(LPS33HW_CTRL_REG1, ctx->ctrl1);
}

static bool lps33hwtr_is_present(void) {
    for (int i = 0x5C; i <= 0x5D; ++i) {
        int v = i2c_read_reg(i, LPS33HW_WHO_AM_I);
        if (v == LPS33HW_ID) {
            lps33hw_addr = i;
            return 1;
        }
    }
    return 0;
}

static void lps33hwtr_init(void) {
    ctx_t *ctx = &state;
    if (ctx->inited)
        return;

    ctx->pressure.min_value = SCALE_PRESSURE(260);
    ctx->pressure.max_value = SCALE_PRESSURE(1260);
    ctx->temperature.min_value = SCALE_TEMP(-40);
    ctx->temperature.max_value = SCALE_TEMP(85);

    ctx->nextsample = now;

    ctx->inited = 1;

    i2c_init();

    int v = readReg(LPS33HW_WHO_AM_I);

    DMESG("LPS33HW id: %x", v);

    if (v == LPS33HW_ID) {
        // OK

        // Reset and reboot
        // BOOT|FIFO_EN|STOP_ON_FTH|IF_ADD_INC|I2C_DIS|[SWRESET]|0|ONE_SHOT
        writeReg(LPS33HW_CTRL_REG2, LPS33HW_DEFAULT_CTRL_REG2 | 0x04);
        while (readReg(LPS33HW_CTRL_REG2) & 0x04)
            ;

        // writeReg(LPS33HW_CTRL_REG1, ctx->ctrl1);
    } else {
        DMESG("invalid chip");
        JD_PANIC();
    }
    lps33hwtr_set_rate(RATE_10_HZ);
    // 0|ODR2|ODR1|ODR0|EN_LPFP|LPFP_CFG|[BDU]|SIM
    // setup block reads Default value: 0 (0: continuous update, 1:output registers not updated
    // until MSB and LSB have been read)
    ctx->ctrl1 = readReg(LPS33HW_CTRL_REG1);
    writeReg(LPS33HW_CTRL_REG1, ctx->ctrl1 | 0x02);
}

__attribute__((unused)) void lps33hwtr_set_pressure_zero(void) {
    uint8_t configInterrupt = readReg(LPS33HW_INTERRUPT_CFG);
    configInterrupt &= ~(0x08);
    configInterrupt |= ((uint8_t)0x01 << 4);

    writeReg(LPS33HW_INTERRUPT_CFG, configInterrupt);
}

__attribute__((unused)) uint32_t lps33hwtr_get_pressure(void) {

    uint32_t data[3];
    readData(LPS33HW_PRESS_OUT_XL, (uint8_t *)data, 3);
    uint32_t pressure = (data[2] << 16) | (data[1] << 8) | data[1];
    pressure = pressure >> 2;

    return pressure;
}

__attribute__((unused)) int32_t lps33hwtr_get_temperature(void) {
    int32_t temp;
    int16_t rtemp = (readReg(LPS33HW_TEMP_OUT_H) << 8);
    rtemp |= readReg(LPS33HW_TEMP_OUT_L);
    temp = rtemp * 10;
    return temp;
}

static void lps33hwtr_process(void) {
    ctx_t *ctx = &state;

    if (jd_should_sample_delay(&ctx->nextsample, SAMPLING_MS * 1000)) {
        uint8_t data[5];
        readData(LPS33HW_PRESS_OUT_XL, (uint8_t *)data, sizeof(data));
        uint32_t pressure = (data[2] << 16) | (data[1] << 8) | data[0];
        pressure = pressure >> 2;

        int16_t rtemp = (data[4] << 8) | data[3];
        int32_t temp = (rtemp << 10) / 100;

        env_set_value(&ctx->pressure, pressure, pressure_error);
        env_set_value(&ctx->temperature, temp, temperature_error);

        ctx->inited = 2;
    }
}

static void *lps33hwtr_pressure(void) {
    ctx_t *ctx = &state;
    if (ctx->inited >= 2)
        return &ctx->pressure;
    return NULL;
}

static void *lps33hwtr_temperature(void) {
    ctx_t *ctx = &state;
    if (ctx->inited >= 2)
        return &ctx->temperature;
    return NULL;
}

const env_sensor_api_t temperature_lps33hwtr = {
    .init = lps33hwtr_init,
    .process = lps33hwtr_process,
    .get_reading = lps33hwtr_temperature,
    .is_present = lps33hwtr_is_present,
};

const env_sensor_api_t pressure_lps33hwtr = {
    .init = lps33hwtr_init,
    .process = lps33hwtr_process,
    .get_reading = lps33hwtr_pressure,
    .is_present = lps33hwtr_is_present,
};
