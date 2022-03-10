#ifndef DRV_DIGITAL_IN_H
#define DRV_DIGITAL_IN_H
#include "drv_name.h"

typedef struct {
    uint8_t pin;
} digital_in_t;

static inline int digital_in_init(digital_in_t *in, pin_name_t name) {
    pin_setup_input(name, PIN_PULL_NONE);
    in->pin = name;
    return 0;
}

static inline uint8_t digital_in_read(digital_in_t *in) {
    return pin_get(in->pin);
}
#endif
