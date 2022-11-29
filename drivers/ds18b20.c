#include "jd_drivers.h"

#define SAMPLING_MS 200   // sampling at SAMPLING_MS+CONVERSION_MS
#define CONVERSION_MS 800 // for 12 bits; less for less bits
#define RESOLUTION 12     // bits

#define PRECISION 10 // i22.10 format

#define DS18B20_SKIP_ROM 0xCC
#define DS18B20_READ_ROM 0x33
#define DS18B20_CONVERT_T 0x44
#define DS18B20_WRITE_SCRATCHPAD 0x4E
#define DS18B20_READ_SCRATCHPAD 0xBE

typedef struct state {
    uint8_t inited;
    uint8_t in_temp;
    env_reading_t temperature;
    uint32_t nextsample;
} ctx_t;
static ctx_t state;

static const int32_t temperature_error[] = {
    ERR_TEMP(-55, 2), ERR_TEMP(-30, 1), ERR_TEMP(-10, 0.5), ERR_TEMP(85, 0.5), ERR_TEMP(100, 1),
    ERR_TEMP(125, 2), ERR_END};

static void ds18b20_cmd(uint8_t cmd) {
    if (one_reset() != 0)
        JD_PANIC();
    if (cmd != DS18B20_READ_ROM)
        one_write(DS18B20_SKIP_ROM);
    one_write(cmd);
}

static int read_data(void) {
    ds18b20_cmd(DS18B20_READ_SCRATCHPAD);
    int16_t v = one_read();
    v |= one_read() << 8;
    return v;
}

static void ds18b20_init(void) {
    ctx_t *ctx = &state;
    if (ctx->inited)
        return;
    ctx->inited = 1;
    one_init();
    ctx->temperature.min_value = SCALE_TEMP(-55);
    ctx->temperature.max_value = SCALE_TEMP(125);

    ds18b20_cmd(DS18B20_READ_ROM);
    for (int i = 0; i < 8; ++i)
        DMESG("r[%d] = %x", i, one_read());

    ds18b20_cmd(DS18B20_WRITE_SCRATCHPAD);
    one_write(0x00);                           // TH
    one_write(0x00);                           // TL
    one_write(((RESOLUTION - 9) << 5) | 0x1F); // Config
}

static void ds18b20_process(void) {
    ctx_t *ctx = &state;

    if (jd_should_sample_delay(&ctx->nextsample, CONVERSION_MS * 1000)) {
        if (ctx->in_temp) {
            int v = read_data();
            ctx->in_temp = 0;
            env_set_value(&ctx->temperature, v << (PRECISION - 4), temperature_error);
            ctx->nextsample = now + SAMPLING_MS * 1000;
            // DMESG("t=%dC", ctx->temperature.value >> PRECISION);
            ctx->inited = 2;
        } else {
            ctx->in_temp = 1;
            ds18b20_cmd(DS18B20_CONVERT_T);
            // DMESG("conv");
        }
    }
}

static void *ds18b20_temperature(void) {
    ctx_t *ctx = &state;
    if (ctx->inited < 2)
        return NULL;
    return &ctx->temperature;
}

const env_sensor_api_t temperature_ds18b20 = {
    .init = ds18b20_init,
    .process = ds18b20_process,
    .get_reading = ds18b20_temperature,
};
