#include "jd_drivers.h"
#include <stdio.h>
#include <math.h>
#include "..\..\jacdac-stm32x0\\bl\blutil.h"
#include "..\..\jacdac-stm32x0\stm32\jdstm.h"

#define SAMPLING_MS 200   // sampling at SAMPLING_MS+CONVERSION_MS
#define CONVERSION_MS 800 // for 12 bits; less for less bits
#define RESOLUTION 12     // bits
#define PRECISION 10 // i22.10 format

// Sensor register addresses
#define DS18B20_SKIP_ROM 0xCC
#define DS18B20_READ_ROM 0x33
#define DS18B20_CONVERT_T 0x44
#define DS18B20_WRITE_SCRATCHPAD 0x4E
#define DS18B20_READ_SCRATCHPAD 0xBE


typedef struct state {
    uint8_t inited;
    uint8_t probePin;
    uint8_t in_temp;
    env_reading_t temperature;
    uint32_t nextsample;
} ctx_t;
static ctx_t state;


void format_float_manual(char *buffer, size_t size, float value, int precision) {
    // Get the integer part of the number
    int integer_part = (int)value;

    // Get the fractional part
    float fractional = value - (float)integer_part;
    if (fractional < 0)
        fractional = -fractional; // Ensure positivity for negative numbers

    // Scale the fractional part to the desired precision
    for (int i = 0; i < precision; i++) {
        fractional *= 10;
    }
    int fractional_part = (int)(fractional + 0.5f); // Round to the nearest value

    // Handle sign for negative values
    if (value < 0) {
        snprintf(buffer, size, "-%d.%0*d", -integer_part, precision, fractional_part);
    } else {
        snprintf(buffer, size, "%d.%0*d", integer_part, precision, fractional_part);
    }
}



