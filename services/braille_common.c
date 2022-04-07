#include "braille_common.h"

#ifdef BRAILLE_HACK

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

const uint16_t BrDotTbl[MAXBRPHASE][MAXBRROW][MAXBRCOL][MAXBRBANK] = {
    // Braittle module table ( ghost hack ( single GND ))
    // Up
    {{{0x5841, 0x0081},
      {0x5841, 0x0101},
      {0x5841, 0x0201},
      {0x5841, 0x0401},
      {0x5841, 0x0801},
      {0x5841, 0x1001},
      {0x58C1, 0x0001},
      {0x5941, 0x0001}},
     {{0x5821, 0x0081},
      {0x5821, 0x0101},
      {0x5821, 0x0201},
      {0x5821, 0x0401},
      {0x5821, 0x0801},
      {0x5821, 0x1001},
      {0x58A1, 0x0001},
      {0x5921, 0x0001}},
     {{0x4611, 0x0081},
      {0x4611, 0x0101},
      {0x4611, 0x0201},
      {0x4611, 0x0401},
      {0x4611, 0x0801},
      {0x4611, 0x1001},
      {0x4691, 0x0001},
      {0x4711, 0x0001}},
     {{0x4609, 0x0081},
      {0x4609, 0x0101},
      {0x4609, 0x0201},
      {0x4609, 0x0401},
      {0x4609, 0x0801},
      {0x4609, 0x1001},
      {0x4689, 0x0001},
      {0x4709, 0x0001}}},
    // Down
    {{{0x5839, 0x0083},
      {0x5839, 0x0105},
      {0x5839, 0x0209},
      {0x5839, 0x0411},
      {0x5839, 0x0821},
      {0x5839, 0x1041},
      {0x5883, 0x0001},
      {0x5905, 0x0001}},
     {{0x5859, 0x0083},
      {0x5859, 0x0105},
      {0x5859, 0x0209},
      {0x5859, 0x0411},
      {0x5859, 0x0821},
      {0x5859, 0x1041},
      {0x5883, 0x0001},
      {0x5905, 0x0001}},
     {{0x4669, 0x0083},
      {0x4669, 0x0105},
      {0x4669, 0x0209},
      {0x4669, 0x0411},
      {0x4669, 0x0821},
      {0x4669, 0x1041},
      {0x4683, 0x0001},
      {0x4705, 0x0001}},
     {{0x4671, 0x0083},
      {0x4671, 0x0105},
      {0x4671, 0x0209},
      {0x4671, 0x0411},
      {0x4671, 0x0821},
      {0x4671, 0x1041},
      {0x4683, 0x0001},
      {0x4705, 0x0001}}}};

const uint16_t BrDotTbl_forced[MAXBRPHASE][MAXBRROW][MAXBRCOL][MAXBRBANK] = {
    // Braittle module table ( original XY, ghost may occur )
    // Up
    {{{0x5041, 0x0081},
      {0x5041, 0x0101},
      {0x5041, 0x0201},
      {0x5041, 0x0401},
      {0x5041, 0x0801},
      {0x5041, 0x1001},
      {0x50C1, 0x0001},
      {0x5141, 0x0001}},
     {{0x4821, 0x0081},
      {0x4821, 0x0101},
      {0x4821, 0x0201},
      {0x4821, 0x0401},
      {0x4821, 0x0801},
      {0x4821, 0x1001},
      {0x48A1, 0x0001},
      {0x4921, 0x0001}},
     {{0x4411, 0x0081},
      {0x4411, 0x0101},
      {0x4411, 0x0201},
      {0x4411, 0x0401},
      {0x4411, 0x0801},
      {0x4411, 0x1001},
      {0x4491, 0x0001},
      {0x4511, 0x0001}},
     {{0x4209, 0x0081},
      {0x4209, 0x0101},
      {0x4209, 0x0201},
      {0x4209, 0x0401},
      {0x4209, 0x0801},
      {0x4209, 0x1001},
      {0x4289, 0x0001},
      {0x4309, 0x0001}}},
    // Down
    {{{0x5001, 0x0083},
      {0x5001, 0x0105},
      {0x5001, 0x0209},
      {0x5001, 0x0411},
      {0x5001, 0x0821},
      {0x5001, 0x1041},
      {0x5083, 0x0001},
      {0x5105, 0x0001}},
     {{0x4801, 0x0083},
      {0x4801, 0x0105},
      {0x4801, 0x0209},
      {0x4801, 0x0411},
      {0x4801, 0x0821},
      {0x4801, 0x1041},
      {0x4883, 0x0001},
      {0x4905, 0x0001}},
     {{0x4401, 0x0083},
      {0x4401, 0x0105},
      {0x4401, 0x0209},
      {0x4401, 0x0411},
      {0x4401, 0x0821},
      {0x4401, 0x1041},
      {0x4483, 0x0001},
      {0x4505, 0x0001}},
     {{0x4201, 0x0083},
      {0x4201, 0x0105},
      {0x4201, 0x0209},
      {0x4201, 0x0411},
      {0x4201, 0x0821},
      {0x4201, 0x1041},
      {0x4283, 0x0001},
      {0x4305, 0x0001}}}};

