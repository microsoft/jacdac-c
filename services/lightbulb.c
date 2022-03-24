#include "jd_services.h"
#include "interfaces/jd_pins.h"
#include "jacdac/dist/c/lightbulb.h"

struct srv_state {
    SRV_COMMON;
    uint16_t intensity;
    uint8_t dimmable;
    uint8_t pin;
};

REG_DEFINITION(                            //
    bulb_regs,                             //
    REG_SRV_COMMON,                        //
    REG_U16(JD_LIGHT_BULB_REG_BRIGHTNESS), //
    REG_U8(JD_LIGHT_BULB_REG_DIMMABLE),    //
)

static void reflect_register_state(srv_t *state) {
    // active
    if (state->intensity) {
        pin_setup_output(state->pin);
        pin_set(state->pin, 1);
    }
    // inactive
    else
        pin_setup_input(state->pin, PIN_PULL_DOWN);
}

void bulb_process(srv_t *state) {
    (void)state;
}

void bulb_handle_packet(srv_t *state, jd_packet_t *pkt) {
    service_handle_register_final(state, pkt, bulb_regs);
    reflect_register_state(state);
}

SRV_DEF(bulb, JD_SERVICE_CLASS_LIGHT_BULB);
void lightbulb_init(const uint8_t pin) {
    SRV_ALLOC(bulb);
    state->pin = pin;
    pin_setup_input(state->pin, PIN_PULL_DOWN);
    state->dimmable = 0;
}