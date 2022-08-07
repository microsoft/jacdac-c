#include "jd_drivers.h"

#ifndef ACC_I2C_ADDR
#define ACC_I2C_ADDR 0x27
#endif

// Registers
#define REG_CHIP_ID 0x01

#define ACCEL_RANGE(dps, cfg, scale)                                                               \
    { dps * 1024 * 1024, cfg, scale }

static const sensor_range_t accel_ranges[] = { //
    ACCEL_RANGE(2, (0), 6),
    ACCEL_RANGE(4, (1), 7),
    ACCEL_RANGE(8, (2), 8),
    ACCEL_RANGE(16, (3), 9),
    {0, 0, 0}};

static const sensor_range_t *r_accel;

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
    int v;
    writeReg(0x7f, 0x83);
    writeReg(0x7f, 0x69);
    writeReg(0x7f, 0xbd);

    v = readReg(0x8e);
    if (v == 0){
        writeReg(0x80, 0x50);
    }

    writeReg(0x0f, 0x40);
    writeReg(0x0f, 0x03 | r_accel->config);
    writeReg(0x20, 0x00);
    writeReg(0x11, 0x34); // power on
    writeReg(0x10, 0x07);
    writeReg(0x1a, 0x04);
    writeReg(0x15, 0x04);
}

static void *da213b_get_sample(void) {
    int16_t data[3];
    static int32_t sample[3];
    readData(0x02, (uint8_t *)data, 6);
    int shift = r_accel->scale;
    sample[0] = data[1] << shift;
    sample[1] = -data[0] << shift;
    sample[2] = -data[2] << shift;

    return sample;
}

static void da213b_sleep(void) {
    writeReg(0x11, 0x80); // power off
}

static int32_t da213b_get_range(void) {
    return r_accel->range;
}

static int32_t da213b_set_range(int32_t range) {
    r_accel = sensor_lookup_range(accel_ranges, range);
    init_chip();
    return r_accel->range;
}

static void da213b_init(void) {
    i2c_init();
    r_accel = sensor_lookup_range(accel_ranges, 8 << 20);

    int v = readReg(REG_CHIP_ID);
    DMESG("DA213b acc id: %x", v); // should be 0x13

    if (v == 0x13) {
        // OK
    } else {
        DMESG("invalid chip");
        hw_panic();
    }

    init_chip();
}

const accelerometer_api_t accelerometer_da213b = {
    .init = da213b_init,
    .get_reading = da213b_get_sample,
    .get_range = da213b_get_range,
    .set_range = da213b_set_range,
    .sleep = da213b_sleep,
};
