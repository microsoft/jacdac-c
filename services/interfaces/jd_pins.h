// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef __JD_IO_H
#define __JD_IO_H

void pin_set(int pin, int v);

void pin_setup_output(int pin);

void pin_set_opendrain(int pin);

void pin_toggle(int pin);

int pin_get(int pin);

// pull: -1, 0, 1
void pin_setup_input(int pin, int pull);

void pin_setup_output_af(int pin, int af);

void pin_setup_analog_input(int pin);

void pin_pulse(int pin, int times);

#endif