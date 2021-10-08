#include "jd_services.h"
#include "interfaces/jd_pins.h"
#include "jacdac/dist/c/dotmatrix.h"
#include "braille_common.h"

#define MAXBRROW 4
#define MAXBRCOL 8
#define MAXBRPHASE 2 // Up, Down
#define MAXBRBANK 2  // HiBank, LoBank
#define BRPHASEUP 0
#define BRPHASEDN 1
#define BRWAITSETU 3   // msec after set/reset each dot for UP
#define BRWAITSETD 12  // msec after set/reset each dot for DOWN
#define BRWAITSETFU 10 // msec after set/reset each dot for forced UP
#define BRWAITSETFD 10 // msec after set/reset each dot for forced DOWN
#define BRWAITSETD1 20 // special down delay for specific modules/dots
#define BRWAITDIS1 1   // msec for discharge state 1
#define BRWAITDIS2 1   // msec for discharge state 2
#define BRWAITHIZ 50   // msec for high-Z
#define BRWAITFHIZ 30  // msec for forced high-Z

#define STATE_DIRTY 0x01

#define DOTS_MAX    64

struct srv_state {
    SRV_COMMON;
    const hbridge_api_t *api;
    uint16_t rows;
    uint16_t cols;
    uint8_t variant;
    uint8_t flags;
    uint16_t padding;
    uint32_t now;
    uint8_t dots[DOTS_MAX];
    const uint8_t* cell_map;
};

REG_DEFINITION(                                   //
    braille_dm_regs,                                   //
    REG_SRV_COMMON,                         //
    REG_U32(JD_REG_PADDING),                         //
    REG_U16(JD_DOT_MATRIX_REG_ROWS),                         //
    REG_U16(JD_DOT_MATRIX_REG_COLUMNS),                         //
    REG_U8(JD_DOT_MATRIX_REG_VARIANT),                //
    REG_U8(JD_REG_PADDING),                // flags
    REG_U16(JD_REG_PADDING),                         //
    REG_U32(JD_REG_PADDING),                         //
    REG_BYTES(JD_DOT_MATRIX_REG_DOTS, DOTS_MAX),                         //
)


void braille_dm_process(srv_t * state) {
    if (state->flags & STATE_DIRTY) {
        BrRfshPtn(state->api);
        state->flags &= ~STATE_DIRTY;
    }
}

static bool get_bit(srv_t * state, uint8_t* data, uint16_t row, uint16_t col) {
    uint16_t cols_padded = state->cols + (8 - (state->cols % 8));
    uint32_t bitindex = row * cols_padded + col;
    uint8_t byte = data[bitindex >> 3];
    uint8_t bit = bitindex % 8;
    return (1 == ((byte >> bit) & 1));
}

static void set_bit(srv_t * state, uint8_t* data, uint16_t row, uint16_t col) {
    uint16_t cols_padded = state->cols + (8 - (state->cols % 8));
    uint32_t bitindex = row * cols_padded + col;
    uint8_t* byte = &data[bitindex >> 3];
    uint8_t bit = bitindex % 8;
    *byte |= 1 << bit;
}

static void handle_disp_write(srv_t * state, jd_packet_t* pkt) {

    // update in progress
    if (state->flags & STATE_DIRTY)
        return;

    // too big for our buffer
    if (pkt->service_size > DOTS_MAX)
        hw_panic();

    BrClrPtn();
    
    for (int row = 0; row < state->rows; row++) 
        for (int col = 0; col < state->cols; col++) {
            uint8_t cell = state->cell_map[col / 2];
            bool upper = (col%2);
            if (get_bit(state, pkt->data, row, col))
                BrSetPtn(cell, (upper) ? 4 + row : row, true);
        }

    state->flags |= STATE_DIRTY;
}

static void handle_disp_read(srv_t * state) {

    memset(state->dots, 0, DOTS_MAX);

    for (int row = 0; row < state->rows; row++) 
        for (int col = 0; col < state->cols; col++) {
            uint8_t cell = state->cell_map[col / 2];
            bool upper = (col%2);
            if (BrGetPtn(cell, (upper) ? 4 + row : row))
                set_bit(state, state->dots, row, col);
        }
}

void braille_dm_handle_packet(srv_t *state, jd_packet_t *pkt) {

    switch (pkt->service_command) {
        case JD_SET(JD_DOT_MATRIX_REG_DOTS):
            handle_disp_write(state, pkt);
            break;
        default:
            handle_disp_read(state);
            switch (service_handle_register(state, pkt, braille_dm_regs)) {
                default:
                    break;
            }
    }
}

SRV_DEF(braille_dm, JD_SERVICE_CLASS_DOT_MATRIX);
void braille_dm_init(const hbridge_api_t *params, uint16_t rows, uint16_t cols, const uint8_t* cell_map) {
    SRV_ALLOC(braille_dm);

    state->api = params;
    state->rows = rows;
    state->cols = cols;
    state->variant = JD_DOT_MATRIX_VARIANT_BRAILLE;
    state->flags = 0;
    state->now = 0;
    state->cell_map = cell_map;

    state->api->init();

    uint16_t dot_bytes = 8 * cols; // this will need fixed up if we expand in future

    if (dot_bytes > DOTS_MAX)
        hw_panic();

    memset(state->dots, 0, DOTS_MAX);

    BrClrPtn();

    BrClrAllDots(state->api);
    target_wait_us(3000);
}