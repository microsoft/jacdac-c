#include "jd_drivers.h"
#include "jd_util.h"

#ifdef TEMP_SHTC3

#define SAMPLING_MS 500
#define PRECISION 10

#ifndef SHTC3_ADDR
#define SHTC3_ADDR 0x70
#endif

#define SHTC3_MEASURE_NORMAL 0x7866
#define SHTC3_MEASURE_LOW_POWER 0x609C

#define SHTC3_SOFTRESET 0x805D
#define SHTC3_ID 0xEFC8
#define SHTC3_SLEEP 0xB098
#define SHTC3_WAKEUP 0x3517

typedef struct state {
    uint8_t inited;
    uint8_t read_issued;
    uint32_t humidity;
    uint32_t temp;
    uint32_t nextsample;
} ctx_t;
static ctx_t state;

static void send_cmd(uint16_t cmd) {
    if (i2c_write_reg16_buf(SHTC3_ADDR, cmd, NULL, 0))
        hw_panic();
}

static void wake(void) {
    send_cmd(SHTC3_WAKEUP);
    target_wait_us(300); // 200us seems minimum; play it safe with 300us
}

static void weather_hw_process(void) {
    ctx_t *ctx = &state;
    if (!ctx->inited) {
        ctx->inited = 1;
        i2c_init();
        wake();
        int id = i2c_read_reg16(SHTC3_ADDR, SHTC3_ID);
        DMESG("SHTC3 id=%x", id);
        if (id <= 0)
            hw_panic();
        send_cmd(SHTC3_SLEEP);
    }

    // the 20ms here is just for readings, we actually sample at SAMPLING_MS
    // the datasheet says max reading time is 12.1ms; give a little more time
    if (jd_should_sample(&ctx->nextsample, 20000)) {
        if (!ctx->read_issued) {
            ctx->read_issued = 1;
            wake();
            send_cmd(SHTC3_MEASURE_NORMAL);
        } else {
            uint8_t data[6];
            if (i2c_read_ex(SHTC3_ADDR, data, sizeof(data)))
                hw_panic();
            uint16_t temp = (data[0] << 8) | data[1];
            uint16_t hum = (data[3] << 8) | data[4];
            // DMESG("t:%x h:%x", temp, hum);
            send_cmd(SHTC3_SLEEP);
            ctx->read_issued = 0;
            ctx->nextsample = now + SAMPLING_MS * 1000;
            ctx->humidity = 100 * hum >> 6;             // u22.10 format for humidity
            ctx->temp = (175 * temp >> 6) - (45 << 10); // i22.10 format
            // DMESG("%d C %d %%", ctx->temp >> 10, ctx->humidity >> 10);
        }
    }
}

uint32_t hw_temp(void) {
    weather_hw_process();
    return state.temp;
}

uint32_t hw_humidity(void) {
    weather_hw_process();
    return state.humidity;
}

#endif // TEMP_SHTC3