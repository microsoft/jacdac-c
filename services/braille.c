#include "jd_services.h"
#include "interfaces/jd_pins.h"
#include "jacdac/dist/c/dotmatrix.h"

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
};

REG_DEFINITION(                                   //
    braille_regs,                                   //
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

const uint16_t BrDotTbl[ MAXBRPHASE ][ MAXBRROW ][ MAXBRCOL ][ MAXBRBANK ] =
 {
  // Braittle module table ( ghost hack ( single GND )) 
  // Up
  { 
    { { 0x5841, 0x0081 }, { 0x5841, 0x0101 }, { 0x5841, 0x0201 }, { 0x5841, 0x0401 }, { 0x5841, 0x0801 }, { 0x5841, 0x1001 }, { 0x58C1, 0x0001 }, { 0x5941, 0x0001 } },
    { { 0x5821, 0x0081 }, { 0x5821, 0x0101 }, { 0x5821, 0x0201 }, { 0x5821, 0x0401 }, { 0x5821, 0x0801 }, { 0x5821, 0x1001 }, { 0x58A1, 0x0001 }, { 0x5921, 0x0001 } },
    { { 0x4611, 0x0081 }, { 0x4611, 0x0101 }, { 0x4611, 0x0201 }, { 0x4611, 0x0401 }, { 0x4611, 0x0801 }, { 0x4611, 0x1001 }, { 0x4691, 0x0001 }, { 0x4711, 0x0001 } }, 
    { { 0x4609, 0x0081 }, { 0x4609, 0x0101 }, { 0x4609, 0x0201 }, { 0x4609, 0x0401 }, { 0x4609, 0x0801 }, { 0x4609, 0x1001 }, { 0x4689, 0x0001 }, { 0x4709, 0x0001 } }
  },
  // Down
  { 
    { { 0x5839, 0x0083 }, { 0x5839, 0x0105 }, { 0x5839, 0x0209 }, { 0x5839, 0x0411 }, { 0x5839, 0x0821 }, { 0x5839, 0x1041 }, { 0x5883, 0x0001 }, { 0x5905, 0x0001 } },
    { { 0x5859, 0x0083 }, { 0x5859, 0x0105 }, { 0x5859, 0x0209 }, { 0x5859, 0x0411 }, { 0x5859, 0x0821 }, { 0x5859, 0x1041 }, { 0x5883, 0x0001 }, { 0x5905, 0x0001 } },
    { { 0x4669, 0x0083 }, { 0x4669, 0x0105 }, { 0x4669, 0x0209 }, { 0x4669, 0x0411 }, { 0x4669, 0x0821 }, { 0x4669, 0x1041 }, { 0x4683, 0x0001 }, { 0x4705, 0x0001 } },
    { { 0x4671, 0x0083 }, { 0x4671, 0x0105 }, { 0x4671, 0x0209 }, { 0x4671, 0x0411 }, { 0x4671, 0x0821 }, { 0x4671, 0x1041 }, { 0x4683, 0x0001 }, { 0x4705, 0x0001 } }
  }
};

const uint16_t BrDotTbl_forced[ MAXBRPHASE ][ MAXBRROW ][ MAXBRCOL ][ MAXBRBANK ] =
 {
  // Braittle module table ( original XY, ghost may occur )
  // Up
  {
  { { 0x5041, 0x0081 }, { 0x5041, 0x0101 }, { 0x5041, 0x0201 }, { 0x5041, 0x0401 }, { 0x5041, 0x0801 }, { 0x5041, 0x1001 }, { 0x50C1, 0x0001 }, { 0x5141, 0x0001 } },
  { { 0x4821, 0x0081 }, { 0x4821, 0x0101 }, { 0x4821, 0x0201 }, { 0x4821, 0x0401 }, { 0x4821, 0x0801 }, { 0x4821, 0x1001 }, { 0x48A1, 0x0001 }, { 0x4921, 0x0001 } },
  { { 0x4411, 0x0081 }, { 0x4411, 0x0101 }, { 0x4411, 0x0201 }, { 0x4411, 0x0401 }, { 0x4411, 0x0801 }, { 0x4411, 0x1001 }, { 0x4491, 0x0001 }, { 0x4511, 0x0001 } },
  { { 0x4209, 0x0081 }, { 0x4209, 0x0101 }, { 0x4209, 0x0201 }, { 0x4209, 0x0401 }, { 0x4209, 0x0801 }, { 0x4209, 0x1001 }, { 0x4289, 0x0001 }, { 0x4309, 0x0001 } }
  },
  // Down
  {
  { { 0x5001, 0x0083 }, { 0x5001, 0x0105 }, { 0x5001, 0x0209 }, { 0x5001, 0x0411 }, { 0x5001, 0x0821 }, { 0x5001, 0x1041 }, { 0x5083, 0x0001 }, { 0x5105, 0x0001 } },
  { { 0x4801, 0x0083 }, { 0x4801, 0x0105 }, { 0x4801, 0x0209 }, { 0x4801, 0x0411 }, { 0x4801, 0x0821 }, { 0x4801, 0x1041 }, { 0x4883, 0x0001 }, { 0x4905, 0x0001 } },
  { { 0x4401, 0x0083 }, { 0x4401, 0x0105 }, { 0x4401, 0x0209 }, { 0x4401, 0x0411 }, { 0x4401, 0x0821 }, { 0x4401, 0x1041 }, { 0x4483, 0x0001 }, { 0x4505, 0x0001 } },
  { { 0x4201, 0x0083 }, { 0x4201, 0x0105 }, { 0x4201, 0x0209 }, { 0x4201, 0x0411 }, { 0x4201, 0x0821 }, { 0x4201, 0x1041 }, { 0x4283, 0x0001 }, { 0x4305, 0x0001 } }
  }
};


static void delay(uint32_t t) {
    jd_services_sleep_us(t * 1000);
}

uint16_t tx = 0;
uint16_t rx = 0;
int done = 0;

static inline uint16_t flip (uint16_t v) {
    return ((v & 0xff00) >> 8) | ((v & 0x00ff) << 8);
}

void BrClrAllDots(srv_t* state);
void BrSetSingleDot(srv_t* state, uint16_t row, uint16_t col, bool set);

// Braille
bool BrDotPtn[MAXBRROW][MAXBRCOL] = {false};

/*
 * Procs.
 */
// Braille Module Control
// BrClrPtn: Clear Braille Pattern
void BrClrPtn(void) {
    unsigned char row, col;

    for (row = 0; row < MAXBRROW; row++) {
        for (col = 0; col < MAXBRCOL; col++) {
            BrDotPtn[row][col] = false;
        }
    }
}

// BrSetPtn: SetBraille Pattern
void BrSetPtn(uint16_t row, uint16_t col, bool state) {
    if (row >= MAXBRROW || col >= MAXBRCOL)
        hw_panic();

    BrDotPtn[row][col] = state;
}

// BrRfshPtn: Refresh and load new braille pattern
void BrRfshPtn(srv_t* state) {
    unsigned char row, col;

    // force clear
    BrClrAllDots(state);

    // load new pattern
    for (row = 0; row < MAXBRROW; row++) {
        for (col = 0; col < MAXBRCOL; col++) {
            // only set direction
            if (BrDotPtn[row][col] == true) {
                BrSetSingleDot(state, row, col, true);
            }
        }
    }
}

// BrClrAllDots: Clear All Dots (forced)
void BrClrAllDots(srv_t * state) {
    unsigned char row, col, bk;

    for (row = 0; row < MAXBRROW; row++) {
        for (col = 0; col < MAXBRCOL; col++) {
            // reset dot
            for (bk = 0; bk < MAXBRBANK; bk++) {
                state->api->write_raw(BrDotTbl_forced[BRPHASEDN][row][col][bk]);
            }
            delay(BRWAITSETFD);
            // disconnect all cells for pre discharge
            state->api->write_raw(0x4001);
            state->api->write_raw(0x0001);
            delay(BRWAITDIS1);
            // discharge ( all connect to LS )
            state->api->write_raw(0x5F81);
            state->api->write_raw(0x1F81);
            delay(BRWAITDIS2);
            // disconnect all cells
            state->api->write_raw(0x4001);
            state->api->write_raw(0x0001);
            delay(BRWAITFHIZ);
        }
    }
}

// BrSetAllDots: Set All Dots (forced)
void BrSetAllDots(srv_t * state) {
    uint16_t row, col, bk;

    for (row = 0; row < MAXBRROW; row++) {
        for (col = 0; col < MAXBRCOL; col++) {
            // reset dot
            for (bk = 0; bk < MAXBRBANK; bk++) {
                state->api->write_raw(BrDotTbl_forced[BRPHASEUP][row][col][bk]);
            }
            delay(BRWAITSETFU);
            // disconnect all cells for pre discharge
            state->api->write_raw(0x4001);
            state->api->write_raw(0x0001);
            delay(BRWAITDIS1);
            // discharge ( all connect to LS )
            state->api->write_raw(0x5F81);
            state->api->write_raw(0x1F81);
            delay(BRWAITDIS2);
            // disconnect all cells
            state->api->write_raw(0x4001);
            state->api->write_raw(0x0001);
            delay(BRWAITFHIZ);
        }
    }
}

// BrSetSingleDot: set single dot (true: set, false: clear): do NOT use directly, may have
// instability
void BrSetSingleDot(srv_t * state, uint16_t row, uint16_t col, bool set) {
    uint16_t bk;
    uint8_t ph;
    int delaytime;

    // set phase
    if (set == true) {
        ph = BRPHASEUP;
    } else {
        ph = BRPHASEDN;
    }

    // set/clr dot
    for (bk = 0; bk < MAXBRBANK; bk++) {
        state->api->write_raw(BrDotTbl[ph][row][col][bk]);
    }

    // load delaytime
    if (set == true) {
        delaytime = BRWAITSETU;
    } else {
        delaytime = BRWAITSETD;
    }

    /*
    // add special delay for specified dots (need to adjust for each module)
    // for module 000
    if ( ( state == false ) && ( (( row == 0 ) && ( col == 1 )) || (( row == 3 ) && ( col == 6 )) )
    ) { delaytime = BRWAITSETD1;
    }
    */

    delay(delaytime);

    // disconnect all cells for pre discharge
    state->api->write_raw(0x4001);
    state->api->write_raw(0x0001);
    delay(BRWAITDIS1);

    // discharge ( all connect to LS )
    state->api->write_raw(0x5F81);
    state->api->write_raw(0x1F81);
    delay(BRWAITDIS2);

    // disconnect all cells
    state->api->write_raw(0x4001);
    state->api->write_raw(0x0001);
    delay(BRWAITHIZ);
}

void braille_process(srv_t * state) {
    if (state->flags & STATE_DIRTY) {
        BrRfshPtn(state);
        state->flags &= ~STATE_DIRTY;
    }
}

bool get_bit(srv_t * state, uint8_t* data, uint16_t row, uint16_t col) {
    uint16_t cols_padded = state->cols + (8 - (state->cols % 8));
    uint32_t bitindex = row * cols_padded + col;
    uint8_t byte = data[bitindex >> 3];
    uint8_t bit = bitindex % 8;
    return (1 == ((byte >> bit) & 1));
}

// translates from sim to real life accurately.
const uint8_t cell_map[] = {2,0,1,3};

void handle_disp_write(srv_t * state, jd_packet_t* pkt) {

    // update in progress
    if (state->flags & STATE_DIRTY)
        return;

    // too big for our buffer
    if (pkt->service_size > DOTS_MAX)
        hw_panic();

    BrClrPtn();
    
    for (int row = 0; row < state->rows; row++) 
        for (int col = 0; col < state->cols; col++) {
            uint8_t cell = cell_map[col / 2];
            bool upper = (col%2);
            if (get_bit(state, pkt->data, row, col)) {
                BrSetPtn(cell, (upper) ? 4 + row : row, true);
                DMESG("SET Cell %d DOT %d", cell, (upper) ? 4 + row : row);
            }
        }

    // memcpy(state->dots, pkt->data, pkt->service_size);

    state->flags |= STATE_DIRTY;
}

void braille_handle_packet(srv_t *state, jd_packet_t *pkt) {

    switch (pkt->service_command) {
        default:
            switch (service_handle_register(state, pkt, braille_regs)) {
                case JD_DOT_MATRIX_REG_DOTS:
                    if (JD_SET(JD_DOT_MATRIX_REG_DOTS) == pkt->service_command)
                        handle_disp_write(state, pkt);
                    break;
            }
    }
}

SRV_DEF(braille, JD_SERVICE_CLASS_DOT_MATRIX);
void braille_init(const hbridge_api_t *params, uint16_t rows, uint16_t cols, uint8_t variant) {
    SRV_ALLOC(braille);

    state->api = params;
    state->rows = rows;
    state->cols = cols;
    state->variant = variant;
    state->flags = 0;
    state->now = 0;

    state->api->init();

    uint16_t dot_bytes = 8 * cols; // this will need fixed up if we expand in future

    if (dot_bytes > DOTS_MAX)
        hw_panic();

    memset(state->dots, 0, DOTS_MAX);

    BrClrPtn();

    BrClrAllDots(state);
    delay(3000);
}