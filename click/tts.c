/****************************************************************************
* Title                 :   Text To Speech Common Requests
* Filename              :   tts.h
* Author                :   MSV
* Origin Date           :   30/01/2016
* Notes                 :   Impelentation ready functions
*****************************************************************************/
/**************************CHANGE LIST **************************************
*
*    Date    Software Version    Initials   Description
*  30/01/16     .1                  MSV     Interface Created.
*
*****************************************************************************/
/**
 * @file text_to_speech.c
 * @brief Text to speech click board higher level functions
 */
/******************************************************************************
* Includes
*******************************************************************************/
#include "tts.h"
#include "tts_hal.h"
#include "tts_image.h"
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
/* Indication Flags */
static volatile bool        _tts_rdy_f;
static volatile bool        _spc_rdy_f;
static volatile bool        _tts_fin_f;
static volatile bool        _spc_fin_f;
/* Error Buffers */
static uint16_t             _req_err;
static uint16_t             _err_code;
/* Default Configuration */
static ACONF_t              _audio_conf;
static TTSCONF_t            _tts_conf;
static PMANCONF_t           _pman_conf;

static bool                 _flush_enable;
/******************************************************************************
* Function Prototypes
*******************************************************************************/
/* Cheks for indications */
static int _parse_ind( void );
/* Message block and error callback function pointers */
static void ( *_fatal_err_callback )( uint16_t *err_code );
static void ( *_err_callback )( uint16_t *err_code );
static void ( *_msg_block_callback )( uint16_t *msg_code,
                                      uint16_t *err_code );

static char* Ltrim(char* str) {
    char* ret = str;
    while (true) {
        char c = *(ret);
        switch (c) {
            case ' ':
            case '\n':
            case '\t':
            case '\v':
            case '\r':
                ret++;
                break;
            case '\0':
            default:
                return ret;
        }
    }
    return ret; 
}

// store needs to be at least 4 chars
void ByteToStr(uint16_t byte, char* store) {
    char tmp[16];
    memset(tmp, 0, 16);
    jd_itoa(byte,tmp);

    // https://download.mikroe.com/documents/compilers/mikroc/pic/help/conversions_library.htm#bytetostr
    // The output string has fixed width of 4 characters including null character at the end (string termination).
    // The output string is right justified and remaining positions on the left (if any) are filled with blanks.
    memset(store, ' ', 3);
    store[3] = 0;
    int len = min(strlen(tmp), 3);
    char* wptr = store + 3;
    char* rptr = tmp;
    while(len--)
        *(wptr--) = *(rptr++);
}

