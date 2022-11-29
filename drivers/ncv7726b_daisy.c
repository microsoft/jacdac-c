#include "jd_drivers.h"
#include "services/jd_services.h"

#define NUM_CHIPS 3 // TODO from JD_CONFIG

#define CH_PER_CHIP 12
#define NUM_CHANNELS (CH_PER_CHIP * NUM_CHIPS)

#if defined(PIN_SCS)

// #define DBG DMESG
#define DBG(...) ((void)0)

#define SRR (1 << 15)
#define HB_SEL (1 << 14)
#define ULDSC (1 << 13)
#define OVLO (1 << 0)

static uint16_t channel_en[NUM_CHIPS];
static uint16_t channel_hs[NUM_CHIPS];

static inline uint16_t flip(uint16_t v) {
    return ((v & 0xff00) >> 8) | ((v & 0x00ff) << 8);
}

static void ncv7726b_write_state(void) {
    uint16_t base = SRR;

    for (int shift = 0; shift <= 6; shift += 6) {
        pin_set(PIN_SCS, 0);
        target_wait_us(10);
        for (int no = NUM_CHIPS - 1; no >= 0; no--) {
            uint16_t en = (channel_en[no] >> shift) & 0x3f;
            uint16_t hs = (channel_hs[no] >> shift) & 0x3f;
            uint16_t cmd = base | (en << 7) | (hs << 1);
            cmd = flip(cmd);
            sspi_tx(&cmd, 2);
        }
        pin_set(PIN_SCS, 1);

        base |= HB_SEL;
    }
}

static int chip_no(uint8_t ch, uint16_t *mask) {
    JD_ASSERT(ch < NUM_CHANNELS);

    for (int i = 0; i < NUM_CHIPS; ++i) {
        if (ch < CH_PER_CHIP) {
            *mask = 1 << ch;
            return i;
        }
        ch -= CH_PER_CHIP;
    }

    JD_PANIC();
}

static void ncv7726b_channel_set(uint8_t channel, int state) {
    uint16_t mask;
    int no = chip_no(channel, &mask);
    channel_en[no] |= mask;
    if (state) {
        channel_hs[no] |= mask;
    } else {
        channel_hs[no] &= ~mask;
    }
}

static void ncv7726b_channel_clear(uint8_t ch) {
    uint16_t mask;
    int no = chip_no(ch, &mask);
    channel_en[no] &= ~mask;
}

static void ncv7726b_clear_all(void) {
    memset(&channel_en, 0, sizeof(channel_en));
    memset(&channel_hs, 0, sizeof(channel_hs)); // for clarity of trace
}

static void ncv7726b_init(void) {
    sspi_init(1, 0, 1);
    pin_set(PIN_SCS, 1);
    pin_setup_output(PIN_SCS);
}

const hbridge_api_t ncv7726b_daisy = {.init = ncv7726b_init,
                                      .channel_clear = ncv7726b_channel_clear,
                                      .channel_set = ncv7726b_channel_set,
                                      .clear_all = ncv7726b_clear_all,
                                      .write_channels = ncv7726b_write_state};

#endif