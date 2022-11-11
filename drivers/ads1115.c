#include "jd_drivers.h"
#include "services/jd_services.h"

#define ADS1115_CONV_REG 0x0
#define ADS1115_CONF_REG 0x1
#define ADS1115_LO_THRESH_REG 0x2
#define ADS1115_HI_THRESH_REG 0x3

//#define LOG JD_LOG
#define LOG JD_NOLOG

/**
 * Bit positions for CONF register
 **/
// OP status / single start
#define ADS1115_CONF_OS 15
/**
 * Mux configuration:
 *   000 : AINP = AIN0 and AINN = AIN1 (default)
 *   001 : AINP = AIN0 and AINN = AIN3
 *   010 : AINP = AIN1 and AINN = AIN3
 *   011 : AINP = AIN2 and AINN = AIN3
 *   100 : AINP = AIN0 and AINN = GND
 *   101 : AINP = AIN1 and AINN = GND
 *   110 : AINP = AIN2 and AINN = GND
 *   111 : AINP = AIN3 and AINN = GND
 **/
#define ADS1115_CONF_MUX 12
/**
 * Programmable gain:
 *   000 : FSR = ±6.144 V(1)
 *   001 : FSR = ±4.096 V(1)
 *   010 : FSR = ±2.048 V (default)
 *   011 : FSR = ±1.024 V
 *   100 : FSR = ±0.512 V
 *   101 : FSR = ±0.256 V
 *   110 : FSR = ±0.256 V
 *   111 : FSR = ±0.256 V
 **/
#define ADS1115_CONF_PGA 9

// operating mode: 0 == continuious; 1 == single shot/power-down
#define ADS1115_CONF_MODE 8

/**
 * Data rate
 *   000 : 8 SPS
 *   001 : 16 SPS
 *   010 : 32 SPS
 *   011 : 64 SPS
 *   100 : 128 SPS (default)
 *   101 : 250 SPS
 *   110 : 475 SPS
 *   111 : 860 SPS
 **/
#define ADS1115_CONF_DR 5
// comparator mode: 0 == traditional; 1 == windowed
#define ADS1115_CONF_COMP_MODE 4
/**
 * comparator polarity:
 *   0: ALERT/RDY is active lo
 *   1: ALERT/RDY is active hi
 **/
#define ADS1115_CONF_COMP_POL 3
/**
 * latching comparator config
 *   0: The ALERT/RDY pin does not latch when asserted.
 *   1: The asserted ALERT/RDY pin remains latched until conversion data are read.
 **/
#define ADS1115_CONF_COMP_LAT 2

/**
 * Comparator queue and disable -- ONLY ON ADS1114/1115
 **/
#define ADS1115_CONF_COMP_QUE 0

// channel type is restricted to uint8_t
// use 255 to mean "GND"
#define ADS1115_GND_CHAN 255

typedef struct {
    uint8_t pos_chan;
    uint8_t neg_chan;
    uint8_t bitmsk;
} ads1115_channel_map_t;

#define ADS1115_CHANNEL_MAP_LEN 8

static const ads1115_channel_map_t channel_map[ADS1115_CHANNEL_MAP_LEN] = {
    {.pos_chan = 0, .neg_chan = 1, .bitmsk = 0x0},
    {.pos_chan = 0, .neg_chan = 3, .bitmsk = 0x1},
    {.pos_chan = 1, .neg_chan = 3, .bitmsk = 0x2},
    {.pos_chan = 2, .neg_chan = 3, .bitmsk = 0x3},
    {.pos_chan = 0, .neg_chan = ADS1115_GND_CHAN, .bitmsk = 0x4},
    {.pos_chan = 1, .neg_chan = ADS1115_GND_CHAN, .bitmsk = 0x5},
    {.pos_chan = 2, .neg_chan = ADS1115_GND_CHAN, .bitmsk = 0x6},
    {.pos_chan = 3, .neg_chan = ADS1115_GND_CHAN, .bitmsk = 0x7},
};

typedef struct {
    int32_t gain;
    uint8_t bitmsk;
    float lsb_mv;
} ads1115_gain_map_t;

#define ADS1115_GAIN_MAP_LEN 8
static const ads1115_gain_map_t gain_map[ADS1115_CHANNEL_MAP_LEN] = {
    {.gain = 6144, .bitmsk = 0x0, .lsb_mv = 0.1875},
    {.gain = 4096, .bitmsk = 0x1, .lsb_mv = 0.125},
    {.gain = 2048, .bitmsk = 0x2, .lsb_mv = 0.0625},
    {.gain = 1024, .bitmsk = 0x3, .lsb_mv = 0.03125},
    {.gain = 512, .bitmsk = 0x4, .lsb_mv = 0.015625},
    {.gain = 256, .bitmsk = 0x5, .lsb_mv = 0.0078125},
    {.gain = 256, .bitmsk = 0x6, .lsb_mv = 0.0078125},
    {.gain = 256, .bitmsk = 0x7, .lsb_mv = 0.0078125},
};

// +/- 2.048V by default
static uint8_t gain_bitmsk = 0x2;
static uint8_t ads1115_address = 0;

static inline uint16_t flip(uint16_t v) {
    return ((v & 0xff00) >> 8) | ((v & 0x00ff) << 8);
}

static inline uint8_t ads1115_channel_bitmsk(uint8_t pos_channel, uint8_t neg_channel) {
    for (int i = 0; i < ADS1115_CHANNEL_MAP_LEN; i++) {
        ads1115_channel_map_t c = channel_map[i];

        if (pos_channel == c.pos_chan && neg_channel == c.neg_chan) {
            LOG("CHAN FOUND: %d, %x", i, c.bitmsk);
            return c.bitmsk;
        }
    }

    ERROR("No channel combination found for the given channels.");
    return 0;
}