void DS18B20_set_pin_output(void) {
    ctx_t *ctx = &state;
    GPIO_TypeDef *GPIOx = PIN_PORT(ctx->probePin);
    uint32_t currentpin = PIN_MASK(ctx->probePin);

    // Do it in the same order as LL_GPIO_Init() just in case
    LL_GPIO_SetPinSpeed(GPIOx, currentpin, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinOutputType(GPIOx, currentpin, LL_GPIO_OUTPUT_OPENDRAIN);
    LL_GPIO_SetPinPull(GPIOx, currentpin, LL_GPIO_PULL_NO);
    LL_GPIO_SetPinMode(GPIOx, currentpin, LL_GPIO_MODE_OUTPUT);
}



void DS18B20_set_pin_input(void) {
    ctx_t *ctx = &state;
    GPIO_TypeDef *GPIOx = PIN_PORT(ctx->probePin);
    uint32_t currentpin = PIN_MASK(ctx->probePin);

    LL_GPIO_SetPinSpeed(GPIOx, currentpin, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinPull(GPIOx, currentpin, LL_GPIO_PULL_NO);
    LL_GPIO_SetPinMode(GPIOx, currentpin, LL_GPIO_MODE_INPUT);
}



void DS18B20_set_data_pin(bool on) {
    ctx_t *ctx = &state;
    if (on)
        LL_GPIO_SetOutputPin(PIN_PORT(ctx->probePin), PIN_MASK(ctx->probePin));
    else
        LL_GPIO_ResetOutputPin(PIN_PORT(ctx->probePin), PIN_MASK(ctx->probePin));
}



int DS18B20_read_data_pin(void) {
    ctx_t *ctx = &state;
    return LL_GPIO_IsInputPinSet(PIN_PORT(ctx->probePin), PIN_MASK(ctx->probePin));
}



uint8_t DS18B20_start_sensor(void) {
    uint8_t response = 0;

    DS18B20_set_pin_output();
    DS18B20_set_data_pin(false);

    tim_delay_us(480);
    DS18B20_set_pin_input();
    tim_delay_us(80);
    
    if (!DS18B20_read_data_pin()) 
        response = 1;
    
    tim_delay_us(400);

    return response;
}




void DS18B20_writeData(uint8_t data) {
    DS18B20_set_pin_output();

    // DS17B20 one-wire timing is so sensitive that interrupts firing during the below sequence can 
    // cause spurious value read errors... so disable IRQs for the sequence
    target_disable_irq();
    for (uint8_t i = 0; i < 8; i++) {

        if (data & (1 << i)) {
            DS18B20_set_pin_output();
            DS18B20_set_data_pin(false);
            tim_delay_us(1);

            DS18B20_set_pin_input();
            tim_delay_us(60);
            continue;
        }

        DS18B20_set_pin_output();
        DS18B20_set_data_pin(false);
        tim_delay_us(60);

        DS18B20_set_pin_input();
    }
    target_enable_irq();
}



uint8_t DS18B20_read_data(void) {
    uint8_t value = 0;
    DS18B20_set_pin_input();

    // DS17B20 one-wire timing is so sensitive that interrupts firing during the below sequence can
    // cause spurious value read errors... so disable IRQs for the sequence
    target_disable_irq();
    for (uint8_t i = 0; i < 8; i++) {
        DS18B20_set_pin_output();
        DS18B20_set_data_pin(false);
        tim_delay_us(2);

        DS18B20_set_pin_input();
        tim_delay_us(2);
        if (DS18B20_read_data_pin()) {
            value |= 1 << i;
        }

        tim_delay_us(58);
    }
    target_enable_irq();
    return value;
}



int16_t DS18B20_read_temp_raw(void) {
    if (DS18B20_start_sensor() == 0) {
        return 0; 
    }

    // Issue "Skip ROM" + "Read Scratchpad"
    DS18B20_writeData(DS18B20_SKIP_ROM);
    DS18B20_writeData(DS18B20_READ_SCRATCHPAD);

    // Read the two temperature bytes
    uint8_t temp_lo = DS18B20_read_data(); // LSB
    uint8_t temp_hi = DS18B20_read_data(); // MSB

    // Combine into a 16-bit value (for 12 bits, lower 4 bits are fractional)
    int16_t raw_temp = (int16_t)((temp_hi << 8) | temp_lo);
    return raw_temp;
}



static const int32_t temperature_error[] = {
    ERR_TEMP(-55, 2), ERR_TEMP(-30, 1), ERR_TEMP(-10, 0.5), ERR_TEMP(85, 0.5), ERR_TEMP(100, 1),
    ERR_TEMP(125, 2), ERR_END};



static void ds18b20_init(void) {
    ctx_t *ctx = &state;
    if (ctx->inited)
        return;
    
    ctx->probePin = (int)temperature_ds18b20.instancedata; 

    ctx->inited = 1;
    ctx->temperature.min_value = SCALE_TEMP(-55);
    ctx->temperature.max_value = SCALE_TEMP(125);
}




static void ds18b20_process(void) {
    ctx_t *ctx = &state;

    if (jd_should_sample_delay(&ctx->nextsample, 0)) {
        // A conversion hasn't been started yet (in_temp == 0).
        if (!ctx->in_temp) {
            // Send DS18B20_CONVERT_T command to start measuring
            if (DS18B20_start_sensor() == 1) {
                tim_delay_us(1);
                DS18B20_writeData(DS18B20_SKIP_ROM);
                DS18B20_writeData(DS18B20_CONVERT_T);
            }

            // Mark that we�re �in conversion� and set a new nextsample
            // for when we expect conversion to be done (750-800ms).
            ctx->in_temp = 1;
            ctx->nextsample = now + CONVERSION_MS * 1000;

        }
        // A conversion has been started (in_temp == 1)
        else {
            // Clear the in_temp flag
            ctx->in_temp = 0;

            // DS18B20 has finished conversion - read the data
            int16_t raw_temp = DS18B20_read_temp_raw(); // Non-blocking
            // float fTempCelcius = ((float)raw_temp) / 16.0f;

            // Scale for fixed-point: 4 bits from DS18B20 => 10 bits 
            env_set_value(&ctx->temperature, raw_temp << 6, temperature_error);

            // Now set the next sample time to next sampling interval
            ctx->nextsample = now + SAMPLING_MS * 1000;
            ctx->inited = 2; // mark "Fully Initialized'

        }
    }
}



static void *ds18b20_temperature(void) {
    ctx_t *ctx = &state;
    if (ctx->inited < 2)
        return NULL;
    return &ctx->temperature;
}



env_sensor_api_t temperature_ds18b20 = {
    .init = ds18b20_init,
    .process = ds18b20_process,
    .get_reading = ds18b20_temperature,
};