/******************************************************************************
* Private Function Definitions
*******************************************************************************/
static int _parse_ind( void )
{
    uint16_t rsp_idx = tts_rsp_idx();

    if ( rsp_idx == ISC_MSG_BLOCKED_RESP )
    {
        uint8_t rsp_data[ 4 ] = { 0 };

        _req_err = 0;
        _err_code = 0;

        tts_rsp_data( rsp_data );

        _req_err |= rsp_data[ 0 ];
        _req_err |= rsp_data[ 1 ] << 8;
        _err_code |= rsp_data[ 2 ];
        _err_code |= rsp_data[ 3 ] << 8;

        if( _msg_block_callback != NULL )
            _msg_block_callback( &_req_err, &_err_code );

        return 1;

    } else if ( rsp_idx == ISC_ERROR_IND ) {

         uint8_t rsp_data[ 4 ] = { 0 };

        _req_err = 0;
        _err_code = 0;

        tts_rsp_data( rsp_data );

        _err_code |= rsp_data[ 0 ];
        _err_code |= rsp_data[ 1 ] << 8;

        if ( _err_code && _err_code < 0x8000 )
        {
            if( _err_callback != NULL )
                _err_callback( &_err_code );

        } else if ( _err_code && _err_code > 0x7FFF ) {

            if( _fatal_err_callback != NULL )
                _fatal_err_callback( &_err_code );
        }

        return 1;

    } else if ( rsp_idx == ISC_TTS_READY_IND ) {

        _tts_rdy_f = 1;

    } else if ( rsp_idx == ISC_SPCODEC_READY_IND ) {

        _spc_rdy_f = 1;

    } else if ( rsp_idx == ISC_TTS_FINISHED_IND ) {

        _tts_fin_f = 1;

    } else if ( rsp_idx == ISC_SPCODEC_FINISHED_IND ) {

        _spc_fin_f = 1;
    }

    return 0;
}
/******************************************************************************
* Public Function Definitions
*******************************************************************************/
void tts_init(void)
{
    _req_err = 0;
    _err_code = 0;

    _tts_rdy_f = true;
    _spc_rdy_f = true;
    _tts_fin_f = true;
    _spc_fin_f = true;

    _audio_conf.as = 0x00;
    _audio_conf.ag = 0x43;
    _audio_conf.amp = 0x00;
    _audio_conf.asr = ASR_11KHZ;
    _audio_conf.ar = 0x00;
    _audio_conf.atc = 0x00;
    _audio_conf.acs = 0x00;
    _audio_conf.dc = 0x00;

    _tts_conf.sr = 0x01;
    _tts_conf.voice = 0x00;
    _tts_conf.ep = 0x00;
    _tts_conf.lang = 0x00;
    _tts_conf.sr_wpm_lsb = 0xc8;
    _tts_conf.sr_wpm_msb = 0x00;
    _tts_conf.ds = 0x00;
    _tts_conf.res = 0x00;

    _pman_conf.am_lsb = 0x01;
    _pman_conf.am_msb = 0x00;
    _pman_conf.spi_clk = 0x01;
    _pman_conf.pad = PADDING_BYTE;

    _flush_enable = false;

    _msg_block_callback = NULL;
    _fatal_err_callback = NULL;
    _err_callback = NULL;

    tts_hw_init();
}

void tts_msg_block_callback( void( *msg_blk_ptr )( uint16_t *req_ptr,
                                                   uint16_t *err_ptr ) )
{
    _msg_block_callback = msg_blk_ptr;
}

void tts_fatal_err_callback( void( *fatal_err_ptr )( uint16_t *err_ptr ) )
{
    _fatal_err_callback = fatal_err_ptr;
}

void tts_err_callback( void( *error_ptr )( uint16_t *err_ptr ) )
{
    _err_callback = error_ptr;
}

void tts_mute(void)
{
    tts_mute_cmd( true );
}

void tts_unmute(void)
{
    tts_mute_cmd( false );
}

void tts_setup(void)
{
    tts_image_load( TTS_INIT_DATA, sizeof( TTS_INIT_DATA ) );
    tts_image_exec();
    tts_interface_test();
    tts_power_default_config();
    tts_audio_default_config();
    tts_volume_set( 0 );
    tts_default_config();
}

uint16_t tts_version_boot( VER_t *buffer )
{
    char tmp_char[ 4 ] = { 0 };
    uint8_t tmp_resp[ 16 ] = { 0 };

    Delay_ms( RESET_TO_BOOT_TIME );
    tts_parse_req( ISC_VERSION_REQ_BOOT, NULL, 0 );

    while( !tts_rsp_chk( ISC_VERSION_RESP_BOOT ) )
    {
        tts_get_resp();

        if( _parse_ind() )
            return _err_code;
    }

    tts_rsp_data( tmp_resp );
    ByteToStr( tmp_resp[ 0 ], tmp_char );
    strcpy( buffer->hwver, Ltrim( tmp_char ) );
    ByteToStr( tmp_resp[ 1 ], tmp_char );
    strcat( buffer->hwver, "." );
    strcat( buffer->hwver, Ltrim( tmp_char ) );

    return 0x0000;
}

