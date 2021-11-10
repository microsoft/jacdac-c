// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include "jd_protocol.h"

#define JD_CLIENT_EV_DEVICE_CREATED 0x0001
#define JD_CLIENT_EV_DEVICE_DESTROYED 0x0002
#define JD_CLIENT_EV_DEVICE_RESET 0x0003
#define JD_CLIENT_EV_DEVICE_PACKET 0x0004
#define JD_CLIENT_EV_NON_DEVICE_PACKET 0x0005
#define JD_CLIENT_EV_BROADCAST_PACKET 0x0006

typedef struct jd_device {
    struct jd_device *next;
    uint64_t device_identifier;
    uint8_t num_services;
    char short_id[5];
    uint32_t _expires;
    void *userdata;

    uint32_t services[0];
} jd_device_t;

extern jd_device_t *jd_devices;

void jd_client_process(void);
void jd_client_handle_packet(jd_packet_t *pkt);
jd_device_t *jd_device_lookup(uint64_t device_identifier);
void jd_device_short_id(char short_id[5], uint64_t long_id);
void jd_client_log_event(int event_id, void *arg0, void *arg1);
void jd_client_emit_event(int event_id, void *arg0, void *arg1);
