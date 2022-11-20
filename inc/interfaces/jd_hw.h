// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef JD_HW_H
#define JD_HW_H

/*
 * Hardware abstraction layer.
 */

#include "jd_config.h"
#include "jd_physical.h"

uint64_t hw_device_id(void);
__attribute((noreturn)) void hw_panic(void);

extern uint32_t now;

#if JD_MS_TIMER
extern uint32_t now_ms;
extern uint64_t now_ms_long;
#endif

#ifndef BL

#if JD_SIGNAL_RDWR
void jd_debug_signal_write(int v);
void jd_debug_signal_read(int v);
#else
#define jd_debug_signal_write(v) ((void)0)
#define jd_debug_signal_read(v) ((void)0)
#endif

// io
void power_pin_enable(int en);

void target_enable_irq(void);
void target_disable_irq(void);
void target_wait_us(uint32_t n);
__attribute__((noreturn)) void target_reset(void);
int target_in_irq(void);

extern uint16_t tim_max_sleep;
extern uint8_t cpu_mhz;
void tim_init(void);
uint64_t tim_get_micros(void);
void tim_set_timer(int delta, cb_t cb);

void uart_init_(void);
int uart_start_tx(const void *data, uint32_t numbytes);
void uart_start_rx(void *data, uint32_t maxbytes);
void uart_disable(void);
int uart_wait_high(void);
// make sure data from UART is flushed to RAM (often no-op)
void uart_flush_rx(void);

#if JD_CONFIG_TEMPERATURE == 1
uint16_t adc_read_temp(void);
#endif

// Things below are only required by drivers/*.c

// i2c.c
// addr are always 7bit
int i2c_init(void);
int i2c_setup_write(uint8_t addr, unsigned len, bool repeated);
int i2c_write(uint8_t c);
int i2c_finish_write(bool repeated);

// utilities, 8-bit register addresses
int i2c_write_reg_buf(uint8_t addr, uint8_t reg, const void *src, unsigned len);
int i2c_read_reg_buf(uint8_t addr, uint8_t reg, void *dst, unsigned len);
int i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t val);
int i2c_read_reg(uint8_t addr, uint8_t reg);

// 16-bit reg addresses
int i2c_write_reg16_buf(uint8_t addr, uint16_t reg, const void *src, unsigned len);
int i2c_read_reg16_buf(uint8_t addr, uint16_t reg, void *dst, unsigned len);
// note that this still reads/writes a single byte, from a 16 bit address
int i2c_write_reg16(uint8_t addr, uint16_t reg, uint8_t val);
int i2c_read_reg16(uint8_t addr, uint16_t reg);

// mbed-style
int i2c_write_ex(uint8_t addr, const void *src, unsigned len, bool repeated);
int i2c_read_ex(uint8_t addr, void *dst, unsigned len);

// bitbang_spi.c
void bspi_send(const void *src, uint32_t len);
void bspi_recv(void *dst, uint32_t len);
void bspi_init(void);

// dspi.c
void dspi_init(bool slow, int cpol, int cpha);
void dspi_tx(const void *data, uint32_t numbytes, cb_t doneHandler);
void dspi_xfer(const void *txdata, void *rxdata, uint32_t numbytes, cb_t doneHandler);

// sspic.c
void sspi_init(int slow, int cpol, int cpha);
void sspi_tx(const void *data, uint32_t numbytes);
void sspi_rx(void *buf, uint32_t numbytes);

// onewire.c
void one_init(void);
int one_reset(void);
void one_write(uint8_t b);
uint8_t one_read(void);

void spiflash_init(void);
uint64_t spiflash_unique_id(void);
uint32_t spiflash_num_bytes(void);
void spiflash_dump_sfdp(void);
void spiflash_read_bytes(uint32_t addr, void *buffer, uint32_t len);
void spiflash_write_bytes(uint32_t addr, const void *buffer, uint32_t len);
void spiflash_erase_4k(uint32_t addr);
void spiflash_erase_32k(uint32_t addr);
void spiflash_erase_64k(uint32_t addr);
void spiflash_erase_chip(void);

// SPI bit-bang driver
void spi_bb_init(void);
void spi_bb_rx(void *data, unsigned len);
void spi_bb_tx(const void *data, unsigned len);
static inline uint8_t spi_bb_byte(void) {
    uint8_t b;
    spi_bb_rx(&b, 1);
    return b;
}

#endif

#endif