uint16_t tts_image_load( const uint8_t *image,
                         uint16_t count )
{
    uint16_t tmp_resp = 0;
    uint16_t index = 0;
    uint8_t raw_resp[ 2 ] = { 0 };

    while ( ( count - index ) > ( BOOT_MESSAGE_MAX - 4 ) )
    {
        tts_parse_boot_img( image + index, BOOT_MESSAGE_MAX - 4 );
        Delay_ms( 10 );

        index += ( BOOT_MESSAGE_MAX - 4 );
    }

    tts_parse_boot_img( image + index, count - index );

    uint8_t retries = 100;
    while( !tts_rsp_chk( ISC_BOOT_LOAD_RESP ) )
    {
        tts_get_resp();
        retries--;

        if (retries == 0)
            target_reset();

        if( _parse_ind() )
            return _err_code;
    }

    tts_rsp_data( raw_resp );

    tmp_resp |= raw_resp[ 0 ];
    tmp_resp |= raw_resp[ 1 ] << 8;

    return ( tmp_resp == 1 ) ? 0x0000 : tmp_resp;
}

uint16_t tts_image_exec(void)
{
    uint16_t tmp_resp = 0;
    uint8_t raw_resp[ 2 ] = { 0 };

    tts_parse_req( ISC_BOOT_RUN_REQ, NULL, 0 );

    while( !tts_rsp_chk( ISC_BOOT_RUN_RESP ) )
    {
        tts_get_resp();

        if( _parse_ind() )
            return _err_code;
    }

    tts_rsp_data( raw_resp );

    tmp_resp |= raw_resp[ 0 ];
    tmp_resp |= raw_resp[ 1 ] << 8;

    return ( tmp_resp == 1 ) ? 0x0000 : tmp_resp;
}

static uint8_t _test[ 8 ] = { 0x01, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00 };
uint16_t tts_interface_test(void)
{
    uint16_t tmp_resp = 0;
    uint8_t raw_resp[ 2 ] = { 0 };

    Delay_ms( BOOT_TO_MAIN_MODE );
    tts_parse_req( ISC_TEST_REQ, _test, 8 );

    while( !tts_rsp_chk( ISC_TEST_RESP ) )
    {
        tts_get_resp();

        if( _parse_ind() )
            return _err_code;
    }

    tts_rsp_data( raw_resp );

    tmp_resp |= raw_resp[ 0 ];
    tmp_resp |= raw_resp[ 1 ] << 8;

    return tmp_resp;
}

uint16_t tts_version_main( VER_t *buffer )
{
    char tmp_char[ 4 ] = { 0 };
    uint32_t tmp_fwf = 0;
    uint32_t tmp_fwef = 0;
    uint8_t tmp_resp[ 20 ] = { 0 };

    tts_parse_req( ISC_VERSION_REQ_MAIN, NULL, 0 );

    while( !tts_rsp_chk( ISC_VERSION_RESP_MAIN ) )
    {
        tts_get_resp();

        if( _parse_ind() )
            return _err_code;
    }

    tts_rsp_data( tmp_resp );
    ByteToStr( tmp_resp[ 0 ], tmp_char );
    strcpy( buffer->hwver, Ltrim( tmp_char ) );
    strcat( buffer->hwver, "." );
    ByteToStr( tmp_resp[ 1 ], tmp_char );
    strcat( buffer->hwver, Ltrim( tmp_char ) );
    ByteToStr( tmp_resp[ 2 ], tmp_char );
    strcpy( buffer->fwver, Ltrim( tmp_char ) );
    strcat( buffer->fwver, "." );
    ByteToStr( tmp_resp[ 3 ], tmp_char );
    strcat( buffer->fwver, Ltrim( tmp_char ) );
    strcat( buffer->fwver, "." );
    ByteToStr( tmp_resp[ 12 ], tmp_char );
    strcat( buffer->fwver, Ltrim( tmp_char ) );

    tmp_fwf |= tmp_resp[ 4 ];
    tmp_fwf |= tmp_resp[ 5 ] << 8;
    tmp_fwf |= tmp_resp[ 6 ] << 16;
    tmp_fwf |= tmp_resp[ 7 ] << 24;
    buffer->fwf = tmp_fwf;
    tmp_fwef |= tmp_resp[ 8 ];
    tmp_fwef |= tmp_resp[ 9 ] << 8;
    tmp_fwef |= tmp_resp[ 10 ] << 16;
    tmp_fwef |= tmp_resp[ 11 ] << 24;
    buffer->fwef = tmp_fwef;

    return 0x0000;
}

