#define SCALE0(c, i) ((((c)&0xff) * (1 + (i & 0xff))) >> 8)
#define SCALE(c, i)                                                                                \
    (SCALE0((c) >> 0, i) << 0) | (SCALE0((c) >> 8, i) << 8) | (SCALE0((c) >> 16, i) << 16)

static void limit_intensity(srv_t *state) {
    uint8_t *d = (uint8_t *)state->pxbuffer;
    unsigned n = state->numpixels * 3;
    int prev_intensity = state->intensity;
    int intensity = state->intensity;

    intensity += 1 + (intensity >> 5);
    if (intensity > state->requested_intensity)
        intensity = state->requested_intensity;

    int current_full = 0;
    int current = 0;
    int current_prev = 0;
    while (n--) {
        uint8_t v = *d++;
        current += SCALE0(v, intensity);
        current_prev += SCALE0(v, prev_intensity);
        current_full += v;
    }

    // 46uA per step of LED
    current *= 46;
    current_prev *= 46;
    current_full *= 46;

    // 14mA is the chip at 48MHz, 930uA per LED is static
    int base_current = 14000 + 930 * state->numpixels;
    int current_limit = state->maxpower * 1000 - base_current;

    if (current <= current_limit) {
        state->intensity = intensity;
        // LOG("curr: %dmA; not limiting %d", (base_current + current) / 1000, state->intensity);
        return;
    }

    if (current_prev <= current_limit) {
        return; // no change needed
    }

    int inten = current_limit / (current_full >> 8) - 1;
    if (inten < 0)
        inten = 0;
    LOG("limiting %d -> %d; %dmA", state->intensity, inten,
        (base_current + (current_full * inten >> 8)) / 1000);
    state->intensity = inten;
}

static bool is_empty(const uint32_t *data, uint32_t size) {
    for (unsigned i = 0; i < size; ++i) {
        if (data[i])
            return false;
    }
    return true;
}

static bool is_enabled(srv_t *state) {
    return state->numpixels > 0 && state->requested_intensity > 0;
}

static void tx_done(void) {
    pwr_leave_pll();
    state_->in_tx = 0;
}
