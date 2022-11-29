#include "jd_drivers.h"

#ifndef SCD40_ADDR
#define SCD40_ADDR 0x62
#endif

#define SCD40_DATA_READY 0xE4B8
#define SCD40_REINIT 0x3646
#define SCD40_FACTORY_RESET 0x3632
#define SCD40_FORCED_RECAL 0x362F
#define SCD40_SELF_TEST 0x3639
#define SCD40_STOP_PERIODIC_MEASUREMENT 0x3F86
#define SCD40_START_PERIODIC_MEASUREMENT 0x21B1
#define SCD40_START_LOW_POWER_PERIODIC_MEASUREMENT 0x21AC
#define SCD40_READ_MEASUREMENT 0xEC05
#define SCD40_SERIAL_NUMBER 0x3682
#define SCD40_GET_TEMP_OFFSET 0x2318
#define SCD40_SET_TEMP_OFFSET 0x241D
#define SCD40_GET_ALTITUDE 0x2322
#define SCD40_SET_ALTITUDE 0x2427
#define SCD40_SET_PRESSURE 0xE000
#define SCD40_PERSIST_SETTINGS 0x3615
#define SCD40_GET_ASCE 0x2313
#define SCD40_SET_ASCE 0x2416

typedef struct state {
    env_reading_t co2;
    env_reading_t temperature;
    env_reading_t humidity;
    uint32_t nextsample;
    uint8_t inited;
} ctx_t;
static ctx_t state;

static const int32_t humidity_error[] = {ERR_HUM(0, 9), ERR_HUM(20, 6), ERR_HUM(65, 6),
                                         ERR_HUM(100, 9), ERR_END};

static const int32_t temperature_error[] = {ERR_TEMP(-10, 1.5), ERR_TEMP(15, 0.8),
                                            ERR_TEMP(35, 0.8), ERR_TEMP(60, 1.5), ERR_END};

static void send_cmd(uint16_t cmd) {
    if (i2c_write_reg16_buf(SCD40_ADDR, cmd, NULL, 0))
        JD_PANIC();
    jd_services_sleep_us(1000);
}

#define WORD(off) ((data[off] << 8) | data[(off) + 1])

static int read_data_ready(void) {
    if (i2c_write_reg16_buf(SCD40_ADDR, SCD40_DATA_READY, NULL, 0))
        return -1;
    jd_services_sleep_us(1000);

    uint8_t data[3];
    if (i2c_read_ex(SCD40_ADDR, data, sizeof(data)))
        return -2;

    if (jd_sgp_crc8(&data[0], 2) != data[2])
        return -3;

    return (WORD(0) & ((1 << 11) - 1)) != 0;
}

static bool data_ready(void) {
    int d = read_data_ready();
    if (d < 0)
        JD_PANIC();
    return (d & ((1 << 11) - 1)) != 0;
}

static int scd_read_serial(void) {
    if (i2c_write_reg16_buf(SCD40_ADDR, SCD40_STOP_PERIODIC_MEASUREMENT, NULL, 0) == 0)
        jd_services_sleep_us(500 * 1000);

    if (i2c_write_reg16_buf(SCD40_ADDR, SCD40_SERIAL_NUMBER, NULL, 0)) {
        DMESG("SCD40 not present");
        return -1;
    }
    jd_services_sleep_us(1000);

    uint8_t data[3 * 3];
    if (i2c_read_ex(SCD40_ADDR, data, sizeof(data)))
        return -1;

    if (jd_sgp_crc8(&data[0], 2) != data[2])
        return -1;
    if (jd_sgp_crc8(&data[3], 2) != data[5])
        return -1;
    if (jd_sgp_crc8(&data[6], 2) != data[8])
        return -1;

    DMESG("SCD40 serial: %x:%x:%x", WORD(0), WORD(3), WORD(6));

    return 0;
}

static bool scd40_is_present(void) {
    i2c_init();
    return scd_read_serial() == 0;
}

static void scd40_init(void) {
    ctx_t *ctx = &state;
    if (ctx->inited)
        return;

    ctx->co2.min_value = SCALE_CO2(400);
    ctx->co2.max_value = SCALE_CO2(2000);
    ctx->humidity.min_value = SCALE_HUM(0);
    ctx->humidity.max_value = SCALE_HUM(100);
    ctx->temperature.min_value = SCALE_TEMP(-10);
    ctx->temperature.max_value = SCALE_TEMP(60);

    ctx->inited = 1;
    i2c_init();

    if (read_data_ready() < 0) {
        DMESG("SCD40 missing");
        JD_PANIC();
    }

    if (scd_read_serial() != 0) {
        DMESG("SCD40: already measuring; stopping");
        send_cmd(SCD40_STOP_PERIODIC_MEASUREMENT);
        // wait before starting again
        ctx->nextsample = now + (600 << 10);
    } else {
        ctx->nextsample = now + (1 << 10);
    }
}

static void scd40_sleep(void) {
    ctx_t *ctx = &state;
    ctx->inited = 0;
    send_cmd(SCD40_STOP_PERIODIC_MEASUREMENT);
}

static void scd40_process(void) {
    ctx_t *ctx = &state;
    // check for data every two seconds; typical measurements are only once every 5s
    if (jd_should_sample_delay(&ctx->nextsample, 2 << 20)) {
        if (ctx->inited == 1) {
            send_cmd(SCD40_START_PERIODIC_MEASUREMENT);
            ctx->inited = 2;
        } else if (data_ready()) {
            send_cmd(SCD40_READ_MEASUREMENT);
            uint8_t data[9];
            if (i2c_read_ex(SCD40_ADDR, data, sizeof(data)))
                JD_PANIC();
            ctx->co2.value = WORD(0) << 10;
            // 50ppm + 5% of reading; 5% * (1<<10) == 51.2
            ctx->co2.error = (50 << 10) + WORD(0) * 51;
            env_set_value(&ctx->temperature, (175 * WORD(3) >> 6) - (45 << 10), temperature_error);
            env_set_value(&ctx->humidity, 100 * WORD(6) >> 6, humidity_error);
            ctx->inited = 3;
        }
    }
}

ENV_INIT_TRIPLE(scd40_init, scd40_sleep);

ENV_DEFINE_GETTER(scd40, co2)
ENV_DEFINE_GETTER(scd40, temperature)
ENV_DEFINE_GETTER(scd40, humidity)

#define FUNS(val, idx) ENV_GETTER(scd40, val, idx), .is_present = scd40_is_present

const env_sensor_api_t co2_scd40 = {FUNS(co2, 0)};
const env_sensor_api_t temperature_scd40 = {FUNS(temperature, 1)};
const env_sensor_api_t humidity_scd40 = {FUNS(humidity, 2)};
