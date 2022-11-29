#include "jd_drivers.h"
#include "services/jd_services.h"

#if defined(SPI_RX) && defined(PIN_CS)

#define ASSERT JD_ASSERT

// #define DBG DMESG
#define DBG(...) ((void)0)

#define DRIVE_LS_ON 0
#define DRIVE_HS_ON 1
#define OUT_COUNT 12

#define SRR (1 << 15)
#define HB_SEL (1 << 14)
#define OVLO (1 << 0)

#define EN_START 0x80
#define CNF_START 0x2

static uint32_t driver_state_en = 0;
static uint32_t driver_state_ls = 0;
static uint32_t driver_state_hs = 0;

static uint32_t ncv7726b_tx = 0;
static uint32_t ncv7726b_resp = 0;
static uint16_t command = 0;
static volatile uint8_t tx_complete = 0;

static void ncv7726b_reset(void);

static inline uint16_t flip(uint16_t v) {
    return ((v & 0xff00) >> 8) | ((v & 0x00ff) << 8);
}

static void ncv7726b_xfer_done(void) {
    ncv7726b_resp = flip(ncv7726b_resp);
    pin_set(PIN_CS, 1);
    tx_complete = 1;
}

static void ncv7726b_send(uint16_t *data) {
    tx_complete = 0;
    command = *data;
    ncv7726b_tx = flip(*data); // ls bit first
    pin_set(PIN_CS, 0);
    target_wait_us(10);
    dspi_xfer(&ncv7726b_tx, &ncv7726b_resp, 2, ncv7726b_xfer_done);
    while (tx_complete == 0)
        ;
}

static uint16_t ncv7726b_write_raw(uint16_t d) {
    ncv7726b_send(&d);
    return ncv7726b_resp;
}

static void ncv7726b_write_state(void) {
    // calculate the lower 16 bits
    uint16_t lower = 0; // OVLO;
    DBG("EN %x Hs %x LS %x", driver_state_en, driver_state_hs, driver_state_ls);
    // OUT channels aren't indexed from one...
    for (int i = 1; i < 7; i++) {
        DBG("COL%d EN %d", i, driver_state_en & (1 << i));
        if (driver_state_en & (1 << i)) {
            lower |= EN_START << (i - 1);
            DBG("ENL %x", EN_START << (i - 1));
            // drive direction needs to be explicit otherwise driver could
            // cause attached component to get hot.
            if (!(driver_state_ls & (1 << i)) && !(driver_state_hs & (1 << i)))
                JD_PANIC();

            if ((driver_state_ls & (1 << i)) && (driver_state_hs & (1 << i)))
                JD_PANIC();

            if (driver_state_ls & (1 << i)) {
                DBG("LSL %x", CNF_START << (i - 1));
                lower &=
                    ~(CNF_START
                      << (i - 1)); // subtract 1 so the shift is correct (OUT1 is BIT 1 of lower 16)
            }

            if (driver_state_hs & (1 << i)) {
                DBG("LSH %x", CNF_START << (i - 1));
                lower |= CNF_START << (i - 1); // subtract 1 so the shift is correct
            }
        }
    }

    // calculate the upper 16 bits
    uint16_t upper = HB_SEL; // | OVLO;

    for (int i = 7; i < OUT_COUNT + 1; i++) {
        // DBG("COL%d EN %d", i, driver_state_en & (1 << i));
        if (driver_state_en & (1 << i)) {
            upper |= EN_START << (i - 7);
            DBG("ENH %x", 1 << ((i - 7) + EN_START));
            // drive direction needs to be explicit otherwise driver could
            // cause attached component to get hot.
            if (!(driver_state_ls & (1 << i)) && !(driver_state_hs & (1 << i)))
                JD_PANIC();

            if ((driver_state_ls & (1 << i)) && (driver_state_hs & (1 << i)))
                JD_PANIC();

            if (driver_state_ls & (1 << i)) {
                DBG("LSH %x", CNF_START << (i - 7));
                upper &= ~(CNF_START << (i - 7)); // subtract 7 so the shift is correct (i.e. OUT7
                                                  // is BIT1 of the upper 16)
            }

            if (driver_state_hs & (1 << i)) {
                DBG("HSH %x", CNF_START << (i - 7));
                upper |= CNF_START << (i - 7); // subtract 1 so the shift is correct
            }
        }
    }

    // if (driver_state_en == 0 && driver_state_ls == 0 && driver_state_hs == 0) {
    //     uint16_t reset = SRR;
    //     ncv7726b_send(&reset);
    //     target_wait_us(3000);
    //     reset =  SRR | HB_SEL;
    //     ncv7726b_send(&reset);
    // } else {
    ncv7726b_send(&upper);
    target_wait_us(10);
    ncv7726b_send(&lower);
    target_wait_us(8000);

    uint16_t reset = SRR | HB_SEL;
    ncv7726b_send(&reset);
    target_wait_us(200);
    reset = SRR;
    ncv7726b_send(&reset);
    target_wait_us(8000);
}

static void ncv7726b_channel_set(uint8_t channel, int state) {

    ASSERT(channel <= OUT_COUNT && channel > 0);

    int msk = (1 << channel);

    driver_state_en |= msk;

    if (state == 0) {
        driver_state_hs &= ~msk;
        driver_state_ls |= msk;
    } else {
        driver_state_ls &= ~msk;
        driver_state_hs |= msk;
    }
}

static void ncv7726b_channel_clear(uint8_t channel) {
    ASSERT(channel <= OUT_COUNT && channel > 0);

    int msk = (1 << channel);

    driver_state_en &= ~msk;
    driver_state_ls &= ~msk;
    driver_state_hs &= ~msk;
}

static void ncv7726b_clear_all(void) {
    driver_state_en = 0;
    driver_state_ls = 0;
    driver_state_hs = 0;
}

static void ncv7726b_reset(void) {
    uint16_t reset = SRR;
    ncv7726b_send(&reset);
    target_wait_us(3000);
    reset = SRR | HB_SEL;
    ncv7726b_send(&reset);
    target_wait_us(3000);
}

static void ncv7726b_init(void) {
    dspi_init(false, 0, 1);
    pin_setup_output(PIN_CS);
    ncv7726b_reset();
    ncv7726b_clear_all();
}

const hbridge_api_t ncv7726b = {.init = ncv7726b_init,
                                .channel_clear = ncv7726b_channel_clear,
                                .channel_set = ncv7726b_channel_set,
                                .clear_all = ncv7726b_clear_all,
                                .write_channels = ncv7726b_write_state,
                                .write_raw = ncv7726b_write_raw};

#endif