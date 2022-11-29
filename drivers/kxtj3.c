#include "jd_drivers.h"

#ifndef ACC_I2C_ADDR
#define ACC_I2C_ADDR 0x0E
#endif

#define WHO_AM_I 0x0F
#define CTRL_REG1 0x1B
#define CTRL_REG2 0x1D
#define INT_CTRL_REG1 0x1E
#define INT_CTRL_REG2 0x1F
#define DATA_CTRL_REG 0x21

#define ACCEL_RANGE(dps, cfg, scale)                                                               \
    { dps * 1024 * 1024, cfg, scale }

static uint8_t acc_addr = ACC_I2C_ADDR;

static const sensor_range_t accel_ranges[] = { //
    ACCEL_RANGE(2, (0b000 << 2), 6),
    ACCEL_RANGE(4, (0b010 << 2), 7),
    ACCEL_RANGE(8, (0b100 << 2), 8),
    ACCEL_RANGE(16, (0b001 << 2), 9),
    {0, 0, 0}};

static const sensor_range_t *r_accel;

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

static bool kxtj3_is_present(void) {
    for (int i = 0x0E; i <= 0x0F; ++i) {
        int v = i2c_read_reg(i, WHO_AM_I);
        if (v == 0x35) {
            acc_addr = i;
            return 1;
        }
    }
    return 0;
}

static void init_chip(void) {
    writeReg(CTRL_REG1, 0); // disable everything

#ifdef ACC_100HZ
    writeReg(DATA_CTRL_REG, 0b0011);
#else
    writeReg(DATA_CTRL_REG, 0b0010);
#endif

#ifdef PIN_ACC_INT
    writeReg(INT_CTRL_REG1, 0b00111000);
#endif

    writeReg(CTRL_REG1, 0b11100000 | r_accel->config); // write ctrl_reg1 last as it enables chip
}

static int32_t kxtj3_accel_get_range(void) {
    return r_accel->range;
}

static int32_t kxtj3_accel_set_range(int32_t range) {
    r_accel = sensor_lookup_range(accel_ranges, range);
    init_chip();
    return r_accel->range;
}

static void *kxtj3_get_sample(void) {
    int16_t data[3];
    static int32_t sample[3];
    readData(0x06, (uint8_t *)data, 6);
    int shift = r_accel->scale;
    sample[0] = data[1] << shift;
    sample[1] = -data[0] << shift;
    sample[2] = -data[2] << shift;
    return sample;
}

static void kxtj3_sleep(void) {
    writeReg(CTRL_REG1, 0x00);
}

static void kxtj3_init(void) {
    i2c_init();

    int v = readReg(WHO_AM_I);
    DMESG("KXTJ3 acc id: %x", v);

    if (v == 0x35) {
        // OK
    } else {
        DMESG("invalid chip");
        JD_PANIC();
    }

    kxtj3_accel_set_range(8 << 20);
}

const accelerometer_api_t accelerometer_kxtj3 = {
    .init = kxtj3_init,
    .get_reading = kxtj3_get_sample,
    .sleep = kxtj3_sleep,
    .get_range = kxtj3_accel_get_range,
    .set_range = kxtj3_accel_set_range,
    .ranges = accel_ranges,
    .is_present = kxtj3_is_present,
};
