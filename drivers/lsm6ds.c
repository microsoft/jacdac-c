#include "jd_drivers.h"

#ifndef ACC_I2C_ADDR
#define ACC_I2C_ADDR 0x6A
#endif

#define LSM6DS_WHOAMI 0x0F
#define LSM6DS_CTRL1_XL 0x10
#define LSM6DS_CTRL2_G 0x11
#define LSM6DS_CTRL3_C 0x12
#define LSM6DS_CTRL8_XL 0x17
#define LSM6DS_CTRL9_XL 0x18
#define LSM6DS_CTRL10_C 0x19
#define LSM6DS_OUT_TEMP_L 0x20
#define LSM6DS_OUTX_L_G 0x22
#define LSM6DS_OUTX_L_A 0x28
#define LSM6DS_STEP_COUNTER 0x4B
#define LSM6DS_TAP_CFG 0x58
#define LSM6DS_INT1_CTRL 0x0D

#ifndef ACC_RANGE
#define ACC_RANGE 8
#endif

#define RANGE_2G (0b00 << 2)
#define RANGE_4G (0b10 << 2)
#define RANGE_8G (0b11 << 2)
#define RANGE_16G (0b01 << 2)

#if ACC_RANGE == 8
#define RANGE RANGE_8G
#define ACC_SHIFT 8
#elif ACC_RANGE == 16
#define RANGE RANGE_16G
#define ACC_SHIFT 9
#else
#error "untested range"
#endif

#define RANGE_GYRO_250 (0b00 << 2)
#define RANGE_GYRO_500 (0b01 << 2)
#define RANGE_GYRO_1000 (0b10 << 2)
#define RANGE_GYRO_2000 (0b11 << 2)

// 35.0 at 1000dps (from data sheet)
#define G_MULT ((int)(1024*1024*35.0)/1000)
#define RANGE_GYRO RANGE_GYRO_1000

#ifdef ACC_100HZ
#define ODR (0b0100 << 4) // 104Hz
#else
#define ODR (0b0011 << 4) // 52Hz
#endif

static void writeReg(uint8_t reg, uint8_t val) {
    i2c_write_reg(ACC_I2C_ADDR, reg, val);
}

static void readData(uint8_t reg, uint8_t *dst, int len) {
    i2c_read_reg_buf(ACC_I2C_ADDR, reg, dst, len);
}

static int readReg(uint8_t reg) {
    uint8_t r = 0;
    readData(reg, &r, 1);
    return r;
}

static void init_chip(void) {
    writeReg(LSM6DS_CTRL3_C, 0b01000100);
#ifdef PIN_ACC_INT
    writeReg(LSM6DS_INT1_CTRL, 0b00000001);
#else
    writeReg(LSM6DS_INT1_CTRL, 0b00);
#endif
    writeReg(LSM6DS_CTRL1_XL, RANGE | ODR);
    writeReg(LSM6DS_CTRL2_G, RANGE_GYRO | ODR);
}

static void *lsm6ds_get_sample(void) {
    int16_t data[3];
    static int32_t sample[3];
    readData(LSM6DS_OUTX_L_A, (uint8_t *)data, 6);
    sample[0] = data[0] << ACC_SHIFT;
    sample[1] = data[1] << ACC_SHIFT;
    sample[2] = data[2] << ACC_SHIFT;
    return sample;
}

static void *lsm6ds_get_sample_gyro(void) {
    int16_t data[3];
    static int32_t sample[3];
    readData(LSM6DS_OUTX_L_G, (uint8_t *)data, 6);
    sample[0] = data[0] * G_MULT;
    sample[1] = data[1] * G_MULT;
    sample[2] = data[2] * G_MULT;
    return sample;
}

static uint8_t inited;

static void lsm6ds_sleep(void) {
    writeReg(LSM6DS_CTRL1_XL, 0);
    writeReg(LSM6DS_CTRL2_G, 0);
    inited = 0;
}

static void lsm6ds_init(void) {
    if (inited)
        return;
    inited = 1;
    i2c_init();

    int v = readReg(LSM6DS_WHOAMI);
    DMESG("LSM acc id: %x", v);

    if (v == 0x6C) {
        // OK
    } else {
        DMESG("invalid chip");
        hw_panic();
    }

    init_chip();
}

const accelerometer_api_t accelerometer_lsm6ds = {
    .init = lsm6ds_init,
    .get_reading = lsm6ds_get_sample,
    .sleep = lsm6ds_sleep,
};

const gyroscope_api_t gyroscope_lsm6ds = {
    .init = lsm6ds_init,
    .get_reading = lsm6ds_get_sample_gyro,
    .sleep = lsm6ds_sleep,
};