uint16_t tts_power_default_config(void)
{
    uint16_t tmp_resp = 0;
    uint8_t raw_resp[ 2 ] = { 0 };

    tts_parse_req( ISC_PMAN_CONFIG_REQ, ( uint8_t* )&_pman_conf, 4 );

    while( !tts_rsp_chk( ISC_PMAN_CONFIG_RESP ) )
    {
        tts_get_resp();

        if( _parse_ind() )
            return _err_code;
    }

    tts_rsp_data( raw_resp );

    tmp_resp |= raw_resp[ 0 ];
    tmp_resp |= raw_resp[ 1 ] << 8;

    return tmp_resp;
}

uint16_t tts_standby_enter(void)
{
    uint16_t tmp_resp = 0;
    uint8_t raw_resp[ 2 ] = { 0 };

    tts_parse_req( ISC_PMAN_STANDBY_ENTRY_REQ, NULL, 0 );

    while( !tts_rsp_chk( ISC_PMAN_STANDBY_ENTRY_RESP ) )
    {
        tts_get_resp();

        if( _parse_ind() )
            return _err_code;
    }

    tts_rsp_data( raw_resp );

    tmp_resp |= raw_resp[ 0 ];
    tmp_resp |= raw_resp[ 1 ] << 8;

    return tmp_resp;
}

uint16_t tts_standby_exit(void)
{
    Delay_ms( STBY_MODE_ENTERY );
    tts_parse_req( ISC_PMAN_STANDBY_EXIT_IND, NULL, 0 );

    while( !tts_rsp_chk( ISC_PMAN_STANDBY_EXIT_IND ) )
    {
        tts_get_resp();

        if( _parse_ind() )
            return _err_code;
    }

    return 0x0000;
}

uint16_t tts_audio_default_config(void)
{
    uint16_t tmp_resp = 0;
    uint8_t raw_resp[ 2 ] = { 0 };

    tts_parse_req( ISC_AUDIO_CONFIG_REQ, ( uint8_t* )&_audio_conf, 8 );

    while( !tts_rsp_chk( ISC_AUDIO_CONFIG_RESP ) )
    {
        tts_get_resp();

        if( _parse_ind() )
            return _err_code;
    }

    tts_rsp_data( raw_resp );

    tmp_resp |= raw_resp[ 0 ];
    tmp_resp |= raw_resp[ 1 ] << 8;

    return tmp_resp;
}

uint16_t tts_audio_config( int8_t audio_gain,
                           ASR_t sample_rate,
                           bool dac_control )
{
    ACONF_t audio_conf;
    uint16_t tmp_resp = 0;
    uint8_t raw_resp[ 2 ] = { 0 };

    if ( audio_gain < -48 || audio_gain > 18 )
        return 0xFFFF;

    if ( sample_rate != 0 || sample_rate != 1 || sample_rate != 3 )
        return 0xFFFF;

    audio_conf.ag = ( uint8_t )audio_gain;
    audio_conf.asr = sample_rate;
    audio_conf.dc = dac_control;

    tts_parse_req( ISC_AUDIO_CONFIG_REQ, ( uint8_t* )&audio_conf, 8 );

    while( !tts_rsp_chk( ISC_AUDIO_CONFIG_RESP ) )
    {
        tts_get_resp();

        if( _parse_ind() )
            return _err_code;
    }

    tts_rsp_data( raw_resp );

    tmp_resp |= raw_resp[ 0 ];
    tmp_resp |= raw_resp[ 1 ] << 8;

    return tmp_resp;
}

