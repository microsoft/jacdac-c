#include "jd_drivers.h"

#ifdef PIN_SCS

#define SAMPLING_MS 500

typedef struct state {
    uint8_t inited;
    env_reading_t temperature;
    uint32_t nextsample;
} ctx_t;
static ctx_t state;

static const int32_t temperature_error[] = {ERR_TEMP(-200, 2), ERR_TEMP(700, 2), ERR_TEMP(701, 4),
                                            ERR_TEMP(1350, 4), ERR_END};

static void max31855_init(void) {
    ctx_t *ctx = &state;
    if (ctx->inited)
        return;

    ctx->temperature.min_value = SCALE_TEMP(-200);
    ctx->temperature.max_value = SCALE_TEMP(1350);

    ctx->nextsample = now;

    pin_set(PIN_SCS, 1);
    pin_setup_output(PIN_SCS);

    sspi_init(1, 1, 1);

    ctx->inited = 1;
}

static void max31855_process(void) {
    ctx_t *ctx = &state;

    if (jd_should_sample(&ctx->nextsample, SAMPLING_MS * 1000)) {
        uint8_t data[4];

        pin_set(PIN_SCS, 0);
        sspi_rx(data, sizeof(data));
        pin_set(PIN_SCS, 1);

        int16_t rtemp = (data[0] << 8) | data[1];
        if (rtemp & 1) {
            DMESG("fault! f=%d", data[3] & 7);
            JD_PANIC();
        }

        rtemp &= ~3;
        int32_t temp = rtemp << 6;

        env_set_value(&ctx->temperature, temp, temperature_error);
        ctx->inited = 2;
    }
}

static void *max31855_temperature(void) {
    ctx_t *ctx = &state;
    if (ctx->inited >= 2)
        return &ctx->temperature;
    return NULL;
}

const env_sensor_api_t temperature_max31855 = {
    .init = max31855_init,
    .process = max31855_process,
    .get_reading = max31855_temperature,
};

#endif