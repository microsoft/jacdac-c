// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include "jd_protocol.h"
#include "jd_pipes.h"
#include "jd_numfmt.h"

typedef struct jdc_client *jdc_t;

#define JDC_STATUS_OK 0
#define JDC_STATUS_UNBOUND -1
#define JDC_STATUS_TIMEOUT -2
#define JDC_STATUS_NOT_IMPL -3
#define JDC_STATUS_OVERFLOW -4

#define JDC_TIMEOUT_IMMEDIATE_ONLY 0
#define JDC_TIMEOUT_DEFAULT 200 // TODO make it less?
#define JDC_TIMEOUT_FOREVER 0xffffffff

#define JDC_WRITE_FLAG_NONE 0x0000
#define JDC_WRITE_FLAG_REQUIRE_ACK 0x0001
#define JDC_READ_FLAG_NONE 0x0000

// no arguments
#define JDC_EV_BOUND 0x0001
#define JDC_EV_UNBOUND 0x0002

// these use the 'pkt' argument; subcode is event/register/action code
#define JDC_EV_REG_VALUE 0x0010
#define JDC_EV_SERVICE_EVENT 0x0011
#define JDC_EV_ACTION_REPORT 0x0012
#define JDC_EV_READING 0x0013 // JDC_EV_REG_VALUE will be fired as well

typedef struct {
    uint16_t code;
    uint16_t subcode;
    jd_packet_t *pkt;
} jdc_event_t;

typedef void (*jdc_event_cb_t)(jdc_t client, jdc_event_t *ev);

//
// These calls are synchronous (they return quickly).
//

// event_cb can be set to NULL to disable
jdc_t jdc_create(uint32_t service_class, const char *name, jdc_event_cb_t event_cb);
// creates a separate client, that can be configured differently and used independently
jdc_t jdc_create_derived(jdc_t parent, jdc_event_cb_t event_cb);

// default: JDC_READ_FLAG_NONE, JDC_TIMEOUT_DEFAULT
void jdc_configure_read(jdc_t c, unsigned flags, unsigned timeout_ms);
// the timeout is significant when ACK is requested or when outgoing queue is full
// default: JDC_WRITE_FLAG_NONE, JDC_TIMEOUT_DEFAULT
void jdc_configure_write(jdc_t c, unsigned flags, unsigned timeout_ms);

// when enabled, streaming_samples will be set periodically
void jdc_set_streaming(jdc_t c, bool enabled);

void jdc_set_userdata(jdc_t c, void *userdata);
void *jdc_get_userdata(jdc_t c);

void jdc_destroy(jdc_t c);

int jdc_get_binding_info(jdc_t c, uint64_t *device_id, uint8_t *service_index);
const char *jdc_get_name(jdc_t c);
uint32_t jdc_get_service_class(jdc_t c);
bool jdc_is_bound(jdc_t c);

//
// This calls can block
//

int jdc_get_register(jdc_t c, uint16_t regcode, void *dst, unsigned size, unsigned cache_policy);
int jdc_get_register_float(jdc_t c, uint16_t regcode, jd_float_t *dst, unsigned numfmt, unsigned cache_policy);
int jdc_set_register(jdc_t c, uint16_t regcode, const void *payload, unsigned size);
int jdc_run_action(jdc_t c, uint16_t action_code, const void *payload, unsigned size);

//
// Globals
//

// Start the Jacdac processing thread, start the callback thread, and run init_cb() there.
// init_cb defaults to jdc_wait_roles_bound(1000) if NULL.
void jdc_start_threads(void (*init_cb)(void));

// Wait until all roles are bound or timeout expires.
void jdc_wait_roles_bound(unsigned timeout_ms);

typedef void (*jdc_generic_cb_t)(void *userdata);

// Schedule code on the callback thread (at the end of event queue).
// Can be only called after jdc_start_threads(). ???
//
void jdc_run(jdc_generic_cb_t cb, void *userdata);
void jdc_run_and_wait(jdc_generic_cb_t cb, void *userdata);

// Used by RTOS-port.
// Run event handlers for all pending events on hclients.
// If there are no events, it will wait up to timeout_ms for events to arrive
// - after a single event arriving, it will run its handlers and return.
// The timeout only applies to waiting for events, the handlers can take longer to execute.
// Returns number of event handlers run.
// Return value of 0 implies timeout_ms elapsed without any incoming events.
int jdc_process_events(unsigned timeout_ms);