uint16_t tts_volume_set( int16_t gain )
{
    uint16_t tmp_resp = 0;
    uint8_t raw_resp[ 2 ] = { 0 };
    uint8_t tmp_gain[ 2 ] = { 0 };

    tmp_gain[ 0 ] = gain & 0x00FF;
    tmp_gain[ 1 ] = ( gain & 0xFF00 ) >> 8;

    tts_parse_req( ISC_AUDIO_VOULME_REQ, tmp_gain, 2 );

    while( !tts_rsp_chk( ISC_AUDIO_VOLUME_RESP ) )
    {
        tts_get_resp();

        if( _parse_ind() )
            return _err_code;
    }

    tts_rsp_data( raw_resp );

    tmp_resp |= raw_resp[ 0 ];
    tmp_resp |= raw_resp[ 1 ] << 8;

    return tmp_resp;
}

uint16_t tts_audio_mute(void)
{
    uint16_t tmp_resp = 0;
    uint8_t raw_resp[ 2 ] = { 0 };
    uint8_t tmp_mute[ 2 ] = { 1, 0 };

    tts_parse_req( ISC_AUDIO_MUTE_REQ, tmp_mute, 2 );

    while( !tts_rsp_chk( ISC_AUDIO_MUTE_RESP ) )
    {
        tts_get_resp();

        if( _parse_ind() )
            return _err_code;
    }

    tts_rsp_data( raw_resp );

    tmp_resp |= raw_resp[ 0 ];
    tmp_resp |= raw_resp[ 1 ] << 8;

    return tmp_resp;
}

uint16_t tts_audio_unmute(void)
{
    uint16_t tmp_resp = 0;
    uint8_t raw_resp[ 2 ] = { 0 };
    uint8_t tmp_mute[ 2 ] = { 0, 0 };

    tts_parse_req( ISC_AUDIO_MUTE_REQ, tmp_mute, 2 );

    while( !tts_rsp_chk( ISC_AUDIO_MUTE_RESP ) )
    {
        tts_get_resp();

        if( _parse_ind() )
            return _err_code;
    }

    tts_rsp_data( raw_resp );

    tmp_resp |= raw_resp[ 0 ];
    tmp_resp |= raw_resp[ 1 ] << 8;

    return tmp_resp;
}

uint16_t tts_default_config(void)
{
    uint16_t tmp_resp = 0;
    uint8_t raw_resp[ 2 ] = { 0 };

    tts_parse_req( ISC_TTS_CONFIG_REQ, ( uint8_t* )&_tts_conf, 8 );

    while( !tts_rsp_chk( ISC_TTS_CONFIG_RESP ) )
    {
        tts_get_resp();

        if( _parse_ind() )
            return _err_code;
    }

    tts_rsp_data( raw_resp );

    tmp_resp |= raw_resp[ 0 ];
    tmp_resp |= raw_resp[ 1 ] << 8;

    return tmp_resp;
}

uint16_t tts_config( uint8_t voice_type,
                     bool epson_parse,
                     TTSV_t language,
                     uint16_t speaking_rate )
{
    TTSCONF_t tts_conf;
    uint16_t tmp_resp = 0;
    uint8_t raw_resp[ 2 ] = { 0 };

    if ( voice_type > 8 )
        return 0xFFFF;

    if ( language > 4 )
        return 0xFFFF;

    if ( speaking_rate < 0x004B || speaking_rate > 0x0258 )
        return 0xFFFF;

    tts_conf.voice = voice_type;
    tts_conf.ep = epson_parse;
    tts_conf.lang = language;
    tts_conf.sr_wpm_lsb = ( speaking_rate & 0x00FF );
    tts_conf.sr_wpm_msb = ( speaking_rate & 0xFF00 ) >> 8;

    tts_parse_req( ISC_TTS_CONFIG_REQ, ( uint8_t* )&tts_conf, 8 );

    while( !tts_rsp_chk( ISC_TTS_CONFIG_RESP ) )
    {
        tts_get_resp();

        if( _parse_ind() )
            return _err_code;
    }

    tts_rsp_data( raw_resp );

    tmp_resp |= raw_resp[ 0 ];
    tmp_resp |= raw_resp[ 1 ] << 8;

    return tmp_resp;
}

