#pragma once

// io
void log_pin_set(int line, int v);
void led_set(int state);
void led_blink(int us);
void power_pin_enable(int en);

void jd_panic(void);
void target_enable_irq(void);
void target_disable_irq(void);
void target_wait_us(uint32_t n);

void tim_init(void);
uint64_t tim_get_micros(void);
void tim_set_timer(int delta, cb_t cb);

void uart_init(void);
int uart_start_tx(const void *data, uint32_t numbytes);
void uart_start_rx(void *data, uint32_t maxbytes);
void uart_disable(void);
int uart_wait_high(void);