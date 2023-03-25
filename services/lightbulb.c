#include "jd_services.h"
#include "interfaces/jd_pins.h"
#include "jacdac/dist/c/lightbulb.h"
#include "interfaces/jd_pwm.h"

struct srv_state {
    SRV_COMMON;
    uint16_t intensity;
    uint8_t dimmable;
    uint8_t pin;
    uint8_t active_lo;
    uint8_t pwm_id;
};

REG_DEFINITION(                            //
    bulb_regs,                             //
    REG_SRV_COMMON,                        //
    REG_U16(JD_LIGHT_BULB_REG_BRIGHTNESS), //
    REG_U8(JD_LIGHT_BULB_REG_DIMMABLE),    //
)

static void reflect_register_state(srv_t *state) {
    if (state->intensity) {
        if (state->dimmable) {
            uint16_t v = state->intensity >> 6;
            if (state->active_lo)
                v = 1023 - v;
            state->pwm_id = jd_pwm_init(state->pin, 1024, v, cpu_mhz / 16);
        } else {
            pin_setup_output(state->pin);
            pin_set(state->pin, state->active_lo ? 0 : 1);
        }
    } else {
        pin_setup_input(state->pin, state->active_lo ? PIN_PULL_UP : PIN_PULL_DOWN);
    }
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

#if JD_DCFG
void lightbulb_config(void) {
    lightbulb_init(jd_srvcfg_pin("pin"));
    srv_t *state = jd_srvcfg_last_service();
    state->active_lo = jd_srvcfg_has_flag("activeLow");
    if (jd_srvcfg_has_flag("dimmable"))
        state->dimmable = 1;
    reflect_register_state(state);
}
#endif