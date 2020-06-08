#ifndef __JDPROTOCOL_H
#define __JDPROTOCOL_H

#include "jd_config.h"

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


void jd_init(void);
void jd_packet_ready(void);
void jd_compute_crc(jd_frame_t *frame);
// these are to be called by uart implementation
void jd_tx_completed(int errCode);
void jd_rx_completed(int dataLeft);
void jd_line_falling(void);
int jd_is_running(void);
int jd_is_busy(void);

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