static inline uint8_t ads1115_gain_bitmsk(int32_t gain_mv) {
    for (int i = ADS1115_GAIN_MAP_LEN - 1; i >= 0 ; i--) {
        ads1115_gain_map_t g = gain_map[i];
        if (g.gain >= gain_mv)
            return g.bitmsk;
    }

    // should we panic here?
    return gain_map[0].bitmsk;
}

static inline float ads1115_gain_mult(uint8_t bitmsk) {
    for (int i = 0; i < ADS1115_GAIN_MAP_LEN; i++) {
        ads1115_gain_map_t g = gain_map[i];
        if (bitmsk == g.bitmsk)
            return g.lsb_mv;
    }

    JD_ASSERT(false);
    return 0;
}

#define ADS1115_MAX_RETRIES 10000

static int ads1115_read(void) {
    int retries = 0;

#ifdef PIN_ACC_INT
    // alert/drdy pin is set to active lo in configure
    while (pin_get(PIN_ACC_INT) == 1 && retries++ < ADS1115_MAX_RETRIES)
        jd_services_sleep_us(300);
    // conversion time is 1/DR
    // we configure DR to 860
    LOG("RETRIES %d", retries);
    JD_ASSERT(retries < ADS1115_MAX_RETRIES);
    // LOG("RETRIES %d", retries);
#else
    while (1) {
        // ads1115_read_i2c(ADS1115_CONF_REG, &res, 2);
        int16_t res = 0;
        i2c_read_reg_buf(ads1115_address, ADS1115_CONF_REG, &res, 2);
        if (res & (1 << ADS1115_CONF_OS))
            break;
        jd_services_sleep_us(500);
        retries++;
        // conversion time is 1/DR
        // we configure DR to 860
        JD_ASSERT(retries < ADS1115_MAX_RETRIES);
    }
#endif
    target_wait_us(30);
    uint8_t buf[2] = {0};
    int rsp = i2c_read_reg_buf(ads1115_address, ADS1115_CONV_REG, buf, 2);
    (void)rsp;
    int16_t rr = (buf[0] << 8) | buf[1];
    // LOG("REC: %x %x %d", res & 0xffff, flip(res) & 0xffff, rsp);
    // ads1115_read_i2c(ADS1115_CONV_REG, &res, 2);
    return rr;
}

static void ads1115_set_gain(int32_t gain_mv) {
    gain_bitmsk = ads1115_gain_bitmsk(gain_mv);
}

static void ads1115_configure(uint8_t pos_chan, uint8_t neg_chan) {
    uint16_t channel_bitmsk = ads1115_channel_bitmsk(pos_chan, neg_chan);

    uint16_t value = (channel_bitmsk << ADS1115_CONF_MUX);

    // single shot start
    value |= (1 << ADS1115_CONF_OS);
    // single shot
    value |= (1 << ADS1115_CONF_MODE);
    // set gain
    value |= (gain_bitmsk << ADS1115_CONF_PGA);
    LOG("GAIN BITMSK %x", gain_bitmsk & 0x7);
    // set conversion rate to max (860 SPS)
    value |= (0x7 << ADS1115_CONF_DR);

#ifdef PIN_ACC_INT
    // latching comparator!
    // value |= (1 << ADS1115_CONF_COMP_LAT);
#else
    // alert/rdy high impedance
    value |= (0x3 << ADS1115_CONF_COMP_QUE);
#endif

    value = flip(value);

    LOG("CFG: %x ", value);
    // LOG("CONF SET: %x", value);
    i2c_write_reg_buf(ads1115_address, ADS1115_CONF_REG, &value, 2);
    // target_wait_us(100);
    // i2c_read_reg_buf(ads1115_address, ADS1115_CONF_REG, &value, 2);
    // LOG("CONF GET: %x", value);
}

static float ads1115_read_absolute(uint8_t channel) {
    ads1115_configure(channel, ADS1115_GND_CHAN);
    return ((float)ads1115_read() * ads1115_gain_mult(gain_bitmsk)) / 1000.0;
}

static float ads1115_read_differential(uint8_t pos_channel, uint8_t neg_channel) {
    ads1115_configure(pos_channel, neg_channel);
    return ((float)ads1115_read() * ads1115_gain_mult(gain_bitmsk)) / 1000.0;
}

static void ads1115_init(uint8_t i2c_address) {
    ads1115_address = i2c_address;
    i2c_init();
#ifdef PIN_ACC_INT
    // ALERT/RDY active low, configure ALRT pin as input
    pin_setup_input(PIN_ACC_INT, PIN_PULL_UP);

    // cfg alert/READY pin to trigger when conversion is ready
    uint16_t alert_cfg = 0;
    i2c_write_reg_buf(ads1115_address, ADS1115_LO_THRESH_REG, &alert_cfg, 2);
    alert_cfg = flip(0x8000);
    i2c_write_reg_buf(ads1115_address, ADS1115_HI_THRESH_REG, &alert_cfg, 2);
#endif
}

static void ads1115_nop(void) {}

const adc_api_t ads1115 = {.init = ads1115_init,
                           .read_differential = ads1115_read_differential,
                           .read_absolute = ads1115_read_absolute,
                           .set_gain = ads1115_set_gain,
                           .process = ads1115_nop,
                           .sleep = ads1115_nop};
