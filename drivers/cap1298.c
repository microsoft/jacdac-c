#include "jd_drivers.h"
#include "services/jd_services.h"

#define CAP1298_ADDR 0x28

#define PROD_ID_ADDR 0xFD
#define PROD_ID_VAL 0x71

#define MFR_ID_ADDR 0xFD
#define MFR_ID_VAL 0x5D

#define INT_EN_ADDR 0x27

#define MAIN_CONTROL_ADDR 0x0
#define GEN_STATUS_ADDR 0x2
#define SEN_STATUS_ADDR 0x3

#define REPEAT_ENABLE_ADDR 0x28
#define MULTI_TOUCH_CFG_ADDR 0x2a

static void writeReg(uint8_t reg, uint8_t val) {
    i2c_write_reg(CAP1298_ADDR, reg, val);
}

static void readData(uint8_t reg, uint8_t *dst, int len) {
    i2c_read_reg_buf(CAP1298_ADDR, reg, dst, len);
}

static int readReg(uint8_t reg) {
    uint8_t r = 0;
    readData(reg, &r, 1);
    return r;
}

static void clear_int(void) {
    writeReg(MAIN_CONTROL_ADDR, 0b00000000); // no multi-touch processing; we let app do that
    writeReg(REPEAT_ENABLE_ADDR, 0);
}

void *cap1298_read(void) {
    clear_int();
    uint8_t gen_status;
    uint8_t sen_status;
    static uint16_t sample[8];
    readData(GEN_STATUS_ADDR, &gen_status, 1);
    readData(SEN_STATUS_ADDR, &sen_status, 1);
    writeReg(MAIN_CONTROL_ADDR, 0b00000000);
    // DMESG("G: %x S: %x", gen_status, sen_status);
    for (int i = 0; i < 8; ++i)
        sample[i] = sen_status & (1 << i) ? 0xffff : 0x0000;
    return sample;
}

void cap1298_cfg(void) {
    clear_int();
    writeReg(MULTI_TOUCH_CFG_ADDR, 0b00000000);
}

bool cap1298_is_present(void) {
    return i2c_read_reg(CAP1298_ADDR, PROD_ID_ADDR) == PROD_ID_VAL;
}

void cap1298_init(void) {

    i2c_init();

    target_wait_us(1000);

    int v = readReg(PROD_ID_ADDR);
    DMESG("CAP1298 acc id: %x", v);

    if (v == PROD_ID_VAL) {
        // OK
    } else {
        DMESG("invalid chip");
        JD_PANIC();
    }

    cap1298_cfg();
}

const captouch_api_t captouch_cap1298 = {
    .init = cap1298_init,
    .get_reading = cap1298_read,
    .is_present = cap1298_is_present,
};