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
void acc_init(const acc_api_t *hw);
void acc_data_transform(int32_t sample[3]);

// Rotary encoder service; pin0/1 are connected to two pins of the encoder
void rotary_init(uint8_t pin0, uint8_t pin1, uint16_t clicks_per_turn);

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
// backlight_pin is active low, and can be disabled with -1.
void btn_init(uint8_t pin, bool active, uint8_t backlight_pin);

// Temperature and humidity services; often from a single I2C sensor (defined in board.h)
void temp_init(env_function_t read);
void humidity_init(env_function_t read);

// Potentiometer service.
// pinM is sampled, while pinL is set low, and pinH is set high.
// When not sampling, pinL and pinH are left as inputs, reducing power consumption.
// pinL and/or pinH can be -1 (it only really makes sense to use one of them).
void potentiometer_init(uint8_t pinL, uint8_t pinM, uint8_t pinH);

// GamePad (arcade controls) service.
// Pins are given in order: L U R D A B Menu MenuAlt Reset Exit
// Missing pins can be given as -1.
// led_pins is the same size as pins, and is used to light-up arcade buttons. Can be NULL.
void gamepad_init(uint8_t num_pins, const uint8_t *pins, const uint8_t *led_pins);

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

// Initialises a joystick service, in particular an analog version.
// pinX/pinY are the wipers for the two potentiometers on the joystick
// pinL is the lower reference voltage and pinH is the higher reference voltage.
// variant is set according to the service specification. See joystick.h
void analog_joystick_init(uint8_t pinL, uint8_t pinH, uint8_t pinX, uint8_t pinY, uint8_t variant);

#endif