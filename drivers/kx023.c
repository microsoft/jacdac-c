#include "jd_drivers.h"

#ifndef ACC_I2C_ADDR
// alt address 0x1E
#define ACC_I2C_ADDR 0x1F
#endif

#define WHO_AM_I 0x0F
#define CNTL1 0x18
#define ODCNTL 0x1B
#define CTRL_REG2 0x1D
#define INC1 0x1C
#define INC4 0x1F

#ifndef ACC_RANGE
#define ACC_RANGE 8
#endif

#define RANGE_2G (0b00 << 3)
#define RANGE_4G (0b01 << 3)
#define RANGE_8G (0b10 << 3)

#if ACC_RANGE == 8
#define RANGE RANGE_8G
#define ACC_SHIFT 8
#else
#error "untested range"
#endif

#ifdef PIN_ACC_CS
#define CS_SET(v) pin_set(PIN_ACC_CS, v)
#else
#define CS_SET(v) ((void)0)
#endif

static uint8_t acc_addr = ACC_I2C_ADDR;

static void writeReg(uint8_t reg, uint8_t val) {
#ifdef ACC_SPI
    CS_SET(0);
    uint8_t buf[] = {reg, val};
    bspi_send(buf, 2);
    CS_SET(1);
#else
    i2c_write_reg(acc_addr, reg, val);
#endif
}

static void readData(uint8_t reg, uint8_t *dst, int len) {
#ifdef ACC_SPI
    CS_SET(0);
    reg |= 0x80;
    bspi_send(&reg, 1);
    bspi_recv(dst, len);
    CS_SET(1);
#else
    i2c_read_reg_buf(acc_addr, reg, dst, len);
#endif
}

static int readReg(uint8_t reg) {
    uint8_t r = 0;
    readData(reg, &r, 1);
    return r;
}

static void init_chip(void) {
    writeReg(CNTL1, 0); // disable everything

#ifdef ACC_100HZ
    writeReg(ODCNTL, 0b0011);
#else
    writeReg(ODCNTL, 0b0010);
#endif

    uint8_t cntl1 = 0b11000000 | RANGE;

#ifdef PIN_ACC_INT
    writeReg(INC1, 0b00111000);
    writeReg(INC4, 0b00010000);
    cntl1 |= 0b00100000;
#endif

    writeReg(CNTL1, cntl1); // write CNTL1 last as it enables chip
}

static void *kx023_get_sample(void) {
    static int32_t sample[3];
    int16_t data[3];
    readData(0x06, (uint8_t *)data, 6);
    sample[0] = data[1] << ACC_SHIFT;
    sample[1] = -data[0] << ACC_SHIFT;
    sample[2] = -data[2] << ACC_SHIFT;
    return sample;
}

static void kx023_sleep(void) {
    writeReg(CNTL1, 0x00);
}

static bool kx023_is_present(void) {
    for (int i = 0x1E; i <= 0x1F; ++i) {
        int v = i2c_read_reg(i, WHO_AM_I);
        if (v == 0x15) {
            acc_addr = i;
            return 1;
        }
    }
    return 0;
}

static void kx023_init(void) {
#ifdef ACC_SPI
    bspi_init();
#else
    i2c_init();
#endif

    int v = readReg(WHO_AM_I);
    DMESG("KX023 acc id: %x", v);

    if (v == 0x15) {
        // OK
    } else {
        DMESG("invalid chip");
        JD_PANIC();
    }

    init_chip();
}

const accelerometer_api_t accelerometer_kx023 = {
    .init = kx023_init,
    .get_reading = kx023_get_sample,
    .sleep = kx023_sleep,
    .is_present = kx023_is_present,
};
