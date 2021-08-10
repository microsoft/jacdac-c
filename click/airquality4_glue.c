#include "airquality4.h"
#include "interfaces/jd_sensor_api.h"

static airquality4_t ctx;
static uint32_t nextsample;
static uint32_t numsamples;
static env_reading_t eco2;
static env_reading_t tvoc;
static uint8_t hum_comp[3];

static uint8_t crc8(const uint8_t *data, int len) {
    uint8_t res = 0xff;
    while (len--) {
        res ^= *data++;
        for (int i = 0; i < 8; ++i)
            if (res & 0x80)
                res = (res << 1) ^ 0x31;
            else
                res = (res << 1);
    }
    return res;
}

static void aq4_init(void) {
    airquality4_cfg_t cfg;
    airquality4_cfg_setup(&cfg);
    AIRQUALITY4_MAP_MIKROBUS(cfg, NA);
    if (airquality4_init(&ctx, &cfg) != AIRQUALITY4_OK)
        hw_panic();
    airquality4_default_cfg(&ctx);

    eco2.min_value = 400 << 10;
    eco2.max_value = 60000 << 10;
    tvoc.min_value = 0 << 10;
    tvoc.max_value = 60000 << 10;

    // "Test vector" from datasheet
    uint8_t tmp[2] = {0xbe, 0xef};
    if (crc8(tmp, 2) != 0x92)
        hw_panic();
}

static void aq4_set_temp_humidity(int32_t temp, int32_t humidity) {
    uint16_t scaled = env_absolute_humidity(temp, humidity) >> 2;
    hum_comp[0] = scaled >> 8;
    hum_comp[1] = scaled & 0xff;
    hum_comp[2] = crc8(hum_comp, 2);
}

static void aq4_process(void) {
    if (jd_should_sample(&nextsample, 1000000)) {
        if (hum_comp[0] || hum_comp[1]) {
            i2c_write_reg16_buf(0x58, 0x2061, hum_comp, 3);
            jd_services_sleep_us(10000);
        }

        uint16_t vals[2];
        air_quality4_get_co2_and_tvoc(&ctx, vals);

        numsamples++;

        eco2.value = vals[0] << 10;
        eco2.error = vals[0] << 6; // just a wild guess

        tvoc.value = vals[1] << 10;
        tvoc.error = vals[1] << 6; // just a wild guess
    }
}

static void aq4_sleep(void) {
    // this is "general call" to register 0x06
    // air_quality4_soft_reset has it wrong
    i2c_write_reg_buf(0x00, 0x06, NULL, 0);
}

static const env_reading_t *eco2_reading(void) {
    return numsamples > 15 ? &eco2 : NULL;
}

static const env_reading_t *tvoc_reading(void) {
    return numsamples > 15 ? &tvoc : NULL;
}

static uint32_t aq4_conditioning_period(void) {
    return 15;
}

ENV_INIT_DUAL(aq4_init, aq4_sleep)

const env_sensor_api_t eco2_airquality4 = {
    .get_reading = eco2_reading,
    .process = aq4_process,
    .conditioning_period = aq4_conditioning_period,
    .set_temp_humidity = aq4_set_temp_humidity,
    ENV_INIT_PTRS(0),
};

const env_sensor_api_t tvoc_airquality4 = {
    .get_reading = tvoc_reading,
    .process = aq4_process,
    .conditioning_period = aq4_conditioning_period,
    .set_temp_humidity = aq4_set_temp_humidity,
    ENV_INIT_PTRS(1),
};