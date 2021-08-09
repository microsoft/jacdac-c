// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef __JD_SERVICE_INIT_H
#define __JD_SERVICE_INIT_H

#include "jd_sensor.h"
#include "interfaces/jd_accel.h"
#include "interfaces/jd_environment.h"

// For pins where PWM or ADC is used, only certain pins are possible.
// Best search for an existing call of the *_init() function and use the same pins.

// Accelerometer service. Uses I2C pins.
void accelerometer_init(const accelerometer_api_t *hw);
void accelerometer_data_transform(int32_t sample[3]);

// Rotary encoder service; pin0/1 are connected to two pins of the encoder
void rotaryencoder_init(uint8_t pin0, uint8_t pin1, uint16_t clicks_per_turn);

// Controller for RGB LED strips (LED pixel service)
// Supported: WS2812B, APA102, SK9822.
// Uses SPI pins.
// In board.h you can define LED_PIXEL_LOCK_TYPE and/or LED_PIXEL_LOCK_NUM_PIXELS to disable writes
// the respective registers.
void ledpixel_init(uint8_t default_ledpixel_type, uint32_t default_num_pixels,
                   uint32_t default_max_power, uint8_t variant);

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

// Temperature and humidity services; often from a single I2C sensor (defined in board.h)
void thermometer_init(env_function_t read);
void humidity_init(env_function_t read);

// Potentiometer service.
// pinM is sampled, while pinL is set low, and pinH is set high.
// When not sampling, pinL and pinH are left as inputs, reducing power consumption.
// pinL and/or pinH can be NO_PIN (it only really makes sense to use one of them).
void potentiometer_init(uint8_t pinL, uint8_t pinM, uint8_t pinH);

// Motor (H-Bridge) control.
// pin1/pin2 are active-high PWM-based input pins.
// pin_nSLEEP is active-low sleep pin (or equivalently active-high EN pin).
// This has been tested with DRV8837.
void motor_init(uint8_t pin1, uint8_t pin2, uint8_t pin_nSLEEP);

// Mono-light service.
// Shows "animations" on a single LED, or a strip of parallel-connected LEDs.
void pwm_light_init(uint8_t pin);

// A touch-sensing service with multiple inputs. pins is a '-1'-terminated array of ADC-enabled
// input pins. Each pin should be connected to a separate capacitive touch electrode, and also
// pulled down with a 2M resistor. This is very preliminary.
void multitouch_init(const uint8_t *pins);

// A single-pin touch control; similar to the multi-touch one above.
// Also preliminary.
void touch_init(uint8_t pin);

// Power-delivery service. Needs work.
void power_init(uint8_t pre_sense, uint8_t gnd_sense, uint8_t overload, uint8_t pulse);

// This is automatically called if you enable JD_CONSOLE in board.h.
void jdcon_init(void);

// Not implemented.
void oled_init(void);

#define JD_JOYSTICK_BUTTONS_INFER_12 0xfff
// pinX/pinY are the wipers for the two potentiometers on the joystick
// pinL is the lower reference voltage and pinH is the higher reference voltage (usually you only
// want one) if the joystick is digital, set pinX to NO_PIN (other pin* don't matter) for every bit
// (1<<X) set in buttons_available, the corresponding pinBtns[X] and ledPins[X] must be set properly
// (possibly to NO_PIN)
typedef struct {
    // sync these with REG_DEFINITION() in joystick.c !
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
} joystick_params_t;

// Initialises a joystick service, in particular an analog version.
// variant is set according to the service specification. See joystick.h
void joystick_init(const joystick_params_t *params);

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

// initialises a relay service. 
// relay state is the driver pin, some relays also have a feedback pin to show whether they are active or not (relay_feedback)
// in some cases, hw mfrs may want to light an LED when relay is active (relay_status)
// For variant values, please see relay.h
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

#endif