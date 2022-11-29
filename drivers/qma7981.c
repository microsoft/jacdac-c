#include "jd_drivers.h"

#ifndef ACC_I2C_ADDR
#define ACC_I2C_ADDR 0x12
#endif

#define REG_CHIP_ID 0x00
#define REG_DX 0x01
#define REG_DY 0x03
#define REG_DZ 0x05
#define REG_STEP_CNT0 0x07
#define REG_STEP_CNT1 0x08
#define REG_INT_ST0 0x09
#define REG_INT_ST1 0x0a
#define REG_INT_ST2 0x0b
#define REG_STEP_CNT2 0x0e
#define REG_FSR 0x0f
#define REG_BW 0x10
#define REG_PM 0x11
#define REG_STEP_CONF0 0x12
#define REG_STEP_CONF1 0x13
#define REG_STEP_CONF2 0x14
#define REG_STEP_CONF3 0x15
#define REG_INT_EN0 0x16
#define REG_INT_EN1 0x17
#define REG_INT_EN2 0x18
#define REG_INT_MAP0 0x19
#define REG_INT_MAP1 0x1a
#define REG_INT_MAP2 0x1b
#define REG_INT_MAP3 0x1c
#define REG_SIG_STEP_TH 0x1d
#define REG_INTPIN_CONF 0x20
#define REG_INT_CFG 0x21
#define REG_OS_CUST_X 0x27
#define REG_OS_CUST_Y 0x28
#define REG_OS_CUST_Z 0x29
#define REG_MOT_CONF0 0x2c
#define REG_MOT_CONF1 0x2d
#define REG_MOT_CONF2 0x2e
#define REG_MOT_CONF3 0x2f
#define REG_ST 0x32
#define REG_RAISE_WAKE_PERIOD 0x35
#define REG_SR 0x36
#define REG_RAISE_WAKE_TIMEOUT_TH 0x3e

#define QMAX981_RANGE_2G 0x01
#define QMAX981_RANGE_4G 0x02
#define QMAX981_RANGE_8G 0x04
#define QMAX981_RANGE_16G 0x08
#define QMAX981_RANGE_32G 0x0f

#define QMAX981_BW_8HZ 0xE7
#define QMAX981_BW_16HZ 0xE6
#define QMAX981_BW_32HZ 0xE5
#define QMAX981_BW_65HZ 0xE0
#define QMAX981_BW_130HZ 0xE1
#define QMAX981_BW_258HZ 0xE2
#define QMAX981_BW_513HZ 0xE3

#ifndef ACC_RANGE
#define ACC_RANGE 8
#endif

#if ACC_RANGE == 8
#define QMAX981_RANGE QMAX981_RANGE_8G
#define ACC_SHIFT 8
#elif ACC_RANGE == 16
#define QMAX981_RANGE QMAX981_RANGE_16G
#define ACC_SHIFT 9
#else
#error "untested range"
#endif

#ifndef ACC_SPI
#define ACC_I2C 1
#endif

static void writeReg(uint8_t reg, uint8_t val) {
#ifdef ACC_I2C
    i2c_write_reg(ACC_I2C_ADDR, reg, val);
#else
    // target_wait_us(200);
    uint8_t cmd[] = {
        0xAE, // IDW
        0x00, // reg high
        reg,  // register
        0x00, // len high
        0x01, // len low,
        val,  // data
    };
    pin_set(PIN_ACC_CS, 0);
    bspi_send(cmd, sizeof(cmd));
    pin_set(PIN_ACC_CS, 1);
#endif
}

static void readData(uint8_t reg, uint8_t *dst, int len) {
#ifdef ACC_I2C
    i2c_read_reg_buf(ACC_I2C_ADDR, reg, dst, len);
#else
    pin_set(PIN_ACC_CS, 0);
    uint8_t cmd[] = {
        0xAF, // IDR
        0x00, // reg high
        reg,  // register
        0x00, // len high
        len,  // len low,
    };

    bspi_send(cmd, sizeof(cmd));
    bspi_recv(dst, len);
    pin_set(PIN_ACC_CS, 1);
#endif
}

static int readReg(uint8_t reg) {
    uint8_t r = 0;
    readData(reg, &r, 1);
    return r;
}

static void init_chip(void) {
    writeReg(REG_PM, 0x80);
    writeReg(REG_SR, 0xB6);
    target_wait_us(100);
    writeReg(REG_SR, 0x00);
    writeReg(REG_PM, 0x80);
#ifdef ACC_I2C
    writeReg(REG_INT_CFG, 0x1C);
#else
    writeReg(REG_INT_CFG, 0x1C | (1 << 5)); // disable I2C
#endif
    writeReg(REG_INTPIN_CONF, 0x05 | (1 << 7)); // disable pull-up on CS
    writeReg(REG_FSR, QMAX981_RANGE);

    // ~50uA
    // writeReg(REG_PM, 0x84); // MCK 50kHz
    // writeReg(REG_BW, 0xE3); // sample 50kHz/975 = 51.3Hz

#if 0
    // ~60uA
    writeReg(REG_PM, 0x83); // MCK 100kHz
#ifdef ACC_100HZ
    writeReg(REG_BW, 0xE3); // sample 100kHz/975 =~ 100Hz
#else
    writeReg(REG_BW, 0xE2);                 // sample 100kHz/1935 = 51.7Hz
#endif
#else
    writeReg(REG_PM, 0x80);                 // MCK 500kHz
#ifdef ACC_100HZ
    writeReg(REG_BW, 0xE1);                 // sample 500kHz/3855 =~ 130Hz
#else
    writeReg(REG_BW, 0xE0); // sample 100kHz/7695 = 65Hz
#endif
#endif

#ifdef PIN_ACC_INT
    writeReg(REG_INT_EN1, 0xE0 | (1 << 4));
    writeReg(REG_INT_MAP1, 0x62 | (1 << 4));
#endif

    //    writeReg(REG_PM, 0x40); // sleep

    // ~150uA - 500kHz
    // writeReg(REG_BW, 0xE0); // 65Hz
    // 0xE1 130Hz
    // 0xE2 260Hz
}

static void *qma7981_get_sample(void) {
    int16_t data[3];
    static int32_t sample[3];
    readData(REG_DX, (uint8_t *)data, 6);
    sample[0] = data[1] << ACC_SHIFT;
    sample[1] = -data[0] << ACC_SHIFT;
    sample[2] = -data[2] << ACC_SHIFT;
    return sample;
}

static void qma7981_sleep(void) {
    writeReg(REG_PM, 0x00);
}

static void qma7981_init(void) {
#ifdef ACC_I2C
    i2c_init();
#else
    bspi_init();
#endif

#ifdef PIN_ACC_VCC
    pin_setup_output(PIN_ACC_VCC);
    pin_set(PIN_ACC_VCC, 1);
#endif

    // 9us is enough; datasheet claims 250us for I2C ready and 2ms for conversion ready
    target_wait_us(250);

    int v = readReg(REG_CHIP_ID);
    DMESG("acc id: %x", v);

    if (0xe0 <= v && v <= 0xe9) {
        if (v >= 0xe8) {
            // v2
        }
    } else {
        DMESG("invalid chip");
        JD_PANIC();
    }

    init_chip();
}

const accelerometer_api_t accelerometer_qma7981 = {
    .init = qma7981_init,
    .get_reading = qma7981_get_sample,
    .sleep = qma7981_sleep,
};
