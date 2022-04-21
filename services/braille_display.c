#include "jd_services.h"
#include "interfaces/jd_pins.h"
#include "jacdac/dist/c/brailledisplay.h"

#define STATE_DIRTY 0x01

#define DOTS_MAX 76

struct srv_state {
    SRV_COMMON;
    uint8_t enabled;
    uint8_t length;
    uint8_t flags;
    const hbridge_api_t *api;
    braille_get_channels_t get_channels;
    uint8_t dots[DOTS_MAX];
    uint8_t tmpbuf[JD_SERIAL_PAYLOAD_SIZE];
};

REG_DEFINITION(                             //
    braile_display_regs,                    //
    REG_SRV_COMMON,                         //
    REG_U8(JD_BRAILLE_DISPLAY_REG_ENABLED), //
    REG_U8(JD_BRAILLE_DISPLAY_REG_LENGTH),  //
)

/*
From Unicode:
1 4
2 5
3 6
7 8
*/
static const uint8_t masks[] = {0x01, 0x02, 0x04, 0x40, 0x08, 0x10, 0x20, 0x80};

static void braile_display_send(srv_t *state) {
    const hbridge_api_t *api = state->api;

    for (int col = 0; col < state->length * 2; ++col) {
        for (int row = 0; row < 4; ++row) {
            api->clear_all();

            uint32_t ch = state->get_channels(row, col);
            uint16_t ch0 = ch & 0xffff;
            uint16_t ch1 = (ch >> 16) & 0xffff;

            if (state->dots[col >> 1] & masks[row + ((col & 1) ? 4 : 0)]) {
                api->channel_set(ch0, 1);
                api->channel_set(ch1, 0);
            } else {
                api->channel_set(ch0, 0);
                api->channel_set(ch1, 1);
            }

            api->write_channels();
            jd_services_sleep_us(7000);
            api->clear_all();
            api->write_channels();
            jd_services_sleep_us(4000);
        }
    }
}

static void handle_get(srv_t *state) {
    int dst = 0;
    int len = state->length;
    // don't send spaces at the end
    while (len > 0 && state->dots[len - 1] == 0)
        len--;
    for (int i = 0; i < len; ++i) {
        uint8_t low = state->dots[i];
        state->tmpbuf[dst++] = 0xE2;
        state->tmpbuf[dst++] = 0xA0 | (low >> 6);
        state->tmpbuf[dst++] = 0x80 | (low & 0x3F);
    }
    jd_send(state->service_index, JD_GET(JD_BRAILLE_DISPLAY_REG_PATTERNS), state->tmpbuf, dst);
}

void braile_display_process(srv_t *state) {
    if (state->flags & STATE_DIRTY) {
        state->flags &= ~STATE_DIRTY; // it's possible we'll get another set while we're updating;
                                      // we want that set to set the dirty flag again
        if (state->enabled)
            braile_display_send(state);
        handle_get(state); // make sure the UI refreshes
    }
}

static void handle_set(srv_t *state, jd_packet_t *pkt) {
    int ptr = 0;
    for (int i = 0; i < pkt->service_size + 2; i += 3) {
        if (pkt->data[i] != 0xE2)
            break;
        if ((pkt->data[i + 1] & 0xFC) != 0xA0)
            break;
        if ((pkt->data[i + 2] & 0xC0) != 0x80)
            break;
        state->dots[ptr++] = ((pkt->data[i + 1] & 3) << 6) | (pkt->data[i + 2] & 0x3f);
        if (ptr >= state->length)
            break;
    }
    while (ptr < state->length)
        state->dots[ptr++] = 0;
    state->flags |= STATE_DIRTY;
}

void braile_display_handle_packet(srv_t *state, jd_packet_t *pkt) {
    switch (pkt->service_command) {
    case JD_SET(JD_BRAILLE_DISPLAY_REG_PATTERNS):
        handle_set(state, pkt);
        break;
    case JD_GET(JD_BRAILLE_DISPLAY_REG_PATTERNS):
        handle_get(state);
        break;
    default:
        service_handle_register_final(state, pkt, braile_display_regs);
    }
}

SRV_DEF(braile_display, JD_SERVICE_CLASS_BRAILLE_DISPLAY);
void braille_display_init(const hbridge_api_t *params, uint8_t length,
                          braille_get_channels_t get_channels) {
    SRV_ALLOC(braile_display);

    state->api = params;
    state->enabled = 1;
    state->length = length;
    state->flags = 0;
    state->get_channels = get_channels;

    state->api->init();

    JD_ASSERT(length < DOTS_MAX);

    memset(state->dots, 0, DOTS_MAX);

    braile_display_send(state);
    target_wait_us(3000);
}