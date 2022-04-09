#ifndef DRV_I2C_MASTER_H
#define DRV_I2C_MASTER_H

#include "drv_name.h"

#define I2C_MASTER_SPEED_STANDARD 100000
#define I2C_MASTER_ERROR -1

typedef struct {
    uint8_t addr;

    // ignored
    pin_name_t sda;
    pin_name_t scl;
    uint32_t speed;

    // todo?
    uint16_t timeout_pass_count;
} i2c_master_config_t;

typedef struct {
    uint8_t address;
    uint16_t timeout_pass_count;
} i2c_master_t;

static inline void i2c_master_configure_default(i2c_master_config_t *config) {
    config->addr = 0;
    // the rest is ignored or needs to be set anyways
    config->timeout_pass_count = 10000;
}

static inline int i2c_master_open(i2c_master_t *obj, i2c_master_config_t *config) {
    obj->address = config->addr;
    obj->timeout_pass_count = config->timeout_pass_count;
    i2c_init();
    return 0;
}

static inline int i2c_master_set_speed(i2c_master_t *obj, uint32_t speed) {
    return 0;
}

static inline int i2c_master_set_timeout(i2c_master_t *obj, uint16_t timeout_pass_count) {
    obj->timeout_pass_count = timeout_pass_count;
    return 0;
}

static inline int i2c_master_set_slave_address(i2c_master_t *obj, uint8_t address) {
    obj->address = address;
    return 0;
}

static inline int i2c_master_write(i2c_master_t *obj, const uint8_t *buf, size_t len) {
    return i2c_write_ex(obj->address, buf, len, false) < 0 ? -1 : 0;
}

static inline int i2c_master_read(i2c_master_t *obj, uint8_t *buf, size_t len) {
    return i2c_read_ex(obj->address, buf, len) < 0 ? -1 : 0;
}

static inline int i2c_master_write_then_read(i2c_master_t *obj, const uint8_t *write_buf,
                                             size_t len_write, uint8_t *read_buf, size_t len_read) {
    int r = i2c_write_ex(obj->address, write_buf, len_write, true);
    if (r < 0)
        return -1;
    return i2c_read_ex(obj->address, read_buf, len_read) < 0 ? -1 : 0;
}

static inline void i2c_master_close(i2c_master_t *obj) {}

#endif
