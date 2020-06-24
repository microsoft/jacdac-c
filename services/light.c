#include "jd_protocol.h"
#include "interfaces/jd_pixel.h"
#include "interfaces/jd_hw_pwr.h"

#define DEFAULT_INTENSITY 15
#define DEFAULT_NUMPIXELS 15
#define DEFAULT_MAXPOWER 200

#define LIGHT_REG_LIGHTTYPE 0x80
#define LIGHT_REG_NUMPIXELS 0x81
//#define LIGHT_REG_DURATION 0x82
//#define LIGHT_REG_COLOR 0x83
#define LIGHT_REG_ACTUAL_INTENSITY 0x180

#define LIGHT_CMD_RUN 0x81

// #define LOG DMESG
// #define LOG NOLOG

/*

* `0xD0: set_all(C+)` - set all pixels in current range to given color pattern
* `0xD1: fade(C+)` - set `N` pixels to color between colors in sequence
* `0xD2: fade_hsv(C+)` - similar to `fade()`, but colors are specified and faded in HSV
* `0xD3: rotate_fwd(K)` - rotate (shift) pixels by `K` positions away from the connector
* `0xD4: rotate_back(K)` - same, but towards the connector
* `0xD5: show(M=50)` - send buffer to strip and wait `M` milliseconds
* `0xD6: range(P=0, N=length)` - range from pixel `P`, `N` pixels long
* `0xD7: mode(K=0)` - set update mode
* `0xD8: mode1(K=0)` - set update mode for next command only

*/

#define LIGHT_PROG_SET_ALL 0xD0
#define LIGHT_PROG_FADE 0xD1
#define LIGHT_PROG_FADE_HSV 0xD2
#define LIGHT_PROG_ROTATE_FWD 0xD3
#define LIGHT_PROG_ROTATE_BACK 0xD4
#define LIGHT_PROG_SHOW 0xD5
#define LIGHT_PROG_RANGE 0xD6
#define LIGHT_PROG_MODE 0xD7
#define LIGHT_PROG_MODE1 0xD8

#define LIGHT_MODE_REPLACE 0x00
#define LIGHT_MODE_ADD_RGB 0x01
#define LIGHT_MODE_SUBTRACT_RGB 0x02
#define LIGHT_MODE_MULTIPLY_RGB 0x03
#define LIGHT_MODE_LAST 0x03

#define LIGHT_PROG_COLN 0xC0
#define LIGHT_PROG_COL1 0xC1
#define LIGHT_PROG_COL2 0xC2
#define LIGHT_PROG_COL3 0xC3

#define LIGHT_PROG_COL1_SET 0xCF

#define PROG_EOF 0
#define PROG_CMD 1
#define PROG_NUMBER 3
#define PROG_COLOR_BLOCK 4

typedef union {
    struct {
        uint8_t b;
        uint8_t g;
        uint8_t r;
        uint8_t _a; // not really used
    };
    uint32_t val;
} RGB;

REG_DEFINITION(                         //
    light_regs,                         //
    REG_SRV_BASE,                       //
    REG_U8(JD_REG_INTENSITY),           //
    REG_U8(LIGHT_REG_ACTUAL_INTENSITY), //
    REG_U8(LIGHT_REG_LIGHTTYPE),        //
    REG_U16(LIGHT_REG_NUMPIXELS),       //
    REG_U16(JD_REG_MAX_POWER),          //
)

struct srv_state {
    SRV_COMMON;

    uint8_t requested_intensity;
    uint8_t intensity;
    uint8_t lighttype;
    uint16_t numpixels;
    uint16_t maxpower;

    uint16_t pxbuffer_allocated;
    uint8_t *pxbuffer;
    volatile uint8_t in_tx;
    volatile uint8_t dirty;
    uint8_t inited;

    uint8_t prog_mode;
    uint8_t prog_tmpmode;

    uint16_t range_start;
    uint16_t range_end;
    uint16_t range_len;
    uint16_t range_ptr;

    uint8_t prog_ptr;
    uint8_t prog_size;
    uint32_t prog_next_step;
    uint8_t prog_data[JD_SERIAL_PAYLOAD_SIZE + 1];

    uint8_t anim_flag;
    uint32_t anim_step, anim_value;
    srv_cb_t anim_fn;
    uint32_t anim_end;
};

static srv_t *state_;

static inline RGB rgb(uint8_t r, uint8_t g, uint8_t b) {
    RGB x = {.r = r, .g = g, .b = b};
    return x;
}

