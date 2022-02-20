#ifndef DRV_DIGITAL_OUT_H
#define DRV_DIGITAL_OUT_H

#include "drv_name.h"

typedef struct {
    uint8_t pin;
} digital_out_t;

static inline int digital_out_init(digital_out_t *out, pin_name_t name) {
    out->pin = name;
    pin_setup_output(name);
    return 0;
}

static inline void digital_out_high(digital_out_t *out) {
    pin_set(out->pin, 1);
}

static inline void digital_out_low(digital_out_t *out) {
    pin_set(out->pin, 0);
}

static inline void digital_out_toggle(digital_out_t *out) {
    pin_toggle(out->pin);
}

static inline void digital_out_write(digital_out_t *out, uint8_t value) {
    pin_set(out->pin, value);
}

#endif
