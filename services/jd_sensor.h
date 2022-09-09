// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include "jd_protocol.h"

typedef void *(*get_reading_t)(void);

typedef struct {
    void (*init)(void);
    void (*process)(void);
    void (*sleep)(void);
    get_reading_t get_reading;
    // only present for some sensors:
    uint32_t (*conditioning_period)(void);                     // for eco2 and tvoc
    void (*set_temp_humidity)(int32_t temp, int32_t humidity); // temp/hum compensation
    int32_t (*get_range)(void);
    int32_t (*set_range)(int32_t range); // returns the range after setting
    const struct sensor_range *ranges;
    bool (*is_present)(void);
} sensor_api_t;

// make sure 'api' is at 8 byte boundary to avoid alignment issues on 64 bit arch
#define SENSOR_COMMON                                                                              \
    SRV_COMMON;                                                                                    \
    uint8_t streaming_samples;                                                                     \
    uint8_t got_query : 1;                                                                         \
    uint8_t jd_inited : 1;                                                                         \
    uint8_t got_reading : 1;                                                                       \
    uint8_t reading_pending : 1;                                                                   \
    uint32_t streaming_interval;                                                                   \
    const sensor_api_t *api;                                                                       \
    uint32_t next_streaming

#define REG_SENSOR_COMMON REG_BYTES(JD_REG_PADDING, JD_PTRSIZE + 2 + 2 + 4 + JD_PTRSIZE + 4)

int sensor_handle_packet(srv_t *state, jd_packet_t *pkt);
int sensor_should_stream(srv_t *state);
int sensor_handle_packet_simple(srv_t *state, jd_packet_t *pkt, const void *sample,
                                uint32_t sample_size);
int sensor_handle_packet_simple_variant(srv_t *state, jd_packet_t *pkt, const void *sample,
                                        uint32_t sample_size, uint8_t variant);
void sensor_process_simple(srv_t *state, const void *sample, uint32_t sample_size);

void sensor_process(srv_t *state);
void sensor_send_status(srv_t *state);
void *sensor_get_reading(srv_t *state);
bool sensor_maybe_init(srv_t *state);
bool sensor_should_send_threshold_event(uint32_t *block, uint32_t debounce_ms, bool cond_ok);

// sync layout changes with env_sensor_handle_packet()
typedef struct {
    int32_t value;
    int32_t error;
    int32_t min_value;
    int32_t max_value;
} env_reading_t;

int env_sensor_handle_packet(srv_t *state, jd_packet_t *pkt);
void env_sensor_process(srv_t *state);

#define SCALE_TEMP(x) (int)((x)*1024.0 + 0.5)
#define SCALE_HUM(x) (int)((x)*1024.0 + 0.5)
#define SCALE_PRESSURE(x) (int)((x)*1024.0 + 0.5)
#define SCALE_TVOC(x) (int)((x)*1024.0 + 0.5)
#define SCALE_CO2(x) (int)((x)*1024.0 + 0.5)

#define SCALE_LUX(x) (int)((x)*1024.0 + 0.5)
#define SCALE_UVI(x) (int)((x)*16 * 1024.0 + 0.5)

#define ERR_HUM(a, b) SCALE_HUM(a), SCALE_HUM(b)
#define ERR_TEMP(a, b) SCALE_TEMP(a), SCALE_TEMP(b)
#define ERR_PRESSURE(a, b) SCALE_PRESSURE(a), SCALE_PRESSURE(b)
#define ERR_END -1, -1

int32_t env_extrapolate_error(int32_t value, const int32_t *error_table);
void env_set_value(env_reading_t *env, int32_t value, const int32_t *error_table);

// relative->absolute humidity conversion;
// all args are i22.10; temp is C, humidity %, result g/m3
int32_t env_absolute_humidity(int32_t temp, int32_t humidity);

// sensor ranges

typedef struct sensor_range {
    int32_t range; // same unit as readings

    // sensor-dependant
    uint32_t config;
    uint32_t scale;
} sensor_range_t;

// returns range closest to requested one; `ranges` is {0,0,0}-terminated and sorted ascending
const sensor_range_t *sensor_lookup_range(const sensor_range_t *ranges, int32_t requested);

// generic 16 bit analog sensor
typedef struct analog_config {
    uint8_t pinH, pinL, pinM;
    uint8_t variant;
    int32_t offset;
    int32_t scale;
    // all values in ms
    uint32_t sampling_ms;        // how often to probe the sensor (default 9ms)
    uint32_t sampling_delay;     // after enabling pinH/pinL how long to wait (default 0)
    uint32_t streaming_interval; // defaults to 100ms
} analog_config_t;

#define ANALOG_SENSOR_STATE                                                                        \
    SENSOR_COMMON;                                                                                 \
    const analog_config_t *config;                                                                 \
    uint16_t sample;                                                                               \
    uint32_t nextSample

void analog_process(srv_t *state);
void analog_handle_packet(srv_t *state, jd_packet_t *pkt);
void analog_init(const srv_vt_t *vt, const analog_config_t *cfg);

#define ANALOG_SRV_DEF(service_cls, ...)                                                           \
    do {                                                                                           \
        static const analog_config_t analog_cfg = {__VA_ARGS__};                                   \
        SRV_DEF_SZ(analog, service_cls, sizeof(struct { ANALOG_SENSOR_STATE; }));                  \
        analog_init(&analog_vt, &analog_cfg);                                                      \
    } while (0)
