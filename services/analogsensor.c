// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_pins.h"
#include "interfaces/jd_adc.h"

struct srv_state {
    ANALOG_SENSOR_STATE;
};

static void analog_update(srv_t *state) {
    const analog_config_t *cfg = state->config;

    if (!state->reading_pending) {
        pin_setup_output(cfg->pinH);
        pin_set(cfg->pinH, 1);
        pin_setup_output(cfg->pinL);
        pin_set(cfg->pinL, 0);

        if (cfg->sampling_delay) {
            state->nextSample = now + 1000 * cfg->sampling_delay;
            state->reading_pending = 1;
            return;
        }
    }

    state->reading_pending = 0;

    int scale = cfg->scale;
    if (!scale)
        scale = 1024;
    int v = cfg->offset + (((int)adc_read_pin(cfg->pinM) * scale) >> 10);
    if (v < 0)
        v = 0;
    else if (v > 0xffff)
        v = 0xffff;
    state->sample = v;

    // save power
    pin_setup_analog_input(cfg->pinH);
    pin_setup_analog_input(cfg->pinL);
}

void analog_process(srv_t *state) {
    if (sensor_maybe_init(state))
        analog_update(state);

    int sampling_ms = state->config->sampling_ms * 1000;
    if (!sampling_ms)
        sampling_ms = 9000;

    if (jd_should_sample(&state->nextSample, sampling_ms) && state->jd_inited)
        analog_update(state);

    sensor_process_simple(state, &state->sample, sizeof(state->sample));
}

void analog_handle_packet(srv_t *state, jd_packet_t *pkt) {
    if (state->config->variant && pkt->service_command == JD_GET(JD_REG_VARIANT)) {
        jd_respond_u8(pkt, state->config->variant);
        return;
    }
    sensor_handle_packet_simple(state, pkt, &state->sample, sizeof(state->sample));
}

void analog_init(const srv_vt_t *vt, const analog_config_t *cfg) {
    srv_t *state = jd_allocate_service(vt);
    if (cfg->streaming_interval)
        state->streaming_interval = cfg->streaming_interval;
    state->config = cfg;
}

#if JD_DCFG

#include "jacdac/dist/c/lightlevel.h"
#include "jacdac/dist/c/reflectedlight.h"
#include "jacdac/dist/c/waterlevel.h"
#include "jacdac/dist/c/soundlevel.h"
#include "jacdac/dist/c/soilmoisture.h"
#include "jacdac/dist/c/potentiometer.h"

typedef struct {
    const char *name;
    uint32_t srvcls;
} subtype_t;
const subtype_t subtypes[] = { //
    {"lightLevel", JD_SERVICE_CLASS_LIGHT_LEVEL},
    {"reflectedLight", JD_SERVICE_CLASS_REFLECTED_LIGHT},
    {"waterLevel", JD_SERVICE_CLASS_WATER_LEVEL},
    {"soundLevel", JD_SERVICE_CLASS_SOUND_LEVEL},
    {"soilMoisture", JD_SERVICE_CLASS_SOIL_MOISTURE},
    {"potentiometer", JD_SERVICE_CLASS_POTENTIOMETER},
    {NULL, 0}};

void analog_config(void) {
    const char *srv = dcfg_get_string(jd_srvcfg_key("service"), NULL);
    if (srv)
        srv = strchr(srv, ':');
    if (srv)
        srv++;
    if (!srv) {
        DMESG("! invalid analog: service");
        return;
    }
    unsigned service_class = 0;
    for (unsigned i = 0; subtypes[i].name; ++i)
        if (strcmp(subtypes[i].name, srv) == 0) {
            service_class = subtypes[i].srvcls;
            break;
        }
    if (!service_class) {
        DMESG("! invalid analog: %s", srv);
        return;
    }

    uint8_t pin = jd_srvcfg_pin("pin");

    if (!adc_can_read_pin(pin)) {
        DMESG("! can't ADC %d", pin);
        return;
    }

    analog_config_t *cfg = jd_alloc(sizeof(analog_config_t));

    cfg->pinM = pin;
    cfg->pinL = jd_srvcfg_pin("pinLow");
    cfg->pinH = jd_srvcfg_pin("pinHigh");
    cfg->offset = jd_srvcfg_i32("offset", 0);
    cfg->scale = jd_srvcfg_i32("offset", 1024);
    cfg->sampling_ms = jd_srvcfg_u32("samplingItv", 1024);
    cfg->sampling_delay = jd_srvcfg_u32("samplingDelay", 1024);
    cfg->streaming_interval = jd_srvcfg_u32("streamingItv", 1024);

    srv_vt_t *vt = jd_alloc(sizeof(srv_vt_t));
    vt->service_class = service_class;
    vt->state_size = sizeof(struct { ANALOG_SENSOR_STATE; });
    vt->process = analog_process;
    vt->handle_pkt = analog_handle_packet;

    analog_init(vt, cfg);
}
#endif