static RGB hsv(uint8_t hue, uint8_t sat, uint8_t val) {
    // scale down to 0..192
    hue = (hue * 192) >> 8;

    // reference: based on FastLED's hsv2rgb rainbow algorithm
    // [https://github.com/FastLED/FastLED](MIT)
    int invsat = 255 - sat;
    int brightness_floor = (val * invsat) >> 8;
    int color_amplitude = val - brightness_floor;
    int section = (hue / 0x40) >> 0; // [0..2]
    int offset = (hue % 0x40) >> 0;  // [0..63]

    int rampup = offset;
    int rampdown = (0x40 - 1) - offset;

    int rampup_amp_adj = ((rampup * color_amplitude) / (256 / 4));
    int rampdown_amp_adj = ((rampdown * color_amplitude) / (256 / 4));

    int rampup_adj_with_floor = (rampup_amp_adj + brightness_floor);
    int rampdown_adj_with_floor = (rampdown_amp_adj + brightness_floor);

    int r, g, b;
    if (section) {
        if (section == 1) {
            // section 1: 0x40..0x7F
            r = brightness_floor;
            g = rampdown_adj_with_floor;
            b = rampup_adj_with_floor;
        } else {
            // section 2; 0x80..0xBF
            r = rampup_adj_with_floor;
            g = brightness_floor;
            b = rampdown_adj_with_floor;
        }
    } else {
        // section 0: 0x00..0x3F
        r = rampdown_adj_with_floor;
        g = rampup_adj_with_floor;
        b = brightness_floor;
    }
    return rgb(r, g, b);
}

static bool is_empty(const uint32_t *data, uint32_t size) {
    for (unsigned i = 0; i < size; ++i) {
        if (data[i])
            return false;
    }
    return true;
}

static bool is_enabled(srv_t *state) {
    // TODO check if all pixels are zero
    return state->numpixels > 0 && state->intensity > 0;
}

static void reset_range(srv_t *state) {
    state->range_ptr = state->range_start;
}

static int mulcol(int c, int m) {
    int c2 = (c * m) >> 7;
    if (m < 128 && c == c2)
        c2--;
    else if (m > 128 && c == c2)
        c2++;
    return c2;
}

static int clamp(int c) {
    if (c < 0)
        return 0;
    if (c > 255)
        return 255;
    return c;
}

static bool set_next(srv_t *state, RGB c) {
    if (state->range_ptr >= state->range_end)
        return false;
    uint8_t *p = &state->pxbuffer[state->range_ptr++ * 3];

    // fast path
    if (state->prog_tmpmode == LIGHT_MODE_REPLACE) {
        p[0] = c.r;
        p[1] = c.g;
        p[2] = c.b;
        return true;
    }

    int r = p[0], g = p[1], b = p[2];
    switch (state->prog_tmpmode) {
    case LIGHT_MODE_ADD_RGB:
        r += c.r;
        g += c.g;
        b += c.b;
        break;
    case LIGHT_MODE_SUBTRACT_RGB:
        r -= c.r;
        g -= c.g;
        b -= c.b;
        break;
    case LIGHT_MODE_MULTIPLY_RGB:
        r = mulcol(r, c.r);
        g = mulcol(g, c.g);
        b = mulcol(b, c.b);
        break;
    }
    p[0] = clamp(r);
    p[1] = clamp(g);
    p[2] = clamp(b);
    return true;
}

static void show(srv_t *state) {
    state->dirty = 1;
}

#define SCALE0(c, i) ((((c)&0xff) * (1 + (i & 0xff))) >> 8)
#define SCALE(c, i)                                                                                \
    (SCALE0((c) >> 0, i) << 0) | (SCALE0((c) >> 8, i) << 8) | (SCALE0((c) >> 16, i) << 16)

static void tx_done(void) {
    pwr_leave_pll();
    state_->in_tx = 0;
}

static void limit_intensity(srv_t *state) {
    uint8_t *d = (uint8_t *)state->pxbuffer;
    unsigned n = state->numpixels * 3;
    int prev_intensity = state->intensity;

    if (state->requested_intensity > state->intensity)
        state->intensity += 1 + (state->intensity >> 5);
    if (state->intensity > state->requested_intensity)
        state->intensity = state->requested_intensity;

    int current_full = 0;
    int current = 0;
    while (n--) {
        uint8_t v = *d++;
        current += SCALE0(v, state->intensity);
        current_full += v;
    }

    // 46uA per step of LED
    current *= 46;
    current_full *= 46;

    // 14mA is the chip at 48MHz, 930uA per LED is static
    int base_current = 14000 + 930 * state->numpixels;
    int current_limit = state->maxpower * 1000 - base_current;

    if (current <= current_limit) {
        // LOG("curr: %dmA; not limiting %d", (base_current + current) / 1000, state->intensity);
        return;
    }

    int inten = current_limit / (current_full >> 8) - 1;
    if (inten < 0)
        inten = 0;
    // only display message if the limit wasn't just a result of relaxing above
    if (inten < prev_intensity)
        LOG("limiting %d -> %d; %dmA", state->intensity, inten,
            (base_current + (current_full * inten >> 8)) / 1000);
    state->intensity = inten;
}

