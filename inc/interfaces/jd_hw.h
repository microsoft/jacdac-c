// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include "jd_config.h"
#include "jd_physical.h"

extern uint32_t now;

#ifdef JD_DEBUG_MODE
void jd_debug_signal_error(int v);
void jd_debug_signal_write(int v);
void jd_debug_signal_read(int v);
#endif

// io
void led_set(int state);
void led_blink(int us);
void power_pin_enable(int en);

void target_enable_irq(void);
void target_disable_irq(void);
void target_wait_us(uint32_t n);
void target_reset(void);
int target_in_irq(void);

extern uint16_t tim_max_sleep;
extern uint8_t cpu_mhz;
void tim_init(void);
uint64_t tim_get_micros(void);
void tim_set_timer(int delta, cb_t cb);

void uart_init(void);
int uart_start_tx(const void *data, uint32_t numbytes);
void uart_start_rx(void *data, uint32_t maxbytes);
void uart_disable(void);
int uart_wait_high(void);
// make sure data from UART is flushed to RAM (often no-op)
void uart_flush_rx(void);

#if JD_CONFIG_TEMPERATURE == 1
uint16_t adc_read_temp(void);
#endif

uint64_t hw_device_id(void);
void hw_panic(void);

// only required by drivers/*.c
// i2c.c
void i2c_init(void);
// addr are 7bit
int i2c_write_buf(uint8_t addr, const void *src, unsigned len);
int i2c_write_reg_buf(uint8_t addr, uint8_t reg, const void *src, unsigned len);
int i2c_read_buf(uint8_t addr, uint8_t reg, void *dst, unsigned len);
int i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t val);
int i2c_read_reg(uint8_t addr, uint8_t reg);
