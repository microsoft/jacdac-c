#include "jd_drivers.h"

#ifdef ACC_KXTJ3

#ifndef ACC_I2C_ADDR
#define ACC_I2C_ADDR 0x0E
#endif

#define WHO_AM_I 0x0F
#define CTRL_REG1 0x1B
#define CTRL_REG2 0x1D
#define INT_CTRL_REG1 0x1E
#define INT_CTRL_REG2 0x1F
#define DATA_CTRL_REG 0x21

#ifndef ACC_RANGE
#define ACC_RANGE 8
#endif

#define RANGE_2G (0b000 << 2)
#define RANGE_4G (0b010 << 2)
#define RANGE_8G (0b100 << 2)
#define RANGE_16G (0b001 << 2)

#if ACC_RANGE == 8
#define RANGE RANGE_8G
#define ACC_SHIFT 8
#elif ACC_RANGE == 16
#define RANGE RANGE_16G
#define ACC_SHIFT 9
#else
#error "untested range"
#endif

static void writeReg(uint8_t reg, uint8_t val) {
    i2c_write_reg(ACC_I2C_ADDR, reg, val);
}

static void readData(uint8_t reg, uint8_t *dst, int len) {
    i2c_read_buf(ACC_I2C_ADDR, reg, dst, len);
}

static int readReg(uint8_t reg) {
    uint8_t r = 0;
    readData(reg, &r, 1);
    return r;
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

    writeReg(CTRL_REG1, 0b11100000 | RANGE); // write ctrl_reg1 last as it enables chip
}

void acc_hw_get(int32_t sample[3]) {
    int16_t data[3];
    readData(0x06, (uint8_t *)data, 6);
    sample[0] = data[1] << ACC_SHIFT;
    sample[1] = -data[0] << ACC_SHIFT;
    sample[2] = -data[2] << ACC_SHIFT;
}

void acc_hw_sleep(void) {
    writeReg(CTRL_REG1, 0x00);
}

void acc_hw_init(void) {
    i2c_init();

    int v = readReg(WHO_AM_I);
    DMESG("KXTJ3 acc id: %x", v);

    if (v == 0x35) {
        // OK
    } else {
        DMESG("invalid chip");
        hw_panic();
    }

    init_chip();
}

#endif
