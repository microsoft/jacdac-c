/****************************************************************************
* Title                 :   Text To Speech Click
* Filename              :   tts_hw.h
* Author                :   MSV
* Origin Date           :   30/01/2016
* Notes                 :   Hardware layer
*****************************************************************************/
/**************************CHANGE LIST **************************************
*
*    Date    Software Version    Initials   Description
*  30/01/16     .1                  MSV     Interface Created.
*
*****************************************************************************/
/**
 * @file XXX.c
 * @brief This module contains the low level function for MikroElektronika's
 * Text to Speech click board.
 */
/******************************************************************************
* Includes
*******************************************************************************/
#include "tts_hw.h"
#include "tts_hal.h"
#include "drv_digital_out.h"
#include "drv_digital_in.h"
#include "drv_i2c_master.h"
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
/* Timer flag and counter */
static volatile bool        _ticker_f;
static volatile uint16_t    _ticker;
/* Input and output buffers */
static ISC_REQ_t   _last_req;
static ISC_RESP_t  _last_rsp;
/******************************************************************************
* Private Function Definitions
*******************************************************************************/
void _read_rsp(void)
{
    uint8_t tmp_byte = 0;
    uint16_t tmp_len = 0;

    tts_hal_cs_low();
    tts_hal_read( &tmp_byte, 1 );

    if( tmp_byte == START_MESSAGE )
    {
        tts_hal_read( ( uint8_t* )&_last_rsp, 2 );

        tmp_len |= _last_rsp.len[ 0 ];
        tmp_len |= _last_rsp.len[ 1 ] << 8;

        tts_hal_read( ( uint8_t* )&_last_rsp + 2, tmp_len );

    } else {

        Delay_ms( 5 );
    }

    tts_hal_cs_high();
}

void _write_req(void)
{
    uint16_t cnt = 0;
    uint8_t start = START_MESSAGE;

    cnt |= _last_req.len[ 0 ];
    cnt |= _last_req.len[ 1 ] << 8;

    tts_hal_cs_low();
    tts_hal_write( &start, 1 );
    tts_hal_write( ( uint8_t* )&_last_req, cnt );
    tts_hal_cs_high();
}
/******************************************************************************
* Public Function Definitions
*******************************************************************************/
void tts_hw_init( void )
{
    tts_hal_init();

    _ticker_f = false;
    _ticker = 0;

    _last_req.idx[ 0 ] = 0;
    _last_req.idx[ 1 ] = 0;
    _last_req.len[ 0 ] = 0;
    _last_req.len[ 1 ] = 0;
    memset( _last_req.payload, 0, MAIN_MESSAGE_MAX );

    _last_rsp.idx[ 0 ] = 255;
    _last_rsp.idx[ 1 ] = 255;
    _last_rsp.len[ 0 ] = 0;
    _last_rsp.len[ 1 ] = 0;
    memset( _last_rsp.payload, 0, RESP_MESSAGE_MAX );
}

void tts_tick_isr(void)
{
    _ticker++;

    if( _ticker > 500 )
        _ticker_f = true;
}

void tts_mute_cmd( bool cmd )
{
    if( cmd )
        tts_hal_mut_high();
    else
        tts_hal_mut_low();
}

void tts_parse_req( uint16_t req,
                    uint8_t *payload,
                    uint16_t pl_len )
{
    uint8_t *pl = payload;
    uint16_t i = 0;
    uint16_t tmp = pl_len + 4;

    _last_req.len[ 0 ] = tmp & 0x00FF;
    _last_req.len[ 1 ] = ( tmp & 0xFF00 ) >> 8;
    _last_req.idx[ 0 ] = req & 0x00FF;
    _last_req.idx[ 1 ] = ( req & 0xFF00 ) >> 8;
    _last_rsp.idx[ 0 ] = 0xFF;
    _last_rsp.idx[ 1 ] = 0xFF;

    if ( pl != NULL )
    {
        while ( pl_len-- )
            _last_req.payload[ i++ ] = *( pl++ );
    }

    _write_req();
}

void tts_parse_boot_img( const uint8_t *payload,
                         uint16_t pl_len )
{
    uint16_t i = 0;
    uint16_t tmp = pl_len + 4;

    _last_req.len[ 0 ] = tmp & 0x00FF;
    _last_req.len[ 1 ] = ( tmp & 0xFF00 ) >> 8;
    _last_req.idx[ 0 ] = 0x00;
    _last_req.idx[ 1 ] = 0x10;
    _last_rsp.idx[ 0 ] = 0xFF;
    _last_rsp.idx[ 1 ] = 0xFF;

    if ( payload != NULL )
    {
        while ( pl_len-- ) {
            _last_req.payload[ i ] = payload[ i ];
            i++;
        }
    }

    _write_req();
}

void tts_parse_speak_req( uint16_t req,
                          uint8_t flush_en,
                          char *word,
                          uint16_t word_len )
{
    char *ptr = word;
    uint16_t i = 1;
    uint16_t tmp = word_len;

    word_len += 7;

    _last_req.len[ 0 ] = word_len & 0x00FF;
    _last_req.len[ 1 ] = ( word_len & 0xFF00 ) >> 8;
    _last_req.idx[ 0 ] = req & 0x00FF;
    _last_req.idx[ 1 ] = ( req & 0xFF00 ) >> 8;
    _last_rsp.idx[ 0 ] = 0xFF;
    _last_rsp.idx[ 1 ] = 0xFF;

    if( flush_en )
    {
        _last_req.payload[ 0 ] = 1;

    } else {

        _last_req.payload[ 0 ] = 0;
    }

    while( tmp-- )
        _last_req.payload[ i++ ] = *( ptr++ );

    _last_req.payload[ i++ ] = 0x20;
    _last_req.payload[ i ] = 0x00;

    _write_req();
}

void tts_get_resp(void)
{
    if( tts_hal_msg_rdy() )
    {
        _read_rsp();
    }
}

bool tts_rsp_chk( uint16_t idx )
{
    uint16_t tmp = 0;

    tmp |= _last_rsp.idx[ 0 ];
    tmp |= _last_rsp.idx[ 1 ] << 8;

    return ( idx == tmp ) ? true : false;
}

uint16_t tts_rsp_idx(void)
{
    uint16_t tmp = 0;

    tmp |= _last_rsp.idx[ 0 ];
    tmp |= _last_rsp.idx[ 1 ] << 8;

    return tmp;
}

void tts_rsp_data( uint8_t *buffer )
{
    uint8_t *bfr = buffer;
    uint16_t cnt = 0;
    uint8_t *ptr = _last_rsp.payload;

    cnt |= _last_rsp.len[ 0 ];
    cnt |= _last_rsp.len[ 1 ] << 8;
    cnt -= 4;

    while( cnt-- )
        *( bfr++ ) = *( ptr++ );
}

/*************** END OF FUNCTIONS *********************************************/