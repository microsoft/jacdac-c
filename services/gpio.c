// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_pwm.h"
#include "interfaces/jd_pins.h"
#include "interfaces/jd_hw_pwr.h"
#include "jacdac/dist/c/gpio.h"
#include "interfaces/jd_adc.h"

#define EDGE_SAMPLE_MS 10

typedef struct {
    jd_gpio_pin_info_report_t info;
    const char *label;
} pin_desc_t;

// this is always called struct srv_state
struct srv_state {
    SENSOR_COMMON;
    uint8_t num_pins;
    uint8_t num_values;
    uint32_t edge_timer;
    uint8_t *values;
    uint8_t *prev_values;
    pin_desc_t *pins;
};

static srv_t *gpiosrv_state;

REG_DEFINITION(                   //
    gpiosrv_regs,                 //
    REG_SENSOR_COMMON,            //
    REG_U8(JD_GPIO_REG_NUM_PINS), //
)

static pin_desc_t *pin_by_hw(srv_t *state, uint8_t hw_id) {
    for (unsigned i = 0; i < state->num_pins; ++i)
        if (state->pins[i].info.hw_pin == hw_id)
            return &state->pins[i];
    return NULL;
}

static pin_desc_t *pin_by_label(srv_t *state, const char *lbl, unsigned len) {
    for (unsigned i = 0; i < state->num_pins; ++i) {
        if (strlen(state->pins[i].label) == len && memcmp(state->pins[i].label, lbl, len) == 0)
            return &state->pins[i];
    }
    return NULL;
}

static inline int pull_from_mode(uint8_t *mode) {
    switch (*mode & 0xf0) {
    case JD_GPIO_MODE_OFF_PULL_UP:
        return PIN_PULL_UP;
    case JD_GPIO_MODE_OFF_PULL_DOWN:
        return PIN_PULL_DOWN;
    default:
        *mode = *mode & 0x0f;
        return PIN_PULL_NONE;
    }
}

static pin_desc_t *pin_by_id(srv_t *state, unsigned hw_id) {
    if (hw_id >= state->num_pins)
        return NULL;
    return &state->pins[hw_id];
}

static void send_info(srv_t *state, jd_packet_t *pkt, pin_desc_t *pin) {
    if (!pin)
        return;
    unsigned size = offsetof(jd_gpio_pin_info_report_t, label) + strlen(pin->label);
    uint8_t buf[size];
    jd_gpio_pin_info_report_t *r = (void *)buf;
    *r = pin->info;
    memcpy(r->label, pin->label, strlen(pin->label));
    jd_send(pkt->service_index, pkt->service_command, buf, size);
}

static void cmd_configure(srv_t *state, jd_packet_t *pkt) {
    jd_gpio_configure_t *d = (void *)pkt->data;
    pin_desc_t *pin = pin_by_id(state, d->pin);
    if (!pin)
        return;
    int gpio = pin->info.hw_pin;
    switch (d->mode & JD_GPIO_MODE_BASE_MODE_MASK) {
    case JD_GPIO_MODE_OFF:
        pin_setup_analog_input(gpio);
        pin_set_pull(gpio, pull_from_mode(&d->mode));
        break;
    case JD_GPIO_MODE_INPUT:
        pin_setup_input(gpio, pull_from_mode(&d->mode));
        break;
    case JD_GPIO_MODE_OUTPUT:
        if (pull_from_mode(&d->mode) == 0)
            return;
        pin_set(gpio, pull_from_mode(&d->mode) == PIN_PULL_UP);
        pin_setup_output(gpio);
        break;
    case JD_GPIO_MODE_ANALOG_IN:
    case JD_GPIO_MODE_ALTERNATIVE:
    default:
        return; // not impl.
    }
    pin->info.mode = d->mode;
}

void gpiosrv_process(srv_t *state) {
    for (unsigned i = 0; i < state->num_pins; ++i) {
        uint8_t mode = state->pins[i].info.mode;
        int v = 0;

        if ((mode & JD_GPIO_MODE_BASE_MODE_MASK) == JD_GPIO_MODE_INPUT)
            v = pin_get(state->pins[i].info.hw_pin);

        if (v)
            state->values[i / 8] |= (1 << (i & 7));
        else
            state->values[i / 8] &= ~(1 << (i & 7));
    }

    // this only triggers when values change and note more often than every EDGE_SAMPLE_MS
    if (!in_future_ms(state->edge_timer) &&
        memcmp(state->values, state->prev_values, state->num_values) != 0) {
        memcpy(state->prev_values, state->values, state->num_values);
        state->edge_timer = now_ms + EDGE_SAMPLE_MS;
        jd_send(state->service_index, JD_GET(JD_REG_READING), state->values, state->num_values);
    }

    // this is regular streaming
    sensor_process_simple(state, state->values, state->num_values);
}