uint16_t tts_speak( char *word )
{
    bool tmp_f              = false;
    char *wptr              = word;
    uint8_t raw_resp[ 2 ]   = { 0 };
    uint16_t tmp_resp       = 0;
    uint32_t wlen           = strlen( wptr );

    tts_parse_speak_req( ISC_TTS_SPEAK_REQ, _flush_enable, wptr, wlen );

    _tts_rdy_f = 0;
    _tts_fin_f = 0;

    while( !( tmp_f && _tts_rdy_f ) )
    {
        tts_get_resp();

        if( _parse_ind() )
            return _err_code;

        if( tts_rsp_chk( ISC_TTS_SPEAK_RESP ) )
        {
            tts_rsp_data( raw_resp );

            tmp_resp |= raw_resp[ 0 ];
            tmp_resp |= raw_resp[ 1 ] << 8;
            tmp_f = true;
        }
    }

    return tmp_resp;
}

uint16_t tts_pause( void )
{
    uint16_t tmp_resp = 0;
    uint8_t raw_resp[ 2 ] = { 0 };
    uint8_t tmp_pause[ 2 ] = { 1, 0 };

    tts_parse_req( ISC_TTS_PAUSE_REQ, tmp_pause, 2 );

    while( !tts_rsp_chk( ISC_TTS_PAUSE_RESP ) )
    {
        tts_get_resp();

        if( _parse_ind() )
            return _err_code;
    }

    tts_rsp_data( raw_resp );

    tmp_resp |= raw_resp[ 0 ];
    tmp_resp |= raw_resp[ 1 ] << 8;

    return tmp_resp;
}

uint16_t tts_unpause( void )
{
    uint16_t tmp_resp = 0;
    uint8_t raw_resp[ 2 ] = { 0 };
    uint8_t tmp_pause[ 2 ] = { 0 };

    tts_parse_req( ISC_TTS_PAUSE_REQ, tmp_pause, 2 );

    while( !tts_rsp_chk( ISC_TTS_PAUSE_RESP ) )
    {
        tts_get_resp();

        if( _parse_ind() )
            return _err_code;
    }

    tts_rsp_data( raw_resp );

    tmp_resp |= raw_resp[ 0 ];
    tmp_resp |= raw_resp[ 1 ] << 8;

    return tmp_resp;
}

uint16_t tts_stop( bool reset )
{
    uint16_t tmp_resp = 0;
    uint8_t raw_resp[ 2 ] = { 0 };
    uint8_t tmp_reset[ 2 ] = { 0 };

    if( reset )
        tmp_reset[ 0 ] = 0x01;

    tts_parse_req( ISC_TTS_STOP_REQ, tmp_reset, 2 );

    while( !tts_rsp_chk( ISC_TTS_STOP_RESP ) )
    {
        tts_get_resp();

        if( _parse_ind() )
            return _err_code;
    }

    tts_rsp_data( raw_resp );

    tmp_resp |= raw_resp[ 0 ];
    tmp_resp |= raw_resp[ 1 ] << 8;

    return tmp_resp;
}

uint16_t tts_user_dict( bool erase,
                        uint8_t *udict_data,
                        uint16_t count )
{
    uint16_t cnt = 2;
    uint16_t tmp_rsp = 0;
    uint8_t rsp_data[ 2 ] = { 0 };
    uint8_t tmp_data[ BOOT_MESSAGE_MAX ] = { 0 };

    if ( erase )
        tmp_data[ 0 ] = 1;

    while ( count-- )
        tmp_data[ cnt ++ ] = *( udict_data++ );

    tts_parse_req( ISC_TTS_UDICT_DATA_REQ, tmp_data, count + 2 );

    while( !tts_rsp_chk( ISC_TTS_UDICT_DATA_RESP ) )
    {
        tts_get_resp();

        if( _parse_ind() )
            return _err_code;
    }

    tts_rsp_data( rsp_data );

    tmp_rsp |= rsp_data[ 0 ];
    tmp_rsp |= rsp_data[ 1 ] << 8;

    return tmp_rsp;
}

