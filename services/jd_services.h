// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef __JD_SERVICE_INIT_H
#define __JD_SERVICE_INIT_H

#include "jd_sensor.h"
#include "interfaces/jd_sensor_api.h"
#include "jacdac/dist/c/potentiometer.h"

// For pins where PWM or ADC is used, only certain pins are possible.
// Best search for an existing call of the *_init() function and use the same pins.

// Accelerometer service. Uses I2C pins.
void accelerometer_init(const accelerometer_api_t *hw);
void accelerometer_data_transform(int32_t sample[3]);

void gyroscope_init(const gyroscope_api_t *hw);
void gyroscope_data_transform(int32_t sample[3]);

#define ROTARY_ENC_FLAG_DENSE 0x01
#define ROTARY_ENC_FLAG_INVERTED 0x02
// Rotary encoder service; pin0/1 are connected to two pins of the encoder
void rotaryencoder_init(uint8_t pin0, uint8_t pin1, uint16_t clicks_per_turn, uint32_t flags);

// Controller for RGB LED strips (LED pixel service)
// Supported: WS2812B, APA102, SK9822.
// Uses SPI pins.
// In board.h you can define LED_STRIP_LOCK_TYPE and/or LED_STRIP_LOCK_NUM_PIXELS to disable writes
// the respective registers.
void ledstrip_init(uint8_t default_ledstrip_type, uint32_t default_num_pixels,
                   uint32_t default_max_power, uint8_t variant);

// This is similar, but for short strips, under 64 pixels.
// The type and number of pixels are always fixed in this case.
void leddisplay_init(uint8_t leddisplay_type, uint32_t num_pixels, uint32_t default_max_power,
                     uint8_t variant);

// Sound (buzzer) service on given pin. Uses BUZZER_OFF config from board.h.
void buzzer_init(uint8_t pin);

// this has to match REG_DEFINITION() in servo.c!
typedef struct {
    uint8_t pin;
    uint8_t fixed; // if set min/max angle/pulse can't be modified by register writes
    uint8_t power_pin;
    int32_t min_angle;
    int32_t max_angle;
    int16_t min_pulse;
    int16_t max_pulse;
} servo_params_t;

// Servo control service.
void servo_init(const servo_params_t *params);

// Button service.
// The button is active-low if active==0, and active-low when active==1.
// An internal pull-up/down is setup correspondingly.
// backlight_pin is active low, and can be disabled with NO_PIN.
void button_init(uint8_t pin, bool active, uint8_t backlight_pin);

void switch_init(uint8_t pin, bool active, uint8_t variant);

// Temperature and humidity services; often from a single I2C sensor (defined in board.h)
void temperature_init(const env_sensor_api_t *api);
void humidity_init(const env_sensor_api_t *api);
void barometer_init(const env_sensor_api_t *api);

// Air quality sensors
void eco2_init(const env_sensor_api_t *api);
void tvoc_init(const env_sensor_api_t *api);

// Potentiometer service.
// pinM is sampled, while pinL is set low, and pinH is set high.
// When not sampling, pinL and pinH are left as inputs, reducing power consumption.
// pinL and/or pinH can be NO_PIN (it only really makes sense to use one of them).
#define potentiometer_init(pin_L, pin_M, pin_H)                                                    \
    ANALOG_SRV_DEF(JD_SERVICE_CLASS_POTENTIOMETER, .pinL = pin_L, .pinM = pin_M, .pinH = pin_H)

// Motor (H-Bridge) control.
// pin1/pin2 are active-high PWM-based input pins.
// pin_nSLEEP is active-low sleep pin (or equivalently active-high EN pin).
// This has been tested with DRV8837.
void motor_init(uint8_t pin1, uint8_t pin2, uint8_t pin_nSLEEP);

// Mono-light service.
// Shows "animations" on a single LED, or a strip of parallel-connected LEDs.
void pwm_light_init(uint8_t pin);

#if 0
// A touch-sensing service with multiple inputs. pins is a '-1'-terminated array of ADC-enabled
// input pins. Each pin should be connected to a separate capacitive touch electrode, and also
// pulled down with a 2M resistor. This is very preliminary.
void multitouch_init(const uint8_t *pins);
#endif

// A single-pin touch control; similar to the multi-touch one above.
// Also preliminary.
void touch_init(uint8_t pin);

// Power-delivery service.
typedef struct {
    uint8_t pin_fault; // active low
    uint8_t pin_en;
    uint8_t pin_pulse;
    uint8_t en_active_high;
} power_config_t;
void power_init(const power_config_t *cfg);

// This is automatically called if you enable JD_CONSOLE in board.h.
void jdcon_init(void);

// Not implemented.
void oled_init(void);

