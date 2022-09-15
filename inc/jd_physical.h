// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef __JDPROTOCOL_H
#define __JDPROTOCOL_H

#include "jd_config.h"

#ifdef __cplusplus
extern "C" {
#endif

// 255 minus size of the serial header, rounded down to 4
#define JD_SERIAL_PAYLOAD_SIZE 236
#define JD_SERIAL_FULL_HEADER_SIZE 16
// the COMMAND flag signifies that the device_identifier is the recipient
// (i.e., it's a command for the peripheral); the bit clear means device_identifier is the source
// (i.e., it's a report from peripheral or a broadcast message)
#define JD_FRAME_FLAG_COMMAND 0x01
// an ACK should be issued with CRC of this frame upon reception
#define JD_FRAME_FLAG_ACK_REQUESTED 0x02
// the device_identifier contains target service class number
#define JD_FRAME_FLAG_IDENTIFIER_IS_SERVICE_CLASS 0x04
#define JD_FRAME_FLAG_BROADCAST 0x04
// set on frames not received from the JD-wire
#define JD_FRAME_FLAG_LOOPBACK 0x40
// when set, the packet may have different layout and should be dropped
#define JD_FRAME_FLAG_VNEXT 0x80

#define JD_FRAME_SIZE(pkt) ((pkt)->size + 12)

#define JD_SERVICE_INDEX_CONTROL 0x00
#define JD_SERVICE_INDEX_MASK 0x3f
#define JD_SERVICE_INDEX_CRC_ACK 0x3f
#define JD_SERVICE_INDEX_STREAM 0x3e
#define JD_SERVICE_INDEX_BROADCAST 0x3d
#define JD_SERVICE_INDEX_MAX_NORMAL 0x30

#define JD_DEVICE_IDENTIFIER_BROADCAST_MARK 0xAAAAAAAA00000000

#define JD_PIPE_COUNTER_MASK 0x001f
#define JD_PIPE_CLOSE_MASK 0x0020
#define JD_PIPE_METADATA_MASK 0x0040
#define JD_PIPE_PORT_SHIFT 7

#define JD_CMD_EVENT_MASK 0x8000
#define JD_CMD_EVENT_CODE_MASK 0xff
#define JD_CMD_EVENT_COUNTER_MASK 0x7f
#define JD_CMD_EVENT_COUNTER_SHIFT 8
#define JD_CMD_EVENT_MK(counter, code)                                                             \
    (JD_CMD_EVENT_MASK | ((((counter)&JD_CMD_EVENT_COUNTER_MASK) << JD_CMD_EVENT_COUNTER_SHIFT)) | \
     (code & JD_CMD_EVENT_CODE_MASK))

#define JDSPI_MAGIC 0x7ACD
#define JDSPI_MAGIC_NOOP 0xB3CD

typedef void (*cb_t)(void);
typedef int (*intfn_t)(void *);

struct _jd_packet_t {
    uint16_t crc;
    uint8_t _size; // of frame data[]
    uint8_t flags;

    uint64_t device_identifier;

    uint8_t service_size;
    uint8_t service_index;
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

struct _jd_pipe_cmd_t {
    uint64_t device_identifier;
    uint16_t port_num;
    uint16_t reserved;
} __attribute__((__packed__, aligned(4)));
typedef struct _jd_pipe_cmd_t jd_pipe_cmd_t;

void jd_packet_ready(void);
void jd_compute_crc(jd_frame_t *frame);
bool jd_frame_crc_ok(jd_frame_t *frame);
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