uint16_t tts_codec_configure(void)
{
    uint16_t tmp_resp = 0;
    uint8_t raw_resp[ 2 ] = { 0 };
    uint8_t tmp_codec[ 32 ] = { 0 };

    tmp_codec[ 0 ] = 0x01;
    tmp_codec[ 1 ] = 0x01;
    tmp_codec[ 24 ] = 0x02;

    tts_parse_req( ISC_SPCODEC_CONFIG_REQ , tmp_codec, 32 );

    while( !tts_rsp_chk( ISC_SPCODEC_CONFIG_RESP ) )
    {
        tts_get_resp();

        if( _parse_ind() )
            return _err_code;
    }

    tts_rsp_data( raw_resp );

    tmp_resp |= raw_resp[ 0 ];
    tmp_resp |= raw_resp[ 1 ] << 8;

    return tmp_resp;
}

uint16_t tts_codec_start( uint8_t *codec_data,
                          uint16_t count )
{
    bool tmp_f = false;
    uint16_t tmp_resp = 0;
    uint8_t raw_resp[ 2 ] = { 0 };

    if ( count != 512 || count != 1024 || count != 2048 )
        return 0xFFFF;

    while( !_spc_fin_f )
    {
        tts_get_resp();

        if( _parse_ind() )
            return _err_code;
    }

    tts_parse_req( ISC_SPCODEC_START_REQ , codec_data, count );

    _spc_rdy_f = 0;
    _spc_fin_f = 0;

    while( !( tmp_f && _spc_rdy_f ) )
    {
        tts_get_resp();

        if( _parse_ind() )
            return _err_code;

        if( tts_rsp_chk( ISC_TTS_SPEAK_RESP ) )
        {
            tts_rsp_data( raw_resp );

            tmp_resp |= raw_resp[ 0 ];
            tmp_resp |= raw_resp[ 1 ] << 8;
            tmp_f = true;
        }
    }

    return tmp_resp;
}

uint16_t tts_codec_pause(void)
{
    uint16_t tmp_resp = 0;
    uint8_t raw_resp[ 2 ] = { 0 };
    uint8_t tmp_data[ 2 ] = { 1, 0 };

    tts_parse_req( ISC_SPCODEC_PAUSE_REQ , tmp_data, 2 );

    while( !tts_rsp_chk( ISC_SPCODEC_PAUSE_RESP ) )
    {
        tts_get_resp();

        if( _parse_ind() )
            return _err_code;
    }

    tts_rsp_data( raw_resp );

    tmp_resp |= raw_resp[ 0 ];
    tmp_resp |= raw_resp[ 1 ] << 8;

    return tmp_resp;
}

uint16_t tts_codec_unpause(void)
{
    uint16_t tmp_resp = 0;
    uint8_t raw_resp[ 2 ] = { 0 };
    uint8_t tmp_data[ 2 ] = { 0 };

    tts_parse_req( ISC_SPCODEC_PAUSE_REQ , tmp_data, 2 );

    while( !tts_rsp_chk( ISC_SPCODEC_PAUSE_RESP ) )
    {
        tts_get_resp();

        if( _parse_ind() )
            return _err_code;
    }

    tts_rsp_data( raw_resp );

    tmp_resp |= raw_resp[ 0 ];
    tmp_resp |= raw_resp[ 1 ] << 8;

    return tmp_resp;
}

uint16_t tts_codec_stop( bool reset )
{
    uint16_t tmp_resp = 0;
    uint8_t raw_resp[ 2 ] = { 0 };
    uint8_t tmp_data[ 2 ] = { 0 };

    if( reset )
        tmp_data[ 0 ] = 1;

    tts_parse_req( ISC_SPCODEC_STOP_REQ, tmp_data, 2 );

    while( !tts_rsp_chk( ISC_SPCODEC_PAUSE_RESP ) )
    {
        tts_get_resp();

        if( _parse_ind() )
            return _err_code;
    }

    tts_rsp_data( raw_resp );

    tmp_resp |= raw_resp[ 0 ];
    tmp_resp |= raw_resp[ 1 ] << 8;

    return tmp_resp;
}
/*************** END OF FUNCTIONS *********************************************/