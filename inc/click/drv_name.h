#pragma once

#include "jd_drivers.h"

typedef int pin_name_t;
typedef int err_t;

#define HAL_PIN_NC (-1)

#define MIKROBUS_SCL PIN_SCL
#define MIKROBUS_SDA PIN_SDA
#define MIKROBUS_AN PIN_AN
#define MIKROBUS_PWM PIN_PWM
#define MIKROBUS_INT PIN_INT
#define MIKROBUS_SCK PIN_SCK
#define MIKROBUS_RST PIN_RST

#ifdef PIN_CS
#define MIKROBUS_CS PIN_CS
#endif
#ifdef PIN_TX
#define MIKROBUS_TX PIN_TX
#endif
#ifdef PIN_MISO
#define MIKROBUS_MISO PIN_MISO
#endif
#ifdef PIN_RX
#define MIKROBUS_RX PIN_RX
#endif
#ifdef PIN_MOSI
#define MIKROBUS_MOSI PIN_MOSI
#endif

#ifdef PIN_TX_MISO
#define MIKROBUS_TX PIN_TX_MISO
#define MIKROBUS_MISO PIN_TX_MISO
#endif

#ifdef PIN_RX_MOSI
#define MIKROBUS_RX PIN_RX_MOSI
#define MIKROBUS_MOSI PIN_RX_MOSI
#endif

#ifdef PIN_RX_CS
#define MIKROBUS_RX PIN_RX_CS
#define MIKROBUS_CS PIN_RX_CS
#endif

#define MIKROBUS(mikrobus, pin) pin
