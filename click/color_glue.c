#include "color.h"
#include "interfaces/jd_sensor_api.h"

#ifdef MIKROBUS_AVAILABLE

static color_t ctx;

static void glue_color_init(void) {
    color_cfg_t cfg;
    color_cfg_setup(&cfg);
    COLOR_MAP_MIKROBUS(cfg, NA);
    if (color_init_(&ctx, &cfg))
        hw_panic();
    color_default_cfg(&ctx);
    color_write_byte(&ctx, COLOR_REG_RGBC_TIME, COLOR_RGBC_TIME_154ms);
    color_set_led(&ctx, 1, 1, 1);
}

static void glue_color_sleep(void) {
    color_set_led(&ctx, 0, 0, 0);
    color_write_byte(&ctx, COLOR_REG_ENABLE, 0x00);
}

// this is what we get on "white"
#define SCALE_R 50
#define SCALE_G 34
#define SCALE_B 24

static void glue_color_get_sample(uint32_t sample[4]) {
    sample[0] = (color_read_data(&ctx, COLOR_COLOR_DATA_RED) << 16) / SCALE_R;
    sample[1] = (color_read_data(&ctx, COLOR_COLOR_DATA_GREEN) << 16) / SCALE_G;
    sample[2] = (color_read_data(&ctx, COLOR_COLOR_DATA_BLUE) << 16) / SCALE_B;
    sample[3] = color_read_data(&ctx, COLOR_COLOR_DATA_CLEAR) << 16;
}

const color_api_t color_click = {
    .init = glue_color_init,
    .get_sample = (get_sample_t)glue_color_get_sample,
    .sleep = glue_color_sleep,
};

#endif
