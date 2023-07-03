// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_protocol.h"
#include "jd_srvcfg.h"
#include "jd_drivers.h"
#include "services/jd_services.h"

#define LOG(fmt, ...) DMESG("srvcfg: " fmt, ##__VA_ARGS__)
#define VLOG JD_NOLOG

#if JD_DCFG

static uint8_t jd_srvcfg_idx;
static uint8_t jd_srvcfg_idx_map[JD_MAX_SERVICES];

static char *mk_key(unsigned idx, const char *key) {
    if (idx == 0xff)
        return NULL;
    return dcfg_idx_key("", idx, key);
}

char *jd_srvcfg_key(const char *key) {
    return mk_key(jd_srvcfg_idx, key);
}

uint8_t jd_srvcfg_pin(const char *key) {
    return dcfg_get_pin(jd_srvcfg_key(key));
}

int32_t jd_srvcfg_i32(const char *key, int32_t defl) {
    return dcfg_get_i32(jd_srvcfg_key(key), defl);
}

int32_t jd_srvcfg_u32(const char *key, int32_t defl) {
    return dcfg_get_u32(jd_srvcfg_key(key), defl);
}

bool jd_srvcfg_has_flag(const char *key) {
    return jd_srvcfg_i32(key, 0) != 0;
}

typedef struct {
    const char *name;
    void (*cfgfn)(void);
} jd_srvcfg_entry_t;

void rotaryencoder_config(void);
void button_config(void);
void switch_config(void);
void flex_config(void);
void relay_config(void);
void analog_config(void);
void accelerometer_config(void);
void power_config(void);
void lightbulb_config(void);
void buzzer_config(void);
void servo_config(void);
void motion_config(void);
void motor_config(void);
void gamepad_config(void);

static const jd_srvcfg_entry_t jd_srvcfg_entries[] = { //
#if !JD_HOSTED
    {"rotaryEncoder", rotaryencoder_config},
    {"button", button_config},
    {"switch", switch_config},
    {"relay", relay_config},
#if JD_ANALOG
    {"flex", flex_config},
    {"analog", analog_config},
    {"gamepad", gamepad_config},
#endif
    {"accelerometer", accelerometer_config},
    {"power", power_config},
    {"lightBulb", lightbulb_config},
    {"buzzer", buzzer_config},
    {"servo", servo_config},
    {"motion", motion_config},
    {"motor", motor_config},
#if JD_HID
    {"hidMouse", hidmouse_init},
    {"hidKeyboard", hidkeyboard_init},
    {"hidJoystick", hidjoystick_init},
#endif
#endif
    {NULL, NULL}};

uint8_t _jd_services_curr_idx(void);

void jd_srvcfg_run(void) {
    JD_ASSERT(jd_srvcfg_idx == 0);
    memset(jd_srvcfg_idx_map, 0xff, sizeof(jd_srvcfg_idx_map));

    for (;;) {
        const char *srv = dcfg_get_string(jd_srvcfg_key("service"), NULL);
        if (!srv) {
            if (jd_srvcfg_idx < 0x40) {
                // user config starts at 0x40
                jd_srvcfg_idx = 0x40;
                continue;
            } else
                break;
        }
        unsigned namelen = strlen(srv);
        const char *colon = strchr(srv, ':');
        if (colon)
            namelen = colon - srv;
        unsigned i;
        for (i = 0; jd_srvcfg_entries[i].name; ++i) {
            if (memcmp(srv, jd_srvcfg_entries[i].name, namelen) == 0 &&
                jd_srvcfg_entries[i].name[namelen] == 0) {
                DMESG("initialize %s @ %d", srv, jd_srvcfg_idx);
                jd_srvcfg_idx_map[_jd_services_curr_idx()] = jd_srvcfg_idx;
                jd_srvcfg_entries[i].cfgfn();
                break;
            }
        }
        if (!jd_srvcfg_entries[i].name)
            ERROR("service %s:%d not found", srv, jd_srvcfg_idx);
        jd_srvcfg_idx++;
    }

    jd_srvcfg_idx = 0xff;
}

struct srv_state {
    SRV_COMMON;
};

const char *jd_srvcfg_instance_name(srv_t *srv) {
    unsigned idx = jd_srvcfg_idx_map[srv->service_index];
    return dcfg_get_string(mk_key(idx, "name"), NULL);
}

int jd_srvcfg_variant(srv_t *srv) {
    unsigned idx = jd_srvcfg_idx_map[srv->service_index];
    return dcfg_get_i32(mk_key(idx, "variant"), -1);
}

#endif