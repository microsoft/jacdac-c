#include "jd_services.h"
#include "interfaces/jd_pins.h"
#include "jacdac/dist/c/relay.h"

struct srv_state {
    SRV_COMMON;
    uint8_t intensity;
    uint8_t variant;
    uint32_t max_switching_current;
    relay_params_t params;
};

REG_DEFINITION(                                    //
    relay_regs,                                    //
    REG_SRV_COMMON,                                //
    REG_U8(JD_RELAY_REG_ACTIVE),                   //
    REG_OPT8(JD_RELAY_REG_VARIANT),                //
    REG_OPT32(JD_RELAY_REG_MAX_SWITCHING_CURRENT), //
)

static void reflect_register_state(srv_t *state) {
    // active
    if (state->intensity) {
        pin_set(state->params.pin_relay_drive, (state->params.drive_active_lo ? 0 : 1));
        pin_set(state->params.pin_relay_led, (state->params.led_active_lo ? 0 : 1));
    }
    // inactive
    else {
        pin_set(state->params.pin_relay_drive, (state->params.drive_active_lo ? 1 : 0));
        pin_set(state->params.pin_relay_led, (state->params.led_active_lo ? 1 : 0));
    }
}

void relay_process(srv_t *state) {
    (void)state;
    // bool server_state = state->intensity;

    // if (state->params.pin_relay_feedback != NO_PIN && server_state !=
    // (bool)pin_get(state->params.pin_relay_feedback))
    //     JD_PANIC();
}

void relay_handle_packet(srv_t *state, jd_packet_t *pkt) {
    switch (service_handle_register_final(state, pkt, relay_regs)) {
    case JD_RELAY_REG_ACTIVE:
        reflect_register_state(state);
        break;
    }
}

SRV_DEF(relay, JD_SERVICE_CLASS_RELAY);
void relay_service_init(const relay_params_t *params) {
    SRV_ALLOC(relay);
    state->params = *params;
    pin_setup_output(state->params.pin_relay_drive);
    pin_setup_output(state->params.pin_relay_led);
    pin_setup_input(state->params.pin_relay_feedback, PIN_PULL_NONE);

    state->max_switching_current = params->max_switching_current;
    state->variant = params->relay_variant;
    state->intensity = (params->initial_state) ? 1 : 0;
    reflect_register_state(state);
}