static RGB prog_fetch_color(srv_t *state) {
    uint32_t ptr = state->prog_ptr;
    if (ptr + 3 > state->prog_size)
        return rgb(0, 0, 0);
    uint8_t *d = state->prog_data + ptr;
    state->prog_ptr = ptr + 3;
    return rgb(d[0], d[1], d[2]);
}

static int prog_fetch(srv_t *state, uint32_t *dst) {
    if (state->prog_ptr >= state->prog_size)
        return PROG_EOF;
    uint8_t *d = state->prog_data;
    uint8_t c = d[state->prog_ptr++];
    if (!(c & 0x80)) {
        *dst = c;
        return PROG_NUMBER;
    } else if ((c & 0xc0) == 0x80) {
        *dst = ((c & 0x3f) << 8) | d[state->prog_ptr++];
        return PROG_NUMBER;
    } else
        switch (c) {
        case LIGHT_PROG_COL1:
            *dst = 1;
            return PROG_COLOR_BLOCK;
        case LIGHT_PROG_COL2:
            *dst = 2;
            return PROG_COLOR_BLOCK;
        case LIGHT_PROG_COL3:
            *dst = 3;
            return PROG_COLOR_BLOCK;
        case LIGHT_PROG_COLN:
            *dst = d[state->prog_ptr++];
            return PROG_COLOR_BLOCK;
        default:
            *dst = c;
            return PROG_CMD;
        }
}

static int prog_fetch_num(srv_t *state, int defl) {
    uint8_t prev = state->prog_ptr;
    uint32_t res;
    int r = prog_fetch(state, &res);
    if (r == PROG_NUMBER)
        return res;
    else {
        state->prog_ptr = prev; // rollback
        return defl;
    }
}

static int prog_fetch_cmd(srv_t *state) {
    uint32_t cmd;
    // skip until there's a command
    for (;;) {
        switch (prog_fetch(state, &cmd)) {
        case PROG_CMD:
            return cmd;
        case PROG_COLOR_BLOCK:
            while (cmd--)
                prog_fetch_color(state);
            break;
        case PROG_EOF:
            return 0;
        }
    }
}

static void prog_set(srv_t *state, uint32_t len) {
    reset_range(state);
    uint8_t start = state->prog_ptr;
    for (;;) {
        state->prog_ptr = start;
        bool ok = false;
        for (uint32_t i = 0; i < len; ++i) {
            // don't break the loop immedietely if !ok - make sure the prog counter advances
            ok = set_next(state, prog_fetch_color(state));
        }
        if (!ok)
            break;
    }
}

static void prog_fade(srv_t *state, uint32_t len, bool usehsv) {
    if (len < 2) {
        prog_set(state, len);
        return;
    }
    unsigned colidx = 0;
    uint8_t endp = state->prog_ptr + 3 * len;
    RGB col0 = prog_fetch_color(state);
    RGB col1 = prog_fetch_color(state);

    uint32_t colstep = ((len - 1) << 16) / state->range_len;
    uint32_t colpos = 0;

    reset_range(state);

    for (;;) {
        while (colidx < (colpos >> 16)) {
            colidx++;
            col0 = col1;
            col1 = prog_fetch_color(state);
        }
        uint32_t fade1 = colpos & 0xffff;
        uint32_t fade0 = 0xffff - fade1;

#define MIX(f) (col0.f * fade0 + col1.f * fade1 + 0x8000) >> 16

        RGB col = rgb(MIX(r), MIX(g), MIX(b));
        if (!set_next(state, usehsv ? hsv(col.r, col.g, col.b) : col))
            break;
        colpos += colstep;
    }

    state->prog_ptr = endp;
}

#define AT(idx) ((uint8_t *)state->pxbuffer)[(idx)*3]

static void prog_rot(srv_t *state, uint32_t shift) {
    if (shift == 0 || shift >= state->range_len)
        return;

    uint8_t *first = &AT(state->range_start);
    uint8_t *middle = &AT(state->range_start + shift);
    uint8_t *last = &AT(state->range_end);
    uint8_t *next = middle;

    while (first != next) {
        uint8_t tmp = *first;
        *first++ = *next;
        *next++ = tmp;

        if (next == last)
            next = middle;
        else if (first == middle)
            middle = next;
    }
}

static int fetch_mode(srv_t *state) {
    int m = prog_fetch_num(state, 0);
    if (m > LIGHT_MODE_LAST)
        return 0;
    return m;
}

