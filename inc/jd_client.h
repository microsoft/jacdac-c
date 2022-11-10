// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

//
// Low-level, event-based, async, single-threaded client interface.
//

#pragma once

#include "jd_protocol.h"
#include "jd_pipes.h"
#include "jacdac/dist/c/rolemanager.h"

// An announce packet was first spotted and a device was created (jd_device_t, jd_packet_t)
#define JD_CLIENT_EV_DEVICE_CREATED 0x0001
// We have not seen an announce for 2s, so device was garbage collected (jd_device_t, NULL)
// (or the device was reset, which triggers destroy and create)
#define JD_CLIENT_EV_DEVICE_DESTROYED 0x0002
// An announce packet with lower reset count was spotted (jd_device_t, jd_packet_t)
// This event is always followed by DEVICE_DESTROYED and DEVICE_CREATED
#define JD_CLIENT_EV_DEVICE_RESET 0x0003

// A regular packet for named service was received (jd_device_service_t, jd_packet_t)
#define JD_CLIENT_EV_SERVICE_PACKET 0x0010
// A non-regular packet (CRC-ACK, pipe, etc) was received (jd_device_t?, jd_packet_t)
// This can also happen if packet is received before announce, in which case first argument is NULL.
#define JD_CLIENT_EV_NON_SERVICE_PACKET 0x0011
// A broadcast (JD_FRAME_FLAG_IDENTIFIER_IS_SERVICE_CLASS) packet was received (NULL, jd_packet_t)
#define JD_CLIENT_EV_BROADCAST_PACKET 0x0012
// Additional copy of an event was received (jd_device_service_t, jd_packet_t)
// This should most likely be ignored; the first copy is sent as regular
// JD_CLIENT_EV_SERVICE_PACKET.
#define JD_CLIENT_EV_REPEATED_EVENT_PACKET 0x0013

// A value of register was first received, or has changed (jd_device_service_t, jd_register_query_t)
#define JD_CLIENT_EV_SERVICE_REGISTER_CHANGED 0x0020
// Register was marked as not implemented (jd_device_service_t, jd_register_query_t)
#define JD_CLIENT_EV_SERVICE_REGISTER_NOT_IMPLEMENTED 0x0021

// Emitted on every jd_client_process() (roughly every 10ms), no arguments
#define JD_CLIENT_EV_PROCESS 0x0030

// A role assignment has changed (jd_device_service_t?, jd_role_t)
#define JD_CLIENT_EV_ROLE_CHANGED 0x0040

typedef struct jd_device_service {
    uint32_t service_class;
    uint8_t service_index;
    uint8_t flags;
    uint16_t userflags;
    void *userdata;
} jd_device_service_t;

#define JD_DEVICE_SERVICE_FLAG_ROLE_ASSIGNED 0x01

#define JD_REGISTER_QUERY_MAX_INLINE 4
typedef struct jd_register_query {
    struct jd_register_query *next;
    uint16_t reg_code;
    uint8_t service_index;
    uint8_t resp_size;
    uint32_t last_query_ms;
    union {
        uint32_t u32;
        uint8_t data[JD_REGISTER_QUERY_MAX_INLINE];
        uint8_t *buffer;
    } value;
} jd_register_query_t;

static inline bool jd_register_not_implemented(const jd_register_query_t *q) {
    return (q->service_index & 0x80) != 0;
}

static inline const void *jd_register_data(const jd_register_query_t *q) {
    return q->resp_size > JD_REGISTER_QUERY_MAX_INLINE ? q->value.buffer : q->value.data;
}

typedef struct jd_device {
    struct jd_device *next;
    jd_register_query_t *_queries;
    uint64_t device_identifier;
    uint8_t num_services;
    uint8_t _event_counter;
    uint16_t announce_flags;
    char short_id[5];
    uint32_t _expires;
    void *userdata;
    jd_device_service_t services[0];
} jd_device_t;

extern jd_device_t *jd_devices;

void jd_pkt_setup_broadcast(jd_packet_t *dst, uint32_t service_class, uint16_t service_command);

void jd_client_process(void);
void jd_client_handle_packet(jd_packet_t *pkt);

void jd_client_log_event(int event_id, void *arg0, void *arg1);

typedef void (*jd_client_event_handler_t)(void *userdata, int event_id, void *arg0, void *arg1);
void jd_client_subscribe(jd_client_event_handler_t handler, void *userdata);

// jd_device_t methods
jd_device_t *jd_device_lookup(uint64_t device_identifier);
static inline jd_device_service_t *jd_device_get_service(jd_device_t *dev, unsigned serv_idx) {
    return dev && serv_idx < dev->num_services ? dev->services + serv_idx : NULL;
}
jd_device_service_t *jd_device_lookup_service(jd_device_t *dev, uint32_t service_class);

// jd_device_service_t  methods
static inline jd_device_t *jd_service_parent(jd_device_service_t *serv) {
    return (jd_device_t *)((uint8_t *)(serv - serv->service_index) -
                           offsetof(jd_device_t, services));
}
int jd_service_send_cmd(jd_device_service_t *serv, uint16_t service_command, const void *data,
                        size_t datasize);
// these are only valid until next event loop process
const jd_register_query_t *jd_service_query(jd_device_service_t *serv, int reg_code,
                                            int refresh_ms);
void jd_device_clear_queries(jd_device_t *d, uint8_t service_idx);

// role manager
typedef struct jd_role {
    struct jd_role *_next;
    const char *name;
    uint32_t service_class;
    uint32_t hidden : 1;
    jd_device_service_t *service;
} jd_role_t;

// name must be kept alive until jd_role_free()
jd_role_t *jd_role_alloc(const char *name, uint32_t service_class);
void jd_role_free(jd_role_t *role);
void jd_role_free_all(void);
void jd_role_force_autobind(void);
// both jd_role_alloc() and jd_role_free*() generate JD_CLIENT_EV_ROLE_CHANGED (now)
// a freshly created role will typically be bound on next auto-bind
jd_role_t *jd_role_by_service(jd_device_service_t *serv);

unsigned rolemgr_serialized_role_size(jd_role_t *r);
jd_role_manager_roles_t *rolemgr_serialize_role(jd_role_t *r);

// call from app_init_services()
void jd_role_manager_init(void);
