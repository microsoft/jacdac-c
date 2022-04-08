#include "jd_services.h"
#include "interfaces/jd_pins.h"
#include "jacdac/dist/c/dotmatrix.h"
#include "braille_common.h"

#define STATE_DIRTY 0x01

#define DOTS_MAX 64

struct srv_state {
    SRV_COMMON;
    const hbridge_api_t *api;
    uint16_t rows;
    uint16_t cols;
    uint8_t variant;
    uint8_t flags;
    uint8_t dots[DOTS_MAX];
    braille_get_channels_t get_channels;
};

REG_DEFINITION(                         //
    braille_dm_regs,                    //
    REG_SRV_COMMON,                     //
    REG_PTR_PADDING(),                  //
    REG_U16(JD_DOT_MATRIX_REG_ROWS),    //
    REG_U16(JD_DOT_MATRIX_REG_COLUMNS), //
    REG_U8(JD_DOT_MATRIX_REG_VARIANT),  //
)

#define BrGetPtn(x, y) ((state->dots[y] & (1 << (x))) != 0)

// BrRfshPtn: Refresh and load new braille pattern
static void braille_dm_send(srv_t *state) {
    const hbridge_api_t *api = state->api;

    for (int col = 0; col < state->cols; ++col) {
        for (int row = 0; row < state->rows; ++row) {
            api->clear_all();

            uint32_t ch = state->get_channels(row, col);
            uint16_t ch0 = ch & 0xffff;
            uint16_t ch1 = (ch >> 16) & 0xffff;

            if (!BrGetPtn(row, col)) {
                api->channel_set(ch0, 0);
                api->channel_set(ch1, 1);
            } else {
                api->channel_set(ch0, 1);
                api->channel_set(ch1, 0);
            }

            api->write_channels();
            jd_services_sleep_us(7000);
            api->clear_all();
            api->write_channels();
            jd_services_sleep_us(4000);
        }
    }
}

void braille_dm_process(srv_t *state) {
    if (state->flags & STATE_DIRTY) {
        braille_dm_send(state);
        state->flags &= ~STATE_DIRTY;
    }
}

void braille_dm_handle_packet(srv_t *state, jd_packet_t *pkt) {
    switch (pkt->service_command) {
    case JD_SET(JD_DOT_MATRIX_REG_DOTS):
        if (pkt->service_size >= state->cols) {
            memcpy(state->dots, pkt->data, state->cols);
            state->flags |= STATE_DIRTY;
        }
        break;
    case JD_GET(JD_DOT_MATRIX_REG_DOTS):
        jd_send(pkt->service_index, pkt->service_command, &state->dots, state->cols);
        break;
    default:
        service_handle_register_final(state, pkt, braille_dm_regs);
    }
}

SRV_DEF(braille_dm, JD_SERVICE_CLASS_DOT_MATRIX);
void braille_dm_init(const hbridge_api_t *params, uint16_t rows, uint16_t cols,
                     braille_get_channels_t get_channels) {
    SRV_ALLOC(braille_dm);

    state->api = params;
    state->rows = rows;
    state->cols = cols;
    state->variant = JD_DOT_MATRIX_VARIANT_BRAILLE;
    state->flags = 0;
    state->get_channels = get_channels;

    state->api->init();

    JD_ASSERT(rows <= 8);
    JD_ASSERT(cols < DOTS_MAX);

    memset(state->dots, 0, DOTS_MAX);

    braille_dm_send(state);
    target_wait_us(3000);
}