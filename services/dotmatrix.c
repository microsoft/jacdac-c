#include "jd_services.h"
#include "interfaces/jd_pins.h"
#include "jacdac/dist/c/dotmatrix.h"
#include "interfaces/jd_hw_pwr.h"
#include "interfaces/jd_disp.h"

#if defined(NUM_DISPLAY_COLS)

#define BUFFER_SIZE (NUM_DISPLAY_COLS * ((NUM_DISPLAY_ROWS + 7) >> 3))

struct srv_state {
    SRV_COMMON;
    uint16_t rows;
    uint16_t cols;
    uint8_t variant;
    uint8_t brightness;
    uint8_t dots[BUFFER_SIZE];
    uint32_t refresh;
    uint8_t was_enabled;
};

REG_DEFINITION(                                     //
    dotmatrix_regs,                                 //
    REG_SRV_COMMON,                                 //
    REG_U16(JD_DOT_MATRIX_REG_ROWS),                //
    REG_U16(JD_DOT_MATRIX_REG_COLUMNS),             //
    REG_U8(JD_DOT_MATRIX_REG_VARIANT),              //
    REG_U8(JD_DOT_MATRIX_REG_BRIGHTNESS),           //
    REG_BYTES(JD_DOT_MATRIX_REG_DOTS, BUFFER_SIZE), //
)

static bool is_empty(srv_t *state) {
    if (state->brightness == 0)
        return 1;
    for (int i = 0; i < BUFFER_SIZE; ++i)
        if (state->dots[i] != 0)
            return 0;
    return 1;
}

void dotmatrix_process(srv_t *state) {
    if (!jd_should_sample(&state->refresh, 9900))
        return;
    if (is_empty(state)) {
        if (state->was_enabled) {
            pwr_leave_no_sleep();
            state->was_enabled = 0;
        }
        return;
    }
    if (!state->was_enabled) {
        state->was_enabled = 1;
        pwr_enter_no_sleep();
    }
    disp_refresh();
}

static void sync_regs(srv_t *state) {
    disp_set_brigthness(state->brightness * state->brightness);
}

void dotmatrix_handle_packet(srv_t *state, jd_packet_t *pkt) {
    switch (service_handle_register_final(state, pkt, dotmatrix_regs)) {
    case JD_DOT_MATRIX_REG_DOTS:
        disp_show(state->dots);
        break;
    case JD_DOT_MATRIX_REG_BRIGHTNESS:
        sync_regs(state);
        break;
    default:
        break;
    }
}

SRV_DEF(dotmatrix, JD_SERVICE_CLASS_DOT_MATRIX);
void dotmatrix_init(void) {
    SRV_ALLOC(dotmatrix);
    state->rows = NUM_DISPLAY_ROWS;
    state->cols = NUM_DISPLAY_COLS;
    state->variant = JD_DOT_MATRIX_VARIANT_LED;
    state->brightness = 100;
    sync_regs(state);
}

#endif