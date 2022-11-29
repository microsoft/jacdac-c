#include "jd_drivers.h"

#ifndef ACC_I2C_ADDR
#define ACC_I2C_ADDR 0x6A
#endif

#define LSM6DS_WHOAMI 0x0F
#define ID_LSM6DSOX 0x6C
#define ID_LSM6DS3TR_C 0x6A

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

#define GYRO_RANGE(dps, cfg, scale)                                                                \
    { dps * 1024 * 1024, cfg, ((int)(1024 * 1024 * scale) / 1000) }

static const sensor_range_t gyro_ranges[] = { //
    GYRO_RANGE(250, (0b00 << 2), 8.75),
    GYRO_RANGE(500, (0b01 << 2), 17.5),
    GYRO_RANGE(1000, (0b10 << 2), 35),
    GYRO_RANGE(2000, (0b11 << 2), 70),
    {0, 0, 0}};

#define ACCEL_RANGE(dps, cfg, scale)                                                               \
    { dps * 1024 * 1024, cfg, scale }

static const sensor_range_t accel_ranges[] = { //
    ACCEL_RANGE(2, (0b00 << 2), 6),
    ACCEL_RANGE(4, (0b10 << 2), 7),
    ACCEL_RANGE(8, (0b11 << 2), 8),
    ACCEL_RANGE(16, (0b01 << 2), 9),
    {0, 0, 0}};

#ifdef ACC_100HZ
#define ODR (0b0100 << 4) // 104Hz
#else
#define ODR (0b0011 << 4) // 52Hz
#endif

static uint8_t inited;
static uint8_t acc_addr = ACC_I2C_ADDR;

static const sensor_range_t *r_accel, *r_gyro;

static void writeReg(uint8_t reg, uint8_t val) {
    i2c_write_reg(acc_addr, reg, val);
}

static void readData(uint8_t reg, uint8_t *dst, int len) {
    i2c_read_reg_buf(acc_addr, reg, dst, len);
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
    writeReg(LSM6DS_CTRL1_XL, r_accel->config | ODR);
    writeReg(LSM6DS_CTRL2_G, r_gyro->config | ODR);
}

static void *lsm6ds_get_sample(void) {
    int16_t data[3];
    static int32_t sample[3];
    readData(LSM6DS_OUTX_L_A, (uint8_t *)data, 6);
    int shift = r_accel->scale;
    sample[0] = data[0] << shift;
    sample[1] = data[1] << shift;
    sample[2] = data[2] << shift;
    return sample;
}

static void *lsm6ds_get_sample_gyro(void) {
    int16_t data[3];
    static int32_t sample[3];
    readData(LSM6DS_OUTX_L_G, (uint8_t *)data, 6);
    int32_t mul = r_gyro->scale;
    sample[0] = data[0] * mul;
    sample[1] = data[1] * mul;
    sample[2] = data[2] * mul;
    return sample;
}

static void lsm6ds_sleep(void) {
    writeReg(LSM6DS_CTRL1_XL, 0);
    writeReg(LSM6DS_CTRL2_G, 0);
    inited = 0;
}

static int32_t lsm6ds_accel_get_range(void) {
    return r_accel->range;
}

static int32_t lsm6ds_accel_set_range(int32_t range) {
    r_accel = sensor_lookup_range(accel_ranges, range);
    init_chip();
    return r_accel->range;
}

static int32_t lsm6ds_gyro_get_range(void) {
    return r_gyro->range;
}

static int32_t lsm6ds_gyro_set_range(int32_t range) {
    r_gyro = sensor_lookup_range(gyro_ranges, range);
    init_chip();
    return r_gyro->range;
}

static bool lsm6ds_is_present(void) {
    for (int i = 0x6A; i <= 0x6B; ++i) {
        int v = i2c_read_reg(i, LSM6DS_WHOAMI);
        if (v == ID_LSM6DSOX || v == ID_LSM6DS3TR_C) {
            acc_addr = i;
            return 1;
        }
    }
    return 0;
}

static void lsm6ds_init(void) {
    if (inited)
        return;
    inited = 1;
    i2c_init();

    r_accel = sensor_lookup_range(accel_ranges, 8 << 20);
    r_gyro = sensor_lookup_range(gyro_ranges, 1000 << 20);

    int v = readReg(LSM6DS_WHOAMI);
    DMESG("LSM acc id: %x", v);

    if (v == ID_LSM6DSOX || v == ID_LSM6DS3TR_C) {
        // OK
    } else {
        DMESG("invalid chip");
        JD_PANIC();
    }

    init_chip();
}

const accelerometer_api_t accelerometer_lsm6ds = {
    .init = lsm6ds_init,
    .get_reading = lsm6ds_get_sample,
    .sleep = lsm6ds_sleep,
    .get_range = lsm6ds_accel_get_range,
    .set_range = lsm6ds_accel_set_range,
    .ranges = accel_ranges,
    .is_present = lsm6ds_is_present,
};

const gyroscope_api_t gyroscope_lsm6ds = {
    .init = lsm6ds_init,
    .get_reading = lsm6ds_get_sample_gyro,
    .sleep = lsm6ds_sleep,
    .get_range = lsm6ds_gyro_get_range,
    .set_range = lsm6ds_gyro_set_range,
    .ranges = gyro_ranges,
    .is_present = lsm6ds_is_present,
};
