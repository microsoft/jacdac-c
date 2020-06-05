#include "jdsimple.h"

uint32_t now;

static const uint8_t output_pins[] = {OUTPUT_PINS};

void pwr_pin_enable(int en) {
#ifdef PWR_PIN_PULLUP
    if (en) {
        pin_setup_output(PIN_PWR);
        pin_set(PIN_PWR, 0);
    } else {
        pin_setup_input(PIN_PWR, 0);
    }
#else
    pin_set(PIN_PWR, !en);
#endif
}

void led_init(void) {
    // To save power, especially in STOP mode,
    // configure all pins in GPIOA,B,C as analog inputs (except for SWD)
    for (unsigned i = 0; i < 16 * 3; ++i)
        if (i != 13 && i != 14) // 13/14 are SWD pins
            pin_setup_analog_input(i);

    // also do all GPIOF (note that it's not enough to just clear PF0 and PF1
    // - the other ones seem to still draw power in stop mode)
    for (unsigned i = 0; i < 16; ++i)
        pin_setup_analog_input(i + 0x50);

    // The effects of the pins shutdown above is quite dramatic - without the MCU can draw
    // ~100uA (but with wide random variation) in STOP; with shutdown we get a stable 4.3uA

    // setup all our output pins
    for (unsigned i = 0; i < sizeof(output_pins); ++i)
        pin_setup_output(output_pins[i]);

    // all power pins are reverse polarity
#ifndef PWR_PIN_PULLUP
    pwr_pin_enable(0);
#endif

#ifdef PIN_GLO0
    pin_set(PIN_GLO0, 1);
    pin_set(PIN_GLO1, 1);
#endif
}

void log_pin_set(int line, int v) {
#ifdef BUTTON_V0
    switch (line) {
    case 4:
        pin_set(PIN_LOG0, v);
        break;
    case 1:
        pin_set(PIN_LOG1, v);
        break;
    case 2:
        pin_set(PIN_LOG2, v);
        break;
    case 0:
        // pin_set(PIN_LOG3, v);
        break;
    }
#else
// do nothing
#endif
}

static uint64_t led_off_time;

// AP2112
// reg 85ua
// reg back 79uA
// no reg 30uA

static void do_nothing(void) {}
void sleep_forever(void) {
    target_wait_us(500000);
    led_set(0);
    int cnt = 0;
    for (;;) {
        pin_pulse(PIN_P0, 2);
        tim_set_timer(10000, do_nothing);
        if (cnt++ > 30) {
            cnt = 0;
            adc_read_pin(PIN_LED_GND);
            adc_read_temp();
        }
        pwr_sleep();
        // rtc_deepsleep();
        // rtc_set_to_seconds_and_standby();
    }
}

void led_set(int state) {
    pin_set(PIN_LED, state);
}

void led_blink(int us) {
    led_off_time = tim_get_micros() + us;
    led_set(1);
}

int main(void) {
    led_init();
    led_set(1);

    if ((device_id() + 1) == 0)
        target_reset();

    alloc_init();

    tim_init();

    adc_init_random(); // 300b
    rtc_init();

    // sleep_forever();

    txq_init();
    jd_init();

    app_init_services();

#if 0
    while(1) {
        led_set(1);
        pwr_pin_enable(1);
        target_wait_us(300000);
        pwr_pin_enable(0);
        target_wait_us(100000);
        led_set(0);
        target_wait_us(10 * 1000 * 1000);
    }
#endif

    led_blink(200000); // initial (on reset) blink

    // When BMP attaches, and we're in deep sleep mode, it will scan us as generic Cortex-M0.
    // The flashing scripts scans once, resets the target (using NVIC), and scans again.
    // The delay is so that the second scan detects us as the right kind of chip.
    uint32_t startup_wait = tim_get_micros() + 300000;

    while (1) {
        uint64_t now_long = tim_get_micros();
        now = (uint32_t)now_long;

        app_process();

        if (led_off_time) {
            int timeLeft = led_off_time - now_long;
            if (timeLeft <= 0) {
                led_off_time = 0;
                led_set(0);
            } else if (timeLeft < 1000) {
                continue; // don't sleep
            }
        }

        if (startup_wait) {
            if (in_future(startup_wait))
                continue; // no sleep
            else
                startup_wait = 0;
        }

        pwr_sleep();
    }
}

static void led_panic_blink(void) {
    pin_setup_output(PIN_LED);
    pin_setup_output(PIN_LED_GND);
    pin_set(PIN_LED_GND, 0);
    led_set(1);
    target_wait_us(70000);
    led_set(0);
    target_wait_us(70000);
}

void jd_panic(void) {
    DMESG("PANIC!");
    target_disable_irq();
    while (1) {
        led_panic_blink();
    }
}

void fail_and_reset(void) {
    DMESG("FAIL!");
    target_disable_irq();
    for (int i = 0; i < 10; ++i) {
        led_panic_blink();
    }
    target_reset();
}