static void prog_process(srv_t *state) {
    if (state->prog_ptr >= state->prog_size)
        return;

    // don't run programs while sending data
    if (state->in_tx || in_future(state->prog_next_step))
        return;

    // full speed ahead! the code below can be a bit heavy and we want it done quickly
    pwr_enter_pll();

    for (;;) {
        int cmd = prog_fetch_cmd(state);
        LOG("cmd:%x", cmd);
        if (!cmd)
            break;

        if (cmd == LIGHT_PROG_SHOW) {
            uint32_t k = prog_fetch_num(state, 50);
            // base the next step of previous expect step time, now current time
            // to keep the clock synchronized
            state->prog_next_step += k * 1000;
            show(state);
            break;
        }

        switch (cmd) {
        case LIGHT_PROG_COL1_SET:
            state->range_ptr = state->range_start + prog_fetch_num(state, 0);
            set_next(state, prog_fetch_color(state));
            break;
        case LIGHT_PROG_FADE:
        case LIGHT_PROG_FADE_HSV:
        case LIGHT_PROG_SET_ALL: {
            uint32_t len;
            if (prog_fetch(state, &len) != PROG_COLOR_BLOCK || len == 0)
                continue; // bailout
            LOG("%x l=%d", cmd, len);
            if (cmd == LIGHT_PROG_SET_ALL)
                prog_set(state, len);
            else
                prog_fade(state, len, cmd == LIGHT_PROG_FADE_HSV);
            break;
        }

        case LIGHT_PROG_ROTATE_BACK:
        case LIGHT_PROG_ROTATE_FWD: {
            int k = prog_fetch_num(state, 1);
            int len = state->range_len;
            if (len == 0)
                continue;
            while (k >= len)
                k -= len;
            if (cmd == LIGHT_PROG_ROTATE_FWD && k != 0)
                k = len - k;
            LOG("rot %x %d l=%d", cmd, k, len);
            prog_rot(state, k);
            break;
        }

        case LIGHT_PROG_MODE1:
            state->prog_tmpmode = fetch_mode(state);
            break;

        case LIGHT_PROG_MODE:
            state->prog_mode = fetch_mode(state);
            break;

        case LIGHT_PROG_RANGE: {
            int start = prog_fetch_num(state, 0);
            int len = prog_fetch_num(state, state->numpixels);
            if (start > state->numpixels)
                start = state->numpixels;
            int end = start + len;
            if (end > state->numpixels)
                end = state->numpixels;
            state->range_start = start;
            state->range_end = end;
            state->range_len = end - start;
            break;
        }
        }

        if (cmd != LIGHT_PROG_MODE1)
            state->prog_tmpmode = state->prog_mode;
    }
    pwr_leave_pll();
}

void light_process(srv_t *state) {
    prog_process(state);

    if (!is_enabled(state))
        return;

    if (state->dirty && !state->in_tx) {
        state->dirty = 0;
        if (is_empty((uint32_t *)state->pxbuffer, PX_WORDS(state->numpixels))) {
            jd_power_enable(0);
            return;
        } else {
            jd_power_enable(1);
        }
        state->in_tx = 1;
        pwr_enter_pll();
        limit_intensity(state);
        px_tx(state->pxbuffer, state->numpixels * 3, state->intensity, tx_done);
    }
}

static void sync_config(srv_t *state) {
    if (!is_enabled(state)) {
        jd_power_enable(0);
        return;
    }

    if (!state->inited) {
        state->inited = true;
        px_init(state->lighttype);
    }

    int needed = PX_WORDS(state->numpixels);
    if (needed > state->pxbuffer_allocated) {
        state->pxbuffer_allocated = needed;
        state->pxbuffer = jd_alloc(needed * 4);
    }

    jd_power_enable(1);
}

static void handle_run_cmd(srv_t *state, jd_packet_t *pkt) {
    state->prog_size = pkt->service_size;
    state->prog_ptr = 0;
    memcpy(state->prog_data, pkt->data, state->prog_size);

    state->range_start = 0;
    state->range_end = state->range_len = state->numpixels;
    state->prog_tmpmode = state->prog_mode = 0;

    state->prog_next_step = now;
    sync_config(state);
}

void light_handle_packet(srv_t *state, jd_packet_t *pkt) {
    switch (pkt->service_command) {
    case LIGHT_CMD_RUN:
        handle_run_cmd(state, pkt);
        break;
    default:
        service_handle_register(state, pkt, light_regs);
        break;
    }
}

SRV_DEF(light, JD_SERVICE_CLASS_LIGHT);
void light_init(void) {
    SRV_ALLOC(light);
    state_ = state; // there is global singleton state
    state->intensity = DEFAULT_INTENSITY;
    state->numpixels = DEFAULT_NUMPIXELS;
    state->maxpower = DEFAULT_MAXPOWER;

#if 0
    state->maxpower = 20;

    state->numpixels = 14;
    state->intensity = 0xff;
    state->color = 0x00ff00;
    state->duration = 100;

    sync_config(state);
    start_animation(state, 2);
#endif
}
