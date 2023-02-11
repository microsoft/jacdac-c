#include "jd_drivers.h"

#if JD_I2C_HELPERS

#define CHECK_RET(call)                                                                            \
    do {                                                                                           \
        int _r = call;                                                                             \
        if (_r < 0)                                                                                \
            return _r;                                                                             \
    } while (0)

int i2c_write_ex(uint8_t device_address, const void *src, unsigned len, bool repeated) {
    return i2c_write_ex2(device_address, src, len, NULL, 0, repeated);
}

// 8-bit reg addr
int i2c_write_reg_buf(uint8_t addr, uint8_t reg, const void *src, unsigned len) {
    return i2c_write_ex2(addr, &reg, 1, src, len, false);
}

int i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t val) {
    return i2c_write_reg_buf(addr, reg, &val, 1);
}

int i2c_read_reg_buf(uint8_t addr, uint8_t reg, void *dst, unsigned len) {
    CHECK_RET(i2c_write_ex(addr, &reg, 1, true) < 0);
    return i2c_read_ex(addr, dst, len);
}

int i2c_read_reg(uint8_t addr, uint8_t reg) {
    uint8_t r = 0;
    CHECK_RET(i2c_read_reg_buf(addr, reg, &r, 1));
    return r;
}

// 16-bit reg addr
int i2c_write_reg16_buf(uint8_t addr, uint16_t reg, const void *src, unsigned len) {
    uint8_t regaddr[2] = {reg >> 8, reg & 0xff};
    return i2c_write_ex2(addr, regaddr, 2, src, len, false);
}

int i2c_write_reg16(uint8_t addr, uint16_t reg, uint8_t val) {
    return i2c_write_reg16_buf(addr, reg, &val, 1);
}

int i2c_read_reg16_buf(uint8_t addr, uint16_t reg, void *dst, unsigned len) {
    uint8_t a[] = {reg >> 8, reg & 0xff};
    CHECK_RET(i2c_write_ex(addr, a, 2, true));
    return i2c_read_ex(addr, dst, len);
}

int i2c_read_reg16(uint8_t addr, uint16_t reg) {
    uint8_t r = 0;
    CHECK_RET(i2c_read_reg16_buf(addr, reg, &r, 1));
    return r;
}

#endif