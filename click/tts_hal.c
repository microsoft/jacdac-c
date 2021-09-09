/*******************************************************************************
* Title                 :   Text to Speech HAL
* Filename              :   tts_hal.c
* Author                :   MSV
* Origin Date           :   01/02/2016
* Notes                 :   None
*******************************************************************************/
/*************** MODULE REVISION LOG ******************************************
*
*    Date    Software Version    Initials       Description
*  01/02/16       1.0.0             MSV        Module Created.
*
*******************************************************************************/
/**
 * @file tts_hal.c
 * @brief <h2> HAL layer </h2>
 */
/******************************************************************************
* Includes
*******************************************************************************/
#include "tts_hal.h"
#include "board.h"
#include "drv_name.h"
#include "tinyhw.h"
#include "lib.h"

/******************************************************************************
* Module Preprocessor Constants
*******************************************************************************/

/******************************************************************************
* Module Preprocessor Macros
*******************************************************************************/

/******************************************************************************
* Module Typedefs
*******************************************************************************/

/******************************************************************************
* Module Variable Definitions
*******************************************************************************/

/******************************************************************************
* Function Prototypes
*******************************************************************************/

/******************************************************************************
* Function Definitions
*******************************************************************************/
void tts_hal_cs_high()
{
#if MIKROBUS_AVAILABLE
    pin_set(PIN_RX_CS,1);
#endif
}

void tts_hal_cs_low()
{
#if MIKROBUS_AVAILABLE
    pin_set(PIN_RX_CS,0);
#endif
}

void tts_hal_mut_high()
{
#if MIKROBUS_AVAILABLE
    pin_set(PIN_AN,1);
#endif
}

void tts_hal_mut_low()
{
#if MIKROBUS_AVAILABLE
    pin_set(PIN_AN,0);
#endif
}

void tts_hal_reset( void )
{
#if MIKROBUS_AVAILABLE
    pin_set(PIN_RST,0);
    Delay_10ms();
    pin_set(PIN_RST,1);
    Delay_ms( POR_TIME );
#endif
}

bool tts_hal_msg_rdy( void )
{
#if MIKROBUS_AVAILABLE
    return pin_get(PIN_INT);
#else
    return 0;
#endif
}

void tts_hal_init()
{
    tts_hal_reset();
    tts_hal_cs_low();
    tts_hal_mut_low();
}

void tts_hal_write( uint8_t *buffer,
                    uint16_t count )
{
    sspi_tx(buffer, count);
}

void tts_hal_read( uint8_t *buffer,
                   uint16_t count )
{
    uint8_t dummy = 0x0;
    for (int i = 0; i < count; i++) {
        sspi_tx(&dummy, 1);
        sspi_rx(&buffer[i], 1);
    }
}

/*************** END OF FUNCTIONS ***************************************************************************/