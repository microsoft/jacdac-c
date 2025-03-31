#include "jd_services.h"
#include "interfaces/jd_pins.h"
#include "jacdac/dist/c/lightbulb.h"
#include "interfaces/jd_pwm.h"
#include "..\..\jacdac-stm32x0\\bl\blutil.h"
#include "..\..\jacdac-stm32x0\stm32\jdstm.h"

#define PRESCALER 1     // 1:1 division of the timer clock.
#define PWM_PERIOD 4096 // Timer counts from 0 to (PWM_PERIOD - 1).
#define DEFAULT_BRIGHTNESS_U08 0

REG_DEFINITION(                                        
    bulb_regs,                         
    REG_SRV_COMMON,                                    
    REG_U8(JD_LIGHT_BULB_REG_BRIGHTNESS), 
)


struct srv_state {
    SRV_COMMON;
    uint8_t brightnessU08;

    // end of registers
    uint8_t pin;
    uint8_t pwm;
    uint8_t brightness;
    const uint16_t *pbrightness_to_pwmLookupTable;
};



// Function to get PWM value from brightness
uint16_t get_pwm_from_brightness(srv_t *state, uint8_t brightness) {
    if (state->pbrightness_to_pwmLookupTable != NULL)
        return state->pbrightness_to_pwmLookupTable[brightness];
    else
        return PWM_PERIOD;
}



void bulb_process(srv_t *state) {    

}



void bulb_handle_packet(srv_t *state, jd_packet_t *pkt) {
    if (service_handle_register_final(state, pkt, bulb_regs) == JD_LED_STRIP_REG_BRIGHTNESS) {
        uint8_t prevBrightness = state->brightness;
        state->brightness = (state->brightnessU08 * 100 + 128) / 256;

        uint16_t pwmDuty = get_pwm_from_brightness(state, state->brightness);
        jd_pwm_set_duty(state->pwm, pwmDuty);

        if (state->brightness == 0 && prevBrightness > 0) {
            jd_pwm_enable(state->pwm, 0);
        } else if (state->brightness > 0 && prevBrightness == 0) {
            jd_pwm_enable(state->pwm, 1);
        }
        return;
    }
}




SRV_DEF(bulb, JD_SERVICE_CLASS_LIGHT_BULB);
void lightbulb_init(const uint8_t pin, const uint16_t *pbrightness_to_pwmLookupTable) {
    SRV_ALLOC(bulb);
    state->pin = pin;
    state->pbrightness_to_pwmLookupTable = pbrightness_to_pwmLookupTable;

    pin_setup_output(state->pin);
    pin_set(state->pin, 0);
    state->brightnessU08 = DEFAULT_BRIGHTNESS_U08;

    state->pwm = jd_pwm_init(state->pin, PWM_PERIOD, 0, 1);
    jd_pwm_enable(state->pwm, 0);
}
