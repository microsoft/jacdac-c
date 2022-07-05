#include "jd_drivers.h"

#ifndef ACC_I2C_ADDR
#define ACC_I2C_ADDR 0x27
#endif

// Registers
#define REG_CHIP_ID 0x01

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

static void *da213b_get_sample(void) {
    int16_t data[3];
    static int32_t sample[3];
    readData(0x02, (uint8_t *)data, 6);
    sample[0] = data[1]/16;
    sample[1] = -data[0]/16;
    sample[2] = -data[2]/16;

    return sample;
}

static void da213b_sleep(void) {
    writeReg(0x11, 0x80); // power off
}


static void da213b_init(void) {
    i2c_init();

    int v = readReg(REG_CHIP_ID);
    DMESG("DA213b acc id: %x", v); // should be 0x13

    if (v == 0x13) {
        // OK
    } else {
        DMESG("invalid chip");
        hw_panic();
    }

    writeReg(0x7f, 0x83);
    writeReg(0x7f, 0x69);
    writeReg(0x7f, 0xbd);

    v = readReg(0x8e);
    if (v == 0){
        writeReg(0x80, 0x50);
    }

    writeReg(0x0f, 0x40);
    writeReg(0x20, 0x00);
    writeReg(0x11, 0x34); // power on
    writeReg(0x10, 0x07);
    writeReg(0x1a, 0x04);
    writeReg(0x15, 0x04);
}

const accelerometer_api_t accelerometer_da213b = {
    .init = da213b_init,
    .get_reading = da213b_get_sample,
    .sleep = da213b_sleep,
};
