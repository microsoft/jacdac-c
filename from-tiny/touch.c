#include "jdsimple.h"

#define EVT_DOWN 1
#define EVT_UP 2
#define EVT_CLICK 3
#define EVT_LONG_CLICK 4

struct srv_state {
    SENSOR_COMMON;
    uint8_t pin;
    uint16_t reading;
    uint32_t nextSample;
};

static void update(srv_t *state) {
    uint8_t pin = state->pin;
    pin_set(pin, 1);
    pin_setup_output(pin);
    pin_setup_analog_input(pin);
    target_wait_us(50);
    state->reading = adc_read_pin(pin);
}

void touch_process(srv_t *state) {
    if (should_sample(&state->nextSample, 50000)) {
        update(state);
        sensor_process_simple(state, &state->reading, sizeof(state->reading));
    }
}

void touch_handle_packet(srv_t *state, jd_packet_t *pkt) {
    sensor_handle_packet_simple(state, pkt, &state->reading, sizeof(state->reading));
}

SRV_DEF(touch, JD_SERVICE_CLASS_BUTTON);

void touch_init(uint8_t pin) {
    SRV_ALLOC(touch);
    state->pin = pin;
    pin_setup_input(state->pin, 0);
    update(state);
}
