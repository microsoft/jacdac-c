#ifndef __JDPROTOCOL_H
#define __JDPROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define JDSPI_MAGIC 0x7ACD
#define JDSPI_MAGIC_NOOP 0xB3CD

#ifndef JD_TIM_OVERHEAD
#define JD_TIM_OVERHEAD 12
#endif

// this is timing overhead (in us) of starting transmission
// see set_tick_timer() for how to calibrate this
#ifndef JD_WR_OVERHEAD
#define JD_WR_OVERHEAD 8
#endif

typedef void (*cb_t)(void);

struct _jd_packet_t {
    uint16_t crc;
    uint8_t _size; // of frame data[]
    uint8_t flags;

    uint64_t device_identifier;

    uint8_t service_size;
    uint8_t service_number;
    uint16_t service_command;

    uint8_t data[0];
} __attribute__((__packed__, aligned(4)));

typedef struct _jd_packet_t jd_packet_t;

struct _jd_frame_t {
    uint16_t crc;
    uint8_t size;
    uint8_t flags;

    uint64_t device_identifier;

    uint8_t data[JD_SERIAL_PAYLOAD_SIZE + 4];
} __attribute__((__packed__, aligned(4)));
typedef struct _jd_frame_t jd_frame_t;

// Required by jdlow.c
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

void log_pin_set(int line, int v);

// the protocol is:
// app calls jd_packet_ready()
// JD stack calls app_pull_frame() when it's ready to send
// JS stack calls app_frame_sent() when the send is done
jd_frame_t *app_pull_frame(void);
void app_frame_sent(jd_frame_t *frame);
void app_queue_annouce(void);
int app_handle_frame(jd_frame_t *frame);

// Provided jdutil.c
uint32_t jd_random_around(uint32_t v);
uint32_t jd_random(void);
void jd_seed_random(uint32_t s);
uint32_t jd_hash_fnv1a(const void *data, unsigned len);
uint16_t jd_crc16(const void *data, uint32_t size);

// Provided jdlow.c
void jd_init(void);
void jd_packet_ready(void);
void jd_compute_crc(jd_frame_t *frame);
// these are to be called by uart implementation
void jd_tx_completed(int errCode);
void jd_rx_completed(int dataLeft);
void jd_line_falling(void);
int jd_is_running(void);
int jd_is_busy(void);
int jd_shift_frame(jd_frame_t *frame);
void jd_reset_frame(jd_frame_t *frame);
void *jd_push_in_frame(jd_frame_t *frame, unsigned service_num, unsigned service_cmd,
                       unsigned service_size);

typedef struct {
    uint32_t bus_state;
    uint32_t bus_lo_error;
    uint32_t bus_uart_error;
    uint32_t bus_timeout_error;
    uint32_t packets_sent;
    uint32_t packets_received;
    uint32_t packets_dropped;
} jd_diagnostics_t;
jd_diagnostics_t *jd_get_diagnostics(void);

#ifdef __cplusplus
}
#endif

#endif