static void delay(uint32_t t) {
    jd_services_sleep_us(t * 1000);
}

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

bool BrGetPtn(uint16_t row, uint16_t col) {
    if (row >= MAXBRROW || col >= MAXBRCOL)
        hw_panic();

    return BrDotPtn[row][col];
}

// BrSetPtn: SetBraille Pattern
void BrSetPtn(uint16_t row, uint16_t col, bool state) {
    if (row >= MAXBRROW || col >= MAXBRCOL)
        hw_panic();

    BrDotPtn[row][col] = state;
}

// BrRfshPtn: Refresh and load new braille pattern
void BrRfshPtn(const hbridge_api_t *api) {
    unsigned char row, col;

    // force clear
    BrClrAllDots(api);

    // load new pattern
    for (row = 0; row < MAXBRROW; row++) {
        for (col = 0; col < MAXBRCOL; col++) {
            // only set direction
            if (BrDotPtn[row][col] == true) {
                BrSetSingleDot(api, row, col, true);
            }
        }
    }
}

// BrClrAllDots: Clear All Dots (forced)
void BrClrAllDots(const hbridge_api_t *api) {
    unsigned char row, col, bk;

    for (row = 0; row < MAXBRROW; row++) {
        for (col = 0; col < MAXBRCOL; col++) {
            // reset dot
            for (bk = 0; bk < MAXBRBANK; bk++) {
                api->write_raw(BrDotTbl_forced[BRPHASEDN][row][col][bk]);
            }
            delay(BRWAITSETFD);
            // disconnect all cells for pre discharge
            api->write_raw(0x4001);
            api->write_raw(0x0001);
            delay(BRWAITDIS1);
            // discharge ( all connect to LS )
            api->write_raw(0x5F81);
            api->write_raw(0x1F81);
            delay(BRWAITDIS2);
            // disconnect all cells
            api->write_raw(0x4001);
            api->write_raw(0x0001);
            delay(BRWAITFHIZ);
        }
    }
}

// BrSetAllDots: Set All Dots (forced)
void BrSetAllDots(const hbridge_api_t *api) {
    uint16_t row, col, bk;

    for (row = 0; row < MAXBRROW; row++) {
        for (col = 0; col < MAXBRCOL; col++) {
            // reset dot
            for (bk = 0; bk < MAXBRBANK; bk++) {
                api->write_raw(BrDotTbl_forced[BRPHASEUP][row][col][bk]);
            }
            delay(BRWAITSETFU);
            // disconnect all cells for pre discharge
            api->write_raw(0x4001);
            api->write_raw(0x0001);
            delay(BRWAITDIS1);
            // discharge ( all connect to LS )
            api->write_raw(0x5F81);
            api->write_raw(0x1F81);
            delay(BRWAITDIS2);
            // disconnect all cells
            api->write_raw(0x4001);
            api->write_raw(0x0001);
            delay(BRWAITFHIZ);
        }
    }
}

// BrSetSingleDot: set single dot (true: set, false: clear): do NOT use directly, may have
// instability
void BrSetSingleDot(const hbridge_api_t *api, uint16_t row, uint16_t col, bool set) {
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
        api->write_raw(BrDotTbl[ph][row][col][bk]);
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
    api->write_raw(0x4001);
    api->write_raw(0x0001);
    delay(BRWAITDIS1);

    // discharge ( all connect to LS )
    api->write_raw(0x5F81);
    api->write_raw(0x1F81);
    delay(BRWAITDIS2);

    // disconnect all cells
    api->write_raw(0x4001);
    api->write_raw(0x0001);
    delay(BRWAITHIZ);
}

#endif