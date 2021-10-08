#include "jd_services.h"
#include "interfaces/jd_pins.h"
#include "jacdac/dist/c/vibration.h"
#include "lib.h"

#define MAX_SEQUENCE        118

struct srv_state {
    SRV_COMMON;
    uint8_t enabled;
    uint8_t padding;
    uint32_t now;
    uint8_t idx;
    const vibration_motor_api_t* api;
    jd_vibration_motor_vibrate_t sequence[118];
};

REG_DEFINITION(                                   //
    vibration_regs,                                   //
    REG_SRV_COMMON,                         //
    REG_U8(JD_VIBRATION_MOTOR_REG_ENABLED),                 //
)

void vibration_process(srv_t * state) {
    if (!jd_should_sample(&state->now, 1000))
        return;

    jd_vibration_motor_vibrate_t* curr = &state->sequence[state->idx];

    if (curr->duration == 0) {
        state->idx = (state->idx + 1) % MAX_SEQUENCE;
        return;
    }
    
    // each speed tick is 8 ms in duration 
    state->api->write_amplitude(curr->speed, 8);
    curr->duration--;
}


static void handle_vibrate_cmd(srv_t *state, jd_packet_t *pkt) {
    memset(state->sequence, 0, sizeof(state->sequence));
    memcpy(state->sequence, pkt->data, min(sizeof(state->sequence), pkt->service_size));
    state->idx = 0;
}

void vibration_handle_packet(srv_t *state, jd_packet_t *pkt) {

    switch(pkt->service_command) {
        case JD_VIBRATION_MOTOR_CMD_VIBRATE:
            handle_vibrate_cmd(state, pkt);
            break;

        default:
            service_handle_register(state, pkt, vibration_regs);
            break;
    }
}

SRV_DEF(vibration, JD_SERVICE_CLASS_VIBRATION_MOTOR);
void vibration_service_init(const vibration_motor_api_t *api) {
    SRV_ALLOC(vibration);
    state->api = api;
    state->enabled = 1;

    state->api->init();
}