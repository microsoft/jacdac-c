#pragma once

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "board.h"
#include "dmesg.h"
#include "pinnames.h"
#include "services.h"
#include "blhw.h"

#ifdef BL
#define DEVICE_CLASS(dev_class, dev_class_name)                                                    \
    struct bl_info_block __attribute__((section(".devinfo"), used)) bl_info = {                    \
        .devinfo =                                                                                 \
            {                                                                                      \
                .magic = DEV_INFO_MAGIC,                                                           \
                .device_id = 0xffffffffffffffffULL,                                                \
                .device_class = dev_class,                                                         \
            },                                                                                     \
        .random_seed0 = 0xffffffff,                                                                \
        .random_seed1 = 0xffffffff,                                                                \
        .reserved0 = 0xffffffff,                                                                   \
        .reserved1 = 0xffffffff,                                                                   \
    };
#else
#define DEVICE_CLASS(dev_class, dev_class_name) const char app_dev_class_name[] = dev_class_name;
#endif

void init_services(void);

void ctrl_init(void);

void jdcon_init(void);
void jdcon_logv(int level, const char *format, va_list ap);
void jdcon_log(const char *format, ...);
void jdcon_warn(const char *format, ...);

void acc_init(void);
void crank_init(uint8_t pin0, uint8_t pin1);
void light_init(void);
void snd_init(uint8_t pin);
void pwm_light_init(uint8_t pin);
void servo_init(uint8_t pin);
void btn_init(uint8_t pin, uint8_t blpin);
void touch_init(uint8_t pin);
void oled_init(void);
void temp_init(void);
void humidity_init(void);
void gamepad_init(uint8_t num_pins, const uint8_t *pins, const uint8_t *ledPins);
void power_init(void);
void slider_init(uint8_t pinL, uint8_t pinM, uint8_t pinH);

extern const char app_dev_class_name[];