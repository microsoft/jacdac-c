#include "jd_services.h"
#include "interfaces/jd_pins.h"
#include "jacdac/dist/c/characterscreen.h"
#include "braille_common.h"

#define STATE_DIRTY 0x01
#define STATE_UPD_PENDING 0x02

static const uint8_t braille_number = 0b01110100;

// numbers are preceded by the pattern above
static const uint8_t ASCII_BRAILLE_MAP[] = {
    0b00000001, // "Aa" / 1
    0b00000011, // "Bb" / 2
    0b00010001, // "Cc" / 3
    0b00110001, // "Dd" / 4
    0b00100001, // "Ee" / 5
    0b00010011, // "Ff" / 6
    0b00110011, // "Gg" / 7
    0b00100011, // "Hh" / 8
    0b00010010, // "Ii" / 9
    0b00110010, // "Jj" / 0
    0b00000101, // "Kk"
    0b00000111, // "Ll"
    0b00010101, // "Mm"
    0b00110101, // "Nn"
    0b00100101, // "Oo"
    0b00010111, // "Pp"
    0b00110111, // "Qq"
    0b00100111, // "Rr"
    0b00010110, // "Ss"
    0b00110110, // "Tt"
    0b01000101, // "Uu"
    0b01000111, // "Vv"
    0b01110010, // "Ww"
    0b01010101, // "Xx"
    0b01110101, // "Yy"
    0b01100101, // "Zz"
};

struct srv_state {
    SRV_COMMON;
    const hbridge_api_t *api;
    uint8_t rows;
    uint8_t cols;
    uint8_t variant;
    uint8_t text_dir;
    char string[60];
    uint8_t flags;
    const uint8_t *cell_map;
};

REG_DEFINITION(                                     //
    braille_char_regs,                              //
    REG_SRV_COMMON,                                 //
    REG_PTR_PADDING(),                              //
    REG_U8(JD_CHARACTER_SCREEN_REG_ROWS),           //
    REG_U8(JD_CHARACTER_SCREEN_REG_COLUMNS),        //
    REG_U8(JD_CHARACTER_SCREEN_REG_VARIANT),        //
    REG_U8(JD_CHARACTER_SCREEN_REG_TEXT_DIRECTION), //
    REG_BYTES(JD_CHARACTER_SCREEN_REG_MESSAGE, 4),  //
)
static void set_pattern(srv_t *state, uint8_t cell, uint8_t bit_pattern) {
    cell = state->cell_map[cell];

    for (int bit = 0; bit < 8; bit++) {
        if (bit_pattern & (1 << bit))
            BrSetPtn(cell, bit, true);
    }
}

static inline int min(int a, int b) {
    return a < b ? a : b;
}

static void update_bitmask(srv_t *state) {
    BrClrPtn();

    int i = 0;
    int cell = 0;

    int len = 0;

    char *ptr = state->string;
    while (*(ptr++) >= ' ')
        len++;

    while (cell < min(state->cols, len)) {
        uint8_t current = state->string[i];
        // if ascii A-Z/a-z snap to zero based index.
        if (current >= 'A' && current <= 'Z')
            current -= 'A';
        else if (current >= 'a' && current <= 'z')
            current -= 'a';
        // numbers require special handling
        else if (current >= '0' && current <= '9') {
            //
            if (i > 0 && (state->string[i - 1] >= '0' && state->string[i - 1] <= '9')) {
                // number pattern already sent
            } else
                set_pattern(state, cell++, braille_number);
            if (current == '0')
                current = ':'; // 0 == ascii j in our map

            current -= '1'; // ascii 1 is our new 0 index...
        } else {
            // q mark by default
            current = 0b01000110; // ?

            switch (state->string[i]) {
            case ',':
                current = 0b00000010; // ,
                break;
            case ';':
                current = 0b00000110; // ;
                break;
            case ':':
                current = 0b00100010; // :
                break;
            case '.':
                current = 0b01100010; // .
                break;
            case '!':
                current = 0b00100110; // !
                break;
            case '(':
            case ')':
                current = 0b01100110; // !
                break;
            case '"':
                current = 0b01000110;
                break;
            case '_':
                current = 0b01000100; // _
                break;
            }

            set_pattern(state, cell++, current);
            i++;
            continue;
        }

        set_pattern(state, cell++, ASCII_BRAILLE_MAP[current]);
        i++;
    }

    state->flags |= STATE_DIRTY;
}

void braille_char_process(srv_t *state) {
    if (state->flags & STATE_DIRTY) {
        BrRfshPtn(state->api);
        state->flags &= ~STATE_DIRTY;
    }

    if (state->flags & STATE_UPD_PENDING) {
        update_bitmask(state);
        state->flags &= ~STATE_UPD_PENDING;
    }
}

void handle_disp_write(srv_t *state, jd_packet_t *pkt) {
    memset(state->string, 0, sizeof(state->string));
    memcpy(state->string, pkt->data, min(pkt->service_size, state->cols));

    // update already in progress
    if (state->flags & STATE_DIRTY) {
        state->flags |= STATE_UPD_PENDING;
        return;
    }

    update_bitmask(state);
}

void braille_char_handle_packet(srv_t *state, jd_packet_t *pkt) {

    switch (pkt->service_command) {
    default:
        switch (service_handle_register_final(state, pkt, braille_char_regs)) {
        case JD_CHARACTER_SCREEN_REG_MESSAGE:
            if (JD_SET(JD_CHARACTER_SCREEN_REG_MESSAGE) == pkt->service_command)
                handle_disp_write(state, pkt);
            break;
        }
    }
}

SRV_DEF(braille_char, JD_SERVICE_CLASS_CHARACTER_SCREEN);
void braille_char_init(const hbridge_api_t *params, uint16_t rows, uint16_t cols,
                       const uint8_t *cell_map) {
    SRV_ALLOC(braille_char);

    state->api = params;
    state->rows = rows;
    state->cols = cols;
    state->variant = JD_CHARACTER_SCREEN_VARIANT_BRAILLE;
    state->text_dir = JD_CHARACTER_SCREEN_TEXT_DIRECTION_LEFT_TO_RIGHT;
    state->flags = 0;
    state->cell_map = cell_map;
    memset(state->string, 0, sizeof(state->string));

    state->api->init();

    BrClrPtn();
    BrRfshPtn(state->api);
    target_wait_us(3000);
}