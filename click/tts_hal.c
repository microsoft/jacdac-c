/*******************************************************************************
* Title                 :   Text to Speech HAL
* Filename              :   text_to_speech_hal.c
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
 * @file text_to_speech_hal.c
 * @brief <h2> HAL layer </h2>
 */
/******************************************************************************
* Includes
*******************************************************************************/
#include "text_to_speech_hal.h"

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
#if defined( __MIKROC_PRO_FOR_ARM__ )   || \
    defined( __MIKROC_PRO_FOR_DSPIC__ )
static void         ( *write_spi_p )            ( unsigned int data_out );
static unsigned int ( *read_spi_p )             ( unsigned int buffer );

#elif defined( __MIKROC_PRO_FOR_AVR__ )     || \
      defined( __MIKROC_PRO_FOR_PIC__ )     || \
      defined( __MIKROC_PRO_FOR_8051__ )
static void         ( *write_spi_p )            ( unsigned char data_out );
static unsigned char( *read_spi_p )             ( unsigned char dummy );

#elif defined( __MIKROC_PRO_FOR_PIC32__ )
static void         ( *write_spi_p )            ( unsigned long data_out );
static unsigned long( *read_spi_p )             ( unsigned long buffer );

#elif defined( __MIKROC_PRO_FOR_FT90x__ )
static void         ( *write_spi_p )            ( unsigned char data_out );
static unsigned char( *read_spi_p )             ( unsigned char dummy );
static void         ( *write_bytes_spi_p )      ( unsigned char *data_out,
                                                  unsigned int count );
static void         ( *read_bytes_spi_p )       ( unsigned char *buffer,
                                                  unsigned int count );
#endif

#if defined( __MIKROC_PRO_FOR_ARM__ )     || \
    defined( __MIKROC_PRO_FOR_AVR__ )     || \
    defined( __MIKROC_PRO_FOR_PIC__ )     || \
    defined( __MIKROC_PRO_FOR_PIC32__ )   || \
    defined( __MIKROC_PRO_FOR_DSPIC__ )   || \
    defined( __MIKROC_PRO_FOR_8051__ )    || \
    defined( __MIKROC_PRO_FOR_FT90x__ )
extern sfr sbit TTS_CS;
extern sfr sbit TTS_RST;
extern sfr sbit TTS_RDY;
extern sfr sbit TTS_MUTE;
#endif

/******************************************************************************
* Function Prototypes
*******************************************************************************/

/******************************************************************************
* Function Definitions
*******************************************************************************/
void tts_hal_cs_high()
{
#if defined( __GNUC__ )

#else
    TTS_CS = 1;
#endif
}

void tts_hal_cs_low()
{
#if defined( __GNUC__ )

#else
    TTS_CS = 0;
#endif
}

void tts_hal_mut_high()
{
#if defined( __GNUC__ )

#else
    TTS_MUTE = 1;
#endif
}

void tts_hal_mut_low()
{
#if defined( __GNUC__ )

#else
    TTS_MUTE = 0;
#endif
}

void tts_hal_reset( void )
{
#if defined( __GNUC__ )

#else
    TTS_RST = 0;
    Delay_10ms();
    TTS_RST = 1;
    Delay_ms( POR_TIME );
#endif
}

bool tts_hal_msg_rdy( void )
{
#if defined( __GNUC__ )

#else
    return TTS_RDY;
#endif
}

void tts_hal_init()
{
#if defined( __MIKROC_PRO_FOR_ARM__ )   || \
    defined( __MIKROC_PRO_FOR_AVR__ )   || \
    defined( __MIKROC_PRO_FOR_DSPIC__ ) || \
    defined( __MIKROC_PRO_FOR_PIC32__ ) || \
    defined( __MIKROC_PRO_FOR_8051__ )
    write_spi_p             = SPI_Wr_Ptr;
    read_spi_p              = SPI_Rd_Ptr;

#elif defined( __MIKROC_PRO_FOR_PIC__ )
    write_spi_p             = SPI1_Write;
    read_spi_p              = SPI1_Read;

#elif defined( __MIKROC_PRO_FOR_FT90x__ )
    write_spi_p             = SPIM_Wr_Ptr;
    read_spi_p              = SPIM_Rd_Ptr;
    write_bytes_spi_p       = SPIM_WrBytes_Ptr;
    read_bytes_spi_p        = SPIM_RdBytes_Ptr;
#endif
    tts_hal_reset();
    tts_hal_cs_low();
    tts_hal_mut_low();
}

void tts_hal_write( uint8_t *buffer,
                    uint16_t count )
{
    while( count-- )
        write_spi_p( *( buffer++ ) );
}

void tts_hal_read( uint8_t *buffer,
                   uint16_t count )
{
    while( count-- )
        *( buffer++ ) = read_spi_p( DUMMY_BYTE );
}

/*************** END OF FUNCTIONS ***************************************************************************/