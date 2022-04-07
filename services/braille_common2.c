#include "braille_common.h"

#ifndef BRAILLE_HACK

#define NUM_COLUMNS 8
#define NUM_ROWS 4

// Braille
static uint8_t columns[NUM_COLUMNS];

/*
 * Procs.
 */
// Braille Module Control
// BrClrPtn: Clear Braille Pattern
void BrClrPtn(void) {
    memset(columns, 0, sizeof(columns));
}

bool BrGetPtn(uint16_t row, uint16_t col) {
    if (row >= NUM_ROWS || col >= NUM_COLUMNS)
        hw_panic();

    return (columns[col] & (1 << row)) != 0;
}

// BrSetPtn: SetBraille Pattern
void BrSetPtn(uint16_t row, uint16_t col, bool state) {
    if (row >= NUM_ROWS || col >= NUM_COLUMNS)
        hw_panic();

    if (state)
        columns[col] |= 1 << row;
    else
        columns[col] &= ~(1 << row);
}

static int col_low(int col) {
    return (NUM_COLUMNS * NUM_ROWS) + (col >> 1);
}

static int col_high(int row, int col) {
    return col * NUM_ROWS + row;
}

// BrRfshPtn: Refresh and load new braille pattern
void BrRfshPtn(const hbridge_api_t *api) {
    for (int col = 0; col < NUM_COLUMNS; col += 2) {
        api->clear_all();
        api->channel_set(col_low(col), 0);
        for (int row = 0; row < NUM_ROWS; ++row) {
            if (BrGetPtn(row, col))
                api->channel_set(col_high(row, col), 1);
            if (BrGetPtn(row, col + 1))
                api->channel_set(col_high(row, col + 1), 1);
        }

        api->write_channels();
        jd_services_sleep_us(7000);
        api->clear_all();
        api->write_channels();
        jd_services_sleep_us(4000);

        api->channel_set(col_low(col), 1);
        for (int row = 0; row < NUM_ROWS; ++row) {
            if (!BrGetPtn(row, col))
                api->channel_set(col_high(row, col), 0);
            if (!BrGetPtn(row, col + 1))
                api->channel_set(col_high(row, col + 1), 0);
        }

        api->write_channels();
        jd_services_sleep_us(7000);
        api->clear_all();
        api->write_channels();
        jd_services_sleep_us(4000);
    }
}

#endif