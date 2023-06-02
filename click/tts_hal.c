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
void tts_hal_cs_high(void)
{
    pin_set(MIKROBUS_CS,1);
}

void tts_hal_cs_low(void)
{
    pin_set(MIKROBUS_CS,0);
}

void tts_hal_mut_high(void)
{
    pin_set(MIKROBUS_AN,1);
}

void tts_hal_mut_low(void)
{
    pin_set(MIKROBUS_AN,0);
}

void tts_hal_reset( void )
{
    pin_set(MIKROBUS_RST,0);
    Delay_10ms();
    pin_set(MIKROBUS_RST,1);
    Delay_ms( POR_TIME );
}

bool tts_hal_msg_rdy( void )
{
    return pin_get(MIKROBUS_INT);
}

void tts_hal_init(void)
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
    sspi_rx(buffer, count);
}

/*************** END OF FUNCTIONS ***************************************************************************/