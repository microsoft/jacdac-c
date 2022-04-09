// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef __JD_ADC_H
#define __JD_ADC_H

void adc_init_random(void);
uint16_t adc_read_temp(void);
bool adc_can_read_pin(uint8_t pin);

void adc_prep_read_pin(uint8_t pin);
uint16_t adc_convert(void);
void adc_disable(void);

// equivalent to the three steps above; result is always scaled to 16 bits
uint16_t adc_read_pin(uint8_t pin);

#endif