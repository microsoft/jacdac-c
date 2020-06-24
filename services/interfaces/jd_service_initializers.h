#ifndef __JD_SERVICE_INIT_H
#define __JD_SERVICE_INIT_H

void ctrl_init(void);
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
void power_init(uint8_t pre_sense, uint8_t gnd_sense, uint8_t overload, uint8_t pulse);
void slider_init(uint8_t pinL, uint8_t pinM, uint8_t pinH);
void motor_init(uint8_t pin1, uint8_t pin2, uint8_t pin_nsleep);

#endif