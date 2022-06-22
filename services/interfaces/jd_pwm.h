// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef __JD_PWM_H
#define __JD_PWM_H

uint8_t jd_pwm_init(uint8_t pin, uint32_t period, uint32_t duty, uint8_t prescaler);
void jd_pwm_set_duty(uint8_t pwm_id, uint32_t duty);
void jd_pwm_enable(uint8_t pwm_id, bool enabled);

// rotary encoder
uint8_t encoder_init(uint8_t pinA, uint8_t pinB);
uint32_t encoder_get(uint8_t pwm);

#endif