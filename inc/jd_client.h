// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include "jd_protocol.h"

#define JD_CLIENT_EV_DEVICE_CREATED 0x0001
#define JD_CLIENT_EV_DEVICE_DESTROYED 0x0002
#define JD_CLIENT_EV_DEVICE_RESET 0x0003
#define JD_CLIENT_EV_SERVICE_PACKET 0x0004
#define JD_CLIENT_EV_NON_SERVICE_PACKET 0x0005
#define JD_CLIENT_EV_BROADCAST_PACKET 0x0006

typedef struct jd_device_service {
    uint32_t service_class;
    uint8_t service_index;
    uint8_t flags;
    uint16_t userflags;
    void *userdata;
} jd_device_service_t;

typedef struct jd_register_query {
    struct jd_register_query *next;
    uint16_t reg_code;
    uint8_t service_index;
    uint8_t resp_size;
    union {
        uint32_t u32;
        uint16_t u16;
        uint8_t u8;
        int32_t i32;
        int16_t i16;
        int8_t i8;
        uint8_t data[4];
        uint8_t *buffer;
    } value;
} jd_register_query_t;

typedef struct jd_device {
    struct jd_device *next;
    uint64_t device_identifier;
    uint8_t num_services;
    uint16_t announce_flags;
    char short_id[5];
    uint32_t _expires;
    void *userdata;
    jd_device_service_t services[0];
} jd_device_t;

extern jd_device_t *jd_devices;

void jd_client_process(void);
void jd_client_handle_packet(jd_packet_t *pkt);

void jd_client_log_event(int event_id, void *arg0, void *arg1);
void jd_client_emit_event(int event_id, void *arg0, void *arg1);

int jd_send_pkt(jd_packet_t *pkt);

// jd_device_t methods
jd_device_t *jd_device_lookup(uint64_t device_identifier);
void jd_device_short_id(char short_id[5], uint64_t long_id);

// jd_device_service_t  methods
static inline jd_device_t *jd_service_parent(jd_device_service_t *serv) {
    return (jd_device_t *)((uint8_t *)(serv - serv->service_index) - sizeof(jd_device_t));
}
int jd_service_send_cmd(jd_device_service_t *serv, uint16_t service_command, const void *data,
                        size_t datasize);
