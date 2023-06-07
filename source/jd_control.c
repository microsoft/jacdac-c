// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_protocol.h"

#if JD_CONFIG_CONTROL_FLOOD == 1
#include "services/interfaces/jd_hw_pwr.h"
#endif

#include "jd_util.h"

struct srv_state {
    SRV_COMMON;
#if JD_CONFIG_CONTROL_FLOOD == 1
    uint8_t flood_size;
#endif
#if JD_CONFIG_WATCHDOG == 1
    uint32_t watchdog;
#endif
#if JD_CONFIG_CONTROL_FLOOD == 1
    uint32_t flood_counter;
    uint32_t flood_remaining;
#endif
};

__attribute__((weak)) void target_standby(uint32_t duration_ms) {
    target_reset();
}

#if JD_CONFIG_CONTROL_FLOOD == 1
static void set_flood(srv_t *state, uint32_t num) {
    if (state->flood_remaining == 0 && num != 0)
        pwr_enter_no_sleep();
    else if (state->flood_remaining != 0 && num == 0)
        pwr_enter_no_sleep();
    state->flood_remaining = num;
}

static void process_flood(srv_t *state) {
    if (state->flood_remaining && jd_tx_is_idle()) {
        uint32_t len = 4 + state->flood_size;
        uint8_t tmp[len];
        memcpy(tmp, &state->flood_counter, 4);
        for (uint32_t i = 4; i < len; ++i)
            tmp[i] = i - 4;
        state->flood_counter++;
        set_flood(state, state->flood_remaining - 1);
        jd_send(JD_SERVICE_INDEX_CONTROL, JD_CONTROL_CMD_FLOOD_PING, tmp, len);
    }
}
#else
static void process_flood(srv_t *state) {}
#endif

#if JD_CONFIG_DEV_SPEC_URL == 1
extern const char app_spec_url[];
#endif

void jd_ctrl_process(srv_t *state) {
    process_flood(state);
#if JD_CONFIG_WATCHDOG == 1
    if (state->watchdog && in_past(state->watchdog))
        target_reset();
#endif
}

void jd_ctrl_handle_packet(srv_t *state, jd_packet_t *pkt) {
#if JD_CONFIG_STATUS == 1
    if (jd_status_handle_packet(pkt))
        return;
#endif

    switch (pkt->service_command) {
    case JD_CONTROL_CMD_SERVICES:
        jd_services_announce();
        break;

    case JD_CONTROL_CMD_IDENTIFY:
        jd_blink(JD_BLINK_IDENTIFY);
        break;

    case JD_CONTROL_CMD_RESET:
        target_reset();
        break;

    case JD_CONTROL_CMD_STANDBY:
        target_standby(*(uint32_t *)pkt->data);
        break;

#if JD_CONFIG_CONTROL_FLOOD == 1
    case JD_CONTROL_CMD_FLOOD_PING:
        if (pkt->service_size >= sizeof(jd_control_flood_ping_t)) {
            jd_control_flood_ping_t *arg = (jd_control_flood_ping_t *)pkt->data;
            state->flood_counter = arg->start_counter;
            state->flood_size = arg->size;
            set_flood(state, arg->num_responses);
        }
        break;
#endif

    case JD_GET(JD_CONTROL_REG_DEVICE_DESCRIPTION):
        jd_respond_string(pkt, app_get_dev_class_name());
        break;

    case JD_GET(JD_CONTROL_REG_FIRMWARE_VERSION):
        jd_respond_string(pkt, app_get_fw_version());
        break;

    case JD_GET(JD_CONTROL_REG_PRODUCT_IDENTIFIER):
    case JD_GET(JD_CONTROL_REG_BOOTLOADER_PRODUCT_IDENTIFIER):
        jd_respond_u32(pkt, app_get_device_class());
        break;

    case JD_GET(JD_CONTROL_REG_UPTIME): {
        uint64_t t = tim_get_micros();
        jd_send(JD_SERVICE_INDEX_CONTROL, pkt->service_command, &t, sizeof(t));
        break;
    }

#if JD_CONFIG_DEV_SPEC_URL == 1
    case JD_GET(JD_CONTROL_REG_DEVICE_SPECIFICATION_URL):
        jd_send(JD_SERVICE_INDEX_CONTROL, pkt->service_command, app_spec_url, strlen(app_spec_url));
        break;
#endif

#if JD_CONFIG_WATCHDOG == 1
    case JD_GET(JD_CONTROL_REG_RESET_IN): {
        uint32_t d = state->watchdog;
        if (d)
            d -= now;
        jd_respond_u32(pkt, d);
        break;
    }
    case JD_SET(JD_CONTROL_REG_RESET_IN):
        if (pkt->service_size == 4) {
            uint32_t delta = *(uint32_t *)pkt->data;
            state->watchdog = delta ? now + delta : 0;
        }
        break;
#endif

#if JD_CONFIG_TEMPERATURE == 1
    case JD_GET(JD_CONTROL_REG_MCU_TEMPERATURE):
        jd_respond_u32(pkt, adc_read_temp());
        break;
#endif

    default:
        jd_send_not_implemented(pkt);
        break;
    }
}

SRV_DEF(jd_ctrl, JD_SERVICE_CLASS_CONTROL);
void jd_ctrl_init(void) {
    SRV_ALLOC(jd_ctrl);
}
