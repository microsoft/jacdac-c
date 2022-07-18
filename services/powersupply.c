#include "jd_services.h"
#include "interfaces/jd_pins.h"
#include "jacdac/dist/c/powersupply.h"

struct srv_state {
    SRV_COMMON;
    uint8_t enabled;
    double output_voltage;
    double minimum_voltage;
    double maximum_voltage;
    power_supply_params_t params;
};

REG_DEFINITION(                                     //
    psu_regs,                                       //
    REG_SRV_COMMON,                                 //
    REG_U8(JD_POWER_SUPPLY_REG_ENABLED),            //
    REG_BYTE8(JD_POWER_SUPPLY_REG_OUTPUT_VOLTAGE),  //
    REG_BYTE8(JD_POWER_SUPPLY_REG_MINIMUM_VOLTAGE), //
    REG_BYTE8(JD_POWER_SUPPLY_REG_MAXIMUM_VOLTAGE), //
)

static void update_wiper_value(srv_t *state) {
    state->params.potentiometer->set_wiper(state->params.wiper_channel,
                                           state->params.voltage_to_wiper(state->output_voltage));
}

static void reflect_register_state(srv_t *state) {
    if (state->enabled) {
        pin_set(state->params.enable_pin, (state->params.enable_active_lo) ? 0 : 1);
        update_wiper_value(state);
    } else
        pin_set(state->params.enable_pin, (state->params.enable_active_lo) ? 1 : 0);
}

void psu_process(srv_t *state) {
    (void)state;
}

void psu_handle_packet(srv_t *state, jd_packet_t *pkt) {
    int ret = service_handle_register_final(state, pkt, psu_regs);

    if (ret == JD_POWER_SUPPLY_REG_OUTPUT_VOLTAGE) {
        if (state->output_voltage < state->minimum_voltage)
            state->output_voltage = state->minimum_voltage;
        if (state->output_voltage > state->maximum_voltage)
            state->output_voltage = state->maximum_voltage;
    }

    reflect_register_state(state);
}

SRV_DEF(psu, JD_SERVICE_CLASS_POWER_SUPPLY);
void powersupply_init(const power_supply_params_t params) {
    SRV_ALLOC(psu);

    state->params = params;
    state->enabled = 0;
    state->output_voltage = params.initial_voltage;
    state->minimum_voltage = params.min_voltage;
    state->maximum_voltage = params.max_voltage;

    state->params.potentiometer->init();

    pin_setup_output(params.enable_pin);
    pin_set(state->params.enable_pin, (state->params.enable_active_lo) ? 1 : 0);

#if 1
    // TODO disable this - currently the dashboard doesn't have power supply switch
    state->enabled = 1;
#endif

    reflect_register_state(state);
}