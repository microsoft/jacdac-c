#include "color.h"
#include "interfaces/jd_sensor_api.h"

#ifdef MIKROBUS_AVAILABLE

static color_t ctx;

static void glue_color_init(void) {
    color_cfg_t cfg;
    color_cfg_setup(&cfg);
    COLOR_MAP_MIKROBUS(cfg, NA);
    if (color_init_(&ctx, &cfg))
        JD_PANIC();
    color_default_cfg(&ctx);
    color_write_byte(&ctx, COLOR_REG_RGBC_TIME, COLOR_RGBC_TIME_154ms);
    color_set_led(&ctx, 1, 1, 1);
}

static void glue_color_sleep(void) {
    color_set_led(&ctx, 0, 0, 0);
    //    color_write_byte(&ctx, COLOR_REG_ENABLE, 0x00);
}

// this is what we get on "white"
#define SCALE_R 50
#define SCALE_G 34
#define SCALE_B 24

static void *glue_color_get_sample(void) {
    static uint32_t sample[4];
    uint32_t r = color_read_data(&ctx, COLOR_COLOR_DATA_RED);
    uint32_t g = color_read_data(&ctx, COLOR_COLOR_DATA_GREEN);
    uint32_t b = color_read_data(&ctx, COLOR_COLOR_DATA_BLUE);
    uint32_t c = color_read_data(&ctx, COLOR_COLOR_DATA_CLEAR);
    sample[0] = r * ((1 << 16) / SCALE_R);
    sample[1] = g * ((1 << 16) / SCALE_G);
    sample[2] = b * ((1 << 16) / SCALE_B);
    sample[3] = c << 16;
    return sample;
}

const color_api_t color_click = {
    .init = glue_color_init,
    .get_reading = glue_color_get_sample,
    .sleep = glue_color_sleep,
};

#endif