void gpiosrv_handle_packet(srv_t *state, jd_packet_t *pkt) {
    switch (pkt->service_command) {
    case JD_GPIO_CMD_CONFIGURE:
        cmd_configure(state, pkt);
        break;
    case JD_GPIO_CMD_PIN_INFO:
        send_info(state, pkt, pin_by_id(state, pkt->data[0]));
        break;
    case JD_GPIO_CMD_PIN_BY_HW_PIN:
        send_info(state, pkt, pin_by_hw(state, pkt->data[0]));
        break;
    case JD_GPIO_CMD_PIN_BY_LABEL:
        send_info(state, pkt, pin_by_label(state, (const char *)pkt->data, pkt->service_size));
        break;
    default:
        if (service_handle_register(state, pkt, gpiosrv_regs))
            return;
        sensor_handle_packet_simple(state, pkt, state->values, state->num_values);
        break;
    }
}

SRV_DEF(gpiosrv, JD_SERVICE_CLASS_GPIO);

#if JD_DCFG
static bool valid_pin(const char *name) {
    return name[5] != '@' && dcfg_get_pin(name) != NO_PIN;
}
void gpiosrv_config(void) {
    SRV_ALLOC(gpiosrv);

    gpiosrv_state = state;

    state->streaming_interval = 500; // this only applies when there are no changes on the pins

    const dcfg_entry_t *info = NULL;
    for (;;) {
        info = dcfg_get_next_entry("pins.", info);
        if (info == NULL)
            break;
        if (valid_pin(info->key))
            state->num_pins++;
    }

    DMESG("GPIO init: %d pins", state->num_pins);

    state->pins = jd_alloc(sizeof(pin_desc_t) * state->num_pins);
    state->num_values = (state->num_pins / 8) + 1;
    state->values = jd_alloc(2 * state->num_values);
    state->prev_values = state->values + state->num_values;

    info = NULL;
    unsigned idx = 0;
    for (;;) {
        info = dcfg_get_next_entry("pins.", info);
        if (info == NULL)
            break;
        if (!valid_pin(info->key))
            continue;

        pin_desc_t *p = &state->pins[idx];
        p->label = info->key + 5;

        p->info.pin = idx;
        int gpio = dcfg_get_pin(info->key);

        DMESG("GPIO init[%u] %s -> %d", idx, p->label, gpio);

        p->info.hw_pin = gpio;
        p->info.capabilities = JD_GPIO_CAPABILITIES_PULL_UP | JD_GPIO_CAPABILITIES_PULL_DOWN |
                               JD_GPIO_CAPABILITIES_INPUT | JD_GPIO_CAPABILITIES_OUTPUT;

        if (adc_can_read_pin(gpio))
            p->info.capabilities |= JD_GPIO_CAPABILITIES_ANALOG;

        pin_setup_analog_input(gpio);
        p->info.mode = JD_GPIO_MODE_OFF;
        idx++;
    }

    info = NULL;
    for (;;) {
        info = dcfg_get_next_entry("sPin.", info);
        if (!info)
            break;
        if (info->value == 0 || info->value == 1) {
            char keybuf[DCFG_KEYSIZE + 1];
            memcpy(keybuf, info->key, sizeof(keybuf));
            memcpy(keybuf, "pins", 4);
            uint8_t gpio = dcfg_get_pin(keybuf);
            if (gpio != NO_PIN) {
                DMESG("sPin: %s(%d) set to %d", keybuf, gpio, (int)info->value);
                pin_set(gpio, info->value);
                pin_setup_output(gpio);

                pin_desc_t *d = pin_by_hw(state, gpio);
                // note that this may be 'hidden' pin
                if (d)
                    d->info.mode = info->value ? JD_GPIO_MODE_OUTPUT_HIGH : JD_GPIO_MODE_OUTPUT_LOW;
            }
        }
    }
}
#endif
