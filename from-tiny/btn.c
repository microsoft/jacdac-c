#include "jdsimple.h"

#define EVT_DOWN 1
#define EVT_UP 2
#define EVT_CLICK 3
#define EVT_LONG_CLICK 4

struct srv_state {
    SENSOR_COMMON;
    uint8_t pin;
    uint8_t blpin;
    uint8_t pressed;
    uint8_t prev_pressed;
    uint8_t num_zero;
    uint8_t active;
    uint32_t press_time;
    uint32_t nextSample;
};

static void update(srv_t *state) {
    state->pressed = pin_get(state->pin) == state->active;
    if (state->pressed != state->prev_pressed) {
        state->prev_pressed = state->pressed;
        pin_set(state->blpin, state->pressed);
        if (state->pressed) {
            txq_push_event(state, EVT_DOWN);
            state->press_time = now;
        } else {
            txq_push_event(state, EVT_UP);
            uint32_t presslen = now - state->press_time;
            if (presslen > 500000)
                txq_push_event(state, EVT_LONG_CLICK);
            else
                txq_push_event(state, EVT_CLICK);
            state->num_zero = 0;
        }
    }
}

void btn_process(srv_t *state) {
    if (should_sample(&state->nextSample, 9000)) {
        update(state);

        if (sensor_should_stream(state) && (state->pressed || state->num_zero < 20)) {
            state->num_zero++;
            txq_push(state->service_number, JD_CMD_GET_REG | JD_REG_READING, &state->pressed,
                     sizeof(state->pressed));
        }
    }
}

void btn_handle_packet(srv_t *state, jd_packet_t *pkt) {
    sensor_handle_packet_simple(state, pkt, &state->pressed, sizeof(state->pressed));
}

SRV_DEF(btn, JD_SERVICE_CLASS_BUTTON);

void btn_init(uint8_t pin, uint8_t blpin) {
    SRV_ALLOC(btn);
    state->pin = pin;
    state->blpin = blpin;
    if (blpin == 0xff) {
        state->active = 0;
    } else {
        state->active = 1;
        pin_setup_output(blpin);
    }
    pin_setup_input(state->pin, state->active == 0 ? 1 : -1);
    update(state);
}
