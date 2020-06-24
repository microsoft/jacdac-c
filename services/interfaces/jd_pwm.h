#ifndef __JD_PWM_H
#define __JD_PWM_H

uint8_t pwm_init(uint8_t pin, uint32_t period, uint32_t duty, uint8_t prescaler);

void pwm_set_duty(uint8_t pwm_id, uint32_t duty);

void pwm_enable(uint8_t pwm_id, bool enabled);

#endif