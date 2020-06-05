#include "jdsimple.h"

struct srv_state {
    SENSOR_COMMON;
    uint8_t state, inited;
    uint8_t pin0, pin1;
    int32_t sample, position;
    uint32_t nextSample;
};

static const int8_t posMap[] = {0, +1, -1, +2, -1, 0, -2, +1, +1, -2, 0, -1, +2, -1, +1, 0};
static void update(srv_t *state) {
    // based on comments in https://github.com/PaulStoffregen/Encoder/blob/master/Encoder.h
    uint16_t s = state->state & 3;
    if (pin_get(state->pin0))
        s |= 4;
    if (pin_get(state->pin1))
        s |= 8;

    state->state = (s >> 2);
    if (posMap[s]) {
        state->position += posMap[s];
        state->sample = state->position >> 2;
    }
}

static void maybe_init(srv_t *state) {
    if (state->got_query && !state->inited) {
        state->inited = true;
        tim_max_sleep = 1000;
        pin_setup_input(state->pin0, 1);
        pin_setup_input(state->pin1, 1);
        update(state);
    }
}

void crank_process(srv_t *state) {
    maybe_init(state);

    if (should_sample(&state->nextSample, 900) && state->inited)
        update(state);

    sensor_process_simple(state, &state->sample, sizeof(state->sample));
}

void crank_handle_packet(srv_t *state, jd_packet_t *pkt) {
    sensor_handle_packet_simple(state, pkt, &state->sample, sizeof(state->sample));
}

SRV_DEF(crank, JD_SERVICE_CLASS_ROTARY_ENCODER);

// specify pin0/1 so that clockwise rotations gives higher readings
void crank_init(uint8_t pin0, uint8_t pin1) {
    SRV_ALLOC(crank);
    state->pin0 = pin0;
    state->pin1 = pin1;
}
