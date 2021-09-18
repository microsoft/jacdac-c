#define DRIVE_LS_ON     0
#define DRIVE_HS_ON     1
#define OUT_COUNT       12

#define HB_SEL  (1 << 14)
#define SRR     (1 << 15)
#define OVLO    (1 << 0)

#define EN_START    0x80
#define CNF_START   0x2

static uint32_t driver_state_en = 0;
static uint32_t driver_state_ls = 0;
static uint32_t driver_state_hs = 0;

static void ncv7726b_write_state(void) {
    // calculate the lower 16 bits
    uint16_t lower = OVLO | SRR;
    // OUT channels aren't indexed from one...
    for (int i = 1; i < 7; i++) {
        if (driver_state_en & (1 << i)) {
            lower |= EN_START << (i - 1);
            // drive direction needs to be explicit otherwise driver could 
            // cause attached component to get hot.
            if (!(driver_state_ls & (1 << i)) && !(driver_state_hs & (1 << i)))
                target_panic();

            if (driver_state_ls & (1 << i))
                lower &= ~(CNF_START << (i - 1)); // subtract 1 so the shift is correct (OUT1 is BIT 1 of lower 16)

            if (driver_state_hs & (1 << i))
                lower |= CNF_START << (i - 1); // subtract 1 so the shift is correct
        }
    }

    // calculate the upper 16 bits
    uint16_t upper = HB_SEL | OVLO | SRR;
    for (int i = 7; i < OUT_COUNT + 1; i++) {
        if (driver_state_en & (1 << i)) {
            upper |= EN_START << (i - 6);
            // drive direction needs to be explicit otherwise driver could 
            // cause attached component to get hot.
            if (!(driver_state_ls & (1 << i)) && !(driver_state_hs & (1 << i)))
                target_panic();

            if (driver_state_ls & (1 << i))
                upper &= ~(CNF_START << (i - 7)); // subtract 7 so the shift is correct (i.e. OUT7 is BIT1 of the upper 16)

            if (driver_state_hs & (1 << i))
                upper |= CNF_START << (i - 7); // subtract 7 so the shift is correct
        }
    }
}

static void ncv7726b_channel_set(uint8_t channel, int state, uint8_t write) {

    ASSERT(channel <= OUT_COUNT && channel > 0);

    int msk = (1 << channel);

    driver_state_en |= msk;

    switch (state) {
        case DRIVE_LS_ON:
            driver_state_hs &= ~msk;
            driver_state_ls |= msk;
            break;
        case DRIVE_HS_ON:
            driver_state_ls &= ~msk;
            driver_state_hs |= msk;
            break;
    }

    if (write)
        ncv7726b_write_state();
}

static void ncv7726b_channel_clear(uint8_t channel, uint8_t write) {
    ASSERT(channel <= OUT_COUNT && channel > 0);

    int msk = (1 << channel);

    driver_state_en &= ~msk;
    driver_state_ls &= ~msk;
    driver_state_hs &= ~msk;

    if (write)
        ncv7726b_write_state();
}

static void ncv7726b_init(void) {
    dspi_init();
    #error add reset command
}