#if 0
// generated client code will look something like this
typedef struct {
    jdc_t jdc;
} jdc_temperature_t;

static jdc_temperature_t jdc_temperature_create(const char *name, jdc_event_cb_t *cb) {
    jdc_temperature_t r = {
        jdc_create(JD_SERVICE_CLASS_TEMPERATURE, name ? name : "temperature", cb)};
    return r;
}

static inline int jdc_temperature_set_streaming_interval(jdc_temperature_t c, uint32_t ms) {
    return jdc_set_register(c.jdc, JD_REG_STREAMING_INTERVAL, &ms, sizeof(ms));
}

static inline int jdc_temperature_get_reading(jdc_temperature_t c, int32_t *res) {
    return jdc_get_register(c.jdc, JD_TEMPERATURE_REG_TEMPERATURE, res, sizeof(res));
}

static inline int jdc_temperature_get_reading_f(jdc_temperature_t c, jd_float_t *res) {
    return jdc_get_register_float(c.jdc, JD_TEMPERATURE_REG_TEMPERATURE, res, JD_NUMFMT(I32, 10));
}

static inline int jdc_temperature_get_variant(jdc_temperature_t c, uint8_t *res) {
    return jdc_get_register(c.jdc, JD_TEMPERATURE_REG_VARIANT, res, sizeof(*res));
}

static inline int jdc_relay_set_active(jdc_relay_t c, jd_bool_t value) {
    return jdc_set_register(c.jdc, JD_RELAY_REG_ACTIVE, &value, sizeof(value));
}

static inline int jdc_accelerometer_get_forces(jdc_accelerometer_t c,
                                               jd_accelerometer_forces_t *forces) {
    return jdc_get_register(...);
}

static int jdc_accelerometer_get_forces_f(jdc_accelerometer_t c,
                                          jd_accelerometer_forces_f_t *forces) {
    jd_accelerometer_forces_t tmp;
    int r = jdc_get_register(... & tmp);
    if (r == 0) {
        forces->x = jd_to_float(&tmp.x, JD_NUMFMT(I32, 20));
        forces->y = jd_to_float(&tmp.y, JD_NUMFMT(I32, 20));
        forces->z = jd_to_float(&tmp.z, JD_NUMFMT(I32, 20));
    }
    return r;
}

// sample usage:
jdc_temperature_t temp;
jdc_relay_t heater;
jdc_rotary_encoder_t setting_wheel;
jdc_button_t btn_up, btn_down;

jd_float_t setpoint;

static void temp_cb(jdc_t c, jdc_event_t *ev) {
    if (ev->code == JDC_EV_READING) {
        jd_float_t f;
        if (jdc_temperature_get_reading_f(temp, &f) == 0) {
            // TODO avoid jitter!
            jdc_relay_set_active(heater, f < setpoint);
        }
    }
}

static void rotary_cb(jdc_t c, jdc_event_t *ev) {
    static int32_t last_setting_wheel;
    static bool inited;

    if (ev->code == JDC_EV_BOUND) {
        // initial setup
        inited = false;
        jdc_set_streaming(c, true);
    } else if (ev->code == JDC_EV_READING) {
        int32_t r;
        if (jdc_rotary_encoder_get_reading(setting_wheel, &r) == 0) {
            if (!inited) {
                last_setting_wheel = r;
                inited = true;
            }
            int delta = r - last_setting_wheel;
            if (delta) {
                last_setting_wheel = r;
                setpoint += delta * 0.25f;
            }
        }
    }
}

static void btn_cb(jdc_t c, jdc_event_t *ev) {
    if (ev->code == JDC_EV_SERVICE_EVENT && ev->subcode == JD_BUTTON_EV_DOWN) {
        int delta = (int32_t)(intptr_t)jdc_get_userdata(c);
        setpoint += delta;
    }
}

static void init_roles() {
    temp = jdc_temperature_create("temp", temp_cb);
    jdc_set_streaming(temp, true);
    heater = jdc_relay_create("heater", NULL);
    setting_wheel = jdc_rotary_encoder_create("setting_wheel", rotary_cb);
    btn_up = jdc_button_create("up", btn_cb);
    jdc_set_userdata(btn_up, (void *)(+10));
    btn_down = jdc_button_create("down", btn_cb);
    jdc_set_userdata(btn_down, (void *)(-10));
}

void sample1() {
    // ...
    jdc_run((jdc_generic_cb_t)init_roles, NULL);
}

void sample2() {
    init_roles();
    jdc_start_threads(NULL);
}

#endif
