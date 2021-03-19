// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include "jd_config.h"
#include "jd_service_classes.h"
#include "jd_physical.h"

#define PKT_UNHANDLED 0
#define PKT_HANDLED_RO 1
#define PKT_HANDLED_RW 2

#define JD_REG_PADDING 0xff0
#define JD_REG_END 0xff1
#define JD_REG_SERVICE_DISABLED 0xff2
#define _REG_(tp, v) (((tp) << 12) | (v))
#define _REG_I8 0
#define _REG_U8 1
#define _REG_I16 2
#define _REG_U16 3
#define _REG_I32 4
#define _REG_U32 5
#define _REG_BYTE4 6
#define _REG_BYTE8 7
#define _REG_BIT 8
#define _REG_BYTES 9
#define REG_I8(v) _REG_(_REG_I8, (v))
#define REG_U8(v) _REG_(_REG_U8, (v))
#define REG_I16(v) _REG_(_REG_I16, (v))
#define REG_U16(v) _REG_(_REG_U16, (v))
#define REG_I32(v) _REG_(_REG_I32, (v))
#define REG_U32(v) _REG_(_REG_U32, (v))
#define REG_BYTE4(v) _REG_(_REG_BYTE4, (v))
#define REG_BYTE8(v) _REG_(_REG_BYTE8, (v))
#define REG_BIT(v) _REG_(_REG_BIT, (v))
#define REG_BYTES(v, n) _REG_(_REG_BYTES, (v)), n

#define REG_DEFINITION(name, ...) static const uint16_t name[] = {__VA_ARGS__ JD_REG_END};

typedef struct srv_state srv_t;

typedef void (*srv_pkt_cb_t)(srv_t *state, jd_packet_t *pkt);
typedef void (*srv_cb_t)(srv_t *state);

struct _srv_vt {
    uint32_t service_class;
    uint16_t state_size;
    srv_cb_t process;
    srv_pkt_cb_t handle_pkt;
};
typedef struct _srv_vt srv_vt_t;

#define SRV_COMMON                                                                                 \
    const srv_vt_t *vt;                                                                            \
    uint8_t service_number;                                                                        \
    uint8_t padding0;
#define REG_SRV_COMMON REG_BYTES(JD_REG_PADDING, 6)

struct srv_state_common {
    SRV_COMMON;
};

typedef struct srv_state_common srv_common_t;

extern const char app_dev_class_name[];
extern const char app_fw_version[];

srv_t *jd_allocate_service(const srv_vt_t *vt);

/**
 * Interprets packet as a register read/write, based on REG_DEFINITION() passed as 'sdesc'.
 * It will either read from or write to 'state', depending on register.
 * Returns 0 if the packet was not handled.
 * Returns register code, if the packet was handled as register write of that code.
 * Returns -register code, if the packet was handled as register read of that code.
 */
int service_handle_register(srv_t *state, jd_packet_t *pkt, const uint16_t sdesc[]);

/**
 * called by jd_init();
 **/
void jd_services_init(void);

/**
 * Should be called each time delta by the application
 **/
void jd_services_tick(void);

/**
 * Can be implemented by the user to get a callback on each packet.
 */
void jd_app_handle_packet(jd_packet_t *pkt);

/**
 * Can be implemented by the user to get a callback on each command directed to current device.
 */
void jd_app_handle_command(jd_packet_t *pkt);

/**
 * Should be called by the user, right before jd_services_tick()
 *
 * Unpacks frames into packets, and passes them to
 * jd_services_handle_packet for delivery to services.
 **/
void jd_services_process_frame(jd_frame_t *frame);

/**
 * invoked by jd_services_process_frame.
 *
 * Handles the routing of packets to services.
 **/
void jd_services_handle_packet(jd_packet_t *pkt);

/**
 * Invoked at various points in jacdac-c.
 *
 * Announces services on the bus.
 **/
void jd_services_announce(void);

/**
 * Called by TX queue implementation when a packet is queued for sending.
 */
void jd_services_packet_queued(void);

/**
 * TODO: work out if this is required, or if we can write it out...
 **/
uint32_t app_get_device_class(void);

#define SRV_DEF(id, service_cls)                                                                   \
    static const srv_vt_t id##_vt = {                                                              \
        .service_class = service_cls,                                                              \
        .state_size = sizeof(srv_t),                                                               \
        .process = id##_process,                                                                   \
        .handle_pkt = id##_handle_packet,                                                          \
    }

#define SRV_ALLOC(id)                                                                              \
    srv_t *state = jd_allocate_service(&id##_vt);                                                  \
    (void)state;
