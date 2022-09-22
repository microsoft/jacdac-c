// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef JD_SERVICE_FRAMEWORK_H
#define JD_SERVICE_FRAMEWORK_H

#include "jd_config.h"
#include "jd_service_classes.h"
#include "jd_physical.h"

#define PKT_UNHANDLED 0
#define PKT_HANDLED_RO 1
#define PKT_HANDLED_RW 2

#define JD_REG_PADDING 0xff0
#define JD_REG_END 0xff1
#define JD_REG_SERVICE_DISABLED 0xff2
#define _JD_REG_(tp, v) (((tp) << 12) | (v))
#define _JD_REG_I8 0
#define _JD_REG_U8 1
#define _JD_REG_I16 2
#define _JD_REG_U16 3
#define _JD_REG_I32 4
#define _JD_REG_U32 5
#define _JD_REG_BYTE4 6
#define _JD_REG_BYTE8 7
#define _JD_REG_BIT 8
#define _JD_REG_BYTES 9
#define _JD_REG_OPT8 10
#define _JD_REG_OPT16 11
#define _JD_REG_OPT32 12
#define _JD_REG_PTR 13
#define REG_I8(v) _JD_REG_(_JD_REG_I8, (v))
#define REG_U8(v) _JD_REG_(_JD_REG_U8, (v))
#define REG_I16(v) _JD_REG_(_JD_REG_I16, (v))
#define REG_U16(v) _JD_REG_(_JD_REG_U16, (v))
#define REG_I32(v) _JD_REG_(_JD_REG_I32, (v))
#define REG_U32(v) _JD_REG_(_JD_REG_U32, (v))
#define REG_OPT8(v) _JD_REG_(_JD_REG_OPT8, (v))
#define REG_OPT16(v) _JD_REG_(_JD_REG_OPT16, (v))
#define REG_OPT32(v) _JD_REG_(_JD_REG_OPT32, (v))
#define REG_BYTE4(v) _JD_REG_(_JD_REG_BYTE4, (v))
#define REG_BYTE8(v) _JD_REG_(_JD_REG_BYTE8, (v))
#define REG_BIT(v) _JD_REG_(_JD_REG_BIT, (v))
#define REG_BYTES(v, n) _JD_REG_(_JD_REG_BYTES, (v)), n
#if JD_64
#define REG_PTR_PADDING() _JD_REG_(_JD_REG_PTR, JD_REG_PADDING)
#else
#define REG_PTR_PADDING() REG_U32(JD_REG_PADDING)
#endif

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
    uint8_t service_index;                                                                         \
    uint8_t srv_flags;
#define REG_SRV_COMMON REG_BYTES(JD_REG_PADDING, JD_PTRSIZE + 2)

struct srv_state_common {
    SRV_COMMON;
};

typedef struct srv_state_common srv_common_t;

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
 * Like `service_handle_register()` but calls `jd_send_not_implemented()` for unknown packets.
 */
int service_handle_register_final(srv_t *state, jd_packet_t *pkt, const uint16_t sdesc[]);

/**
 * If `pkt` is `JD_GET(reg_code)` send `value` as response and return 1.
 */
int service_handle_string_register(jd_packet_t *pkt, uint16_t reg_code, const char *value);

/**
 * Respond to variant query.
 */
int service_handle_variant(jd_packet_t *pkt, uint8_t variant);

/**
 * called by jd_init();
 **/
void jd_services_init(void);

/**
 * De-allocates all services. This is not normally used, except when tracing memory leaks.
 */
void jd_services_deinit(void);

/**
 * Called by jd_process_everything()
 **/
void jd_services_tick(void);

/**
 * Should be called in a loop from the main application.
 * This may also be called recursively if sensor implementation decides to sleep.
 */
void jd_process_everything(void);

/**
 * Refresh 'now' (and 'now_ms' if configured).
 */
void jd_refresh_now(void);

/**
 * Can be implemented by the user to get a callback on each packet.
 */
void jd_app_handle_packet(jd_packet_t *pkt);

/**
 * Can be implemented by the user to get a callback on each command directed to current device.
 */
void jd_app_handle_command(jd_packet_t *pkt);

/**
 * Called by jd_process_everything().
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
 * Do a quick check whether jd_services_process_frame() would possibly do anything.
 */
bool jd_services_needs_frame(jd_frame_t *frame);

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
 * This can be only invoked from service process callback.
 * It will continue calling jd_process_everything(), excluding any service
 * process callbacks that are currently sleeping.
 * This will also prevent the MCU from sleeping.
 * Be mindful of stack usage.
 */
void jd_services_sleep_us(uint32_t delta);

uint32_t app_get_device_class(void);
const char *app_get_fw_version(void);
const char *app_get_dev_class_name(void);

#define SRV_DEF_SZ(id, service_cls, sz)                                                            \
    static const srv_vt_t id##_vt = {                                                              \
        .service_class = service_cls,                                                              \
        .state_size = sz,                                                                          \
        .process = id##_process,                                                                   \
        .handle_pkt = id##_handle_packet,                                                          \
    }

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

#endif
