#ifndef DRV_NAME_H
#define DRV_NAME_H

#include "jd_drivers.h"

typedef int pin_name_t;
typedef int err_t;

#define HAL_PIN_NC (-1)
#if MIKROBUS_AVAILABLE
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
#ifndef MIKROBUS_CS
#define MIKROBUS_CS PIN_RX_CS
#endif

#ifndef MIKROBUS_RX
#define MIKROBUS_RX PIN_RX_CS
#endif
#endif

#define MIKROBUS(mikrobus, pin) pin

#else

#define MIKROBUS_SCL NO_PIN
#define MIKROBUS_SDA NO_PIN
#define MIKROBUS_AN NO_PIN
#define MIKROBUS_PWM NO_PIN
#define MIKROBUS_INT NO_PIN
#define MIKROBUS_SCK NO_PIN
#define MIKROBUS_RST NO_PIN

#define MIKROBUS_CS NO_PIN
#define MIKROBUS_TX NO_PIN
#define MIKROBUS_MISO NO_PIN
#define MIKROBUS_RX NO_PIN
#define MIKROBUS_MOSI NO_PIN

#define MIKROBUS(mikrobus, pin) pin

#endif

static inline void Delay_1us(void) {
    jd_services_sleep_us(1);
}
static inline void Delay_10us(void) {
    jd_services_sleep_us(10);
}
static inline void Delay_22us(void) {
    jd_services_sleep_us(22);
}
static inline void Delay_50us(void) {
    jd_services_sleep_us(50);
}
static inline void Delay_80us(void) {
    jd_services_sleep_us(80);
}
static inline void Delay_500us(void) {
    jd_services_sleep_us(500);
}
static inline void Delay_1ms(void) {
    jd_services_sleep_us(1000);
}
static inline void Delay_5ms(void) {
    jd_services_sleep_us(5000);
}
static inline void Delay_8ms(void) {
    jd_services_sleep_us(8000);
}
static inline void Delay_10ms(void) {
    jd_services_sleep_us(10000);
}
static inline void Delay_100ms(void) {
    jd_services_sleep_us(100000);
}
static inline void Delay_1sec(void) {
    jd_services_sleep_us(1000000);
}
static inline void Delay_ms(int ms) {
    jd_services_sleep_us(ms * 1000);
}

#endif