void lorawan_init(void);

#define JD_GAMEPAD_BUTTONS_INFER_12 0xfff
// pinX/pinY are the wipers for the two potentiometers on the gamepad
// pinL is the lower reference voltage and pinH is the higher reference voltage (usually you only
// want one) if the gamepad is digital, set pinX to NO_PIN (other pin* don't matter) for every bit
// (1<<X) set in buttons_available, the corresponding pinBtns[X] and ledPins[X] must be set properly
// (possibly to NO_PIN)
typedef struct {
    // sync these with REG_DEFINITION() in gamepad.c !
    uint32_t buttons_available;
    uint8_t variant;
    // these don't need to be synced
    bool active_level;
    uint8_t pinL;
    uint8_t pinH;
    uint8_t pinX;
    uint8_t pinY;
    uint8_t pinBtns[16];
    uint8_t pinLeds[16];
} gamepad_params_t;

// Initialises a gamepad service, in particular an analog version.
// variant is set according to the service specification. See gamepad.h
void gamepad_init(const gamepad_params_t *params);

// all mult_* values default to 0xff
typedef struct {
    // these registers are optional - if you leave them at 0, the service will not respond to them
    // keep in sync with REG_DEFINITION() in led.c!
    uint8_t variant;
    uint16_t wave_length;
    uint16_t led_count;
    uint16_t max_power;
    uint16_t luminous_intensity;

    // these are settings, not registers
    uint8_t pin_r; // also used for monochrome
    uint8_t mult_r;
    uint8_t pin_g; // use NO_PIN for monochrome
    uint8_t mult_g;
    uint8_t pin_b; // use NO_PIN for monochrome
    uint8_t mult_b;
    uint8_t active_high; // common cathode
    uint16_t pwm_period; // defaults to 512; can't be less than 512
} led_params_t;
void led_service_init(const led_params_t *params);

// Color sensor service.
void color_init(const color_api_t *api);

typedef struct {
    void (*init)(uint8_t volume, uint32_t rate, uint32_t pitch, char *language);
    void (*speak)(char *);
    void (*set_volume)(uint8_t);
    void (*set_rate)(uint32_t);
    bool (*set_language)(char *);
    bool (*set_pitch)(uint32_t);
    void (*pause)(void);
    void (*resume)(void);
    void (*cancel)(void);
} speech_synth_api_t;
void speech_synthesis_init(const speech_synth_api_t *api);
extern const speech_synth_api_t tts_click;

typedef struct {
    void (*init)(void);
    void (*channel_clear)(uint8_t);
    void (*channel_set)(uint8_t, int);
    void (*clear_all)(void);
    void (*write_channels)(void);
    uint16_t (*write_raw)(uint16_t);
} hbridge_api_t;
// dot matrix (pixel set/clear)
void braille_dm_init(const hbridge_api_t *api, uint16_t rows, uint16_t cols,
                     const uint8_t *cell_map);
// character interface (character level set/clear)
void braille_char_init(const hbridge_api_t *api, uint16_t rows, uint16_t cols,
                       const uint8_t *cell_map);
extern const hbridge_api_t ncv7726b;

// initialises a relay service.
// relay state is the driver pin, some relays also have a feedback pin to show whether they are
// active or not (relay_feedback) in some cases, hw mfrs may want to light an LED when relay is
// active (relay_status) For variant values, please see relay.h
typedef struct {
    uint8_t relay_variant;
    uint32_t max_switching_current;
    uint8_t pin_relay_drive;
    uint8_t pin_relay_feedback;
    uint8_t pin_relay_led;
    uint8_t drive_active_lo;
    uint8_t led_active_lo;
} relay_params_t;
void relay_service_init(const relay_params_t *params);

// Flex service.
// pinM is sampled, while pinL is set low, and pinH is set high.
// When not sampling, pinL and pinH are left as inputs, reducing power consumption.
// pinL and/or pinH can be NO_PIN (it only really makes sense to use one of them).
void flex_init(uint8_t pinL, uint8_t pinM, uint8_t pinH);

typedef struct {
    void (*init)(void);
    void (*write_amplitude)(uint8_t amplitude, uint32_t duration_ms);
} vibration_motor_api_t;
void vibration_motor_init(const vibration_motor_api_t *api);
extern const vibration_motor_api_t aw86224fcr;

void uvindex_init(const env_sensor_api_t *api);
void illuminance_init(const env_sensor_api_t *api);

typedef struct motion_cfg {
    uint8_t pin;
    uint8_t variant; // PIR==1
    bool inactive;
    uint16_t angle;
    uint32_t max_distance;
} motion_cfg_t;
void motion_init(const motion_cfg_t *cfg);

#endif