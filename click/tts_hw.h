/****************************************************************************
* Title                 :   Text to Speech Click
* Filename              :   text_to_speech_hw.h
* Author                :   MSV
* Origin Date           :   30/01/2016
* Notes                 :   None
*****************************************************************************/
/**************************CHANGE LIST **************************************
*
*    Date    Software Version    Initials   Description 
*  30/01/16    XXXXXXXXXXX         MSV      Interface Created.
*
*****************************************************************************/
/**
 * @file text_to_speech_hw.h
 * @brief <h3>Hardware Layer</h3>
 *
 * @par
 * Low level functions for
 * <a href="http://www.mikroe.com">MikroElektronika's</a> TextToSpeech click
 * board.
 */

/**
 * @page LIB_INFO Library Info
 * @date 28 Jan 2016
 * @author Milos Vidojevic
 * @copyright GNU Public License
 * @version 1.0.0 - Initial testing and verification
 */

/**
 * @page TEST_CFG Test Configurations
 *
 * ### Test configuration 1 : ###
 * @par
 * - <b>MCU</b> :             STM32F107VC
 * - <b>Dev.Board</b> :       EasyMx Pro v7
 * - <b>Oscillator</b> :      72 Mhz internal
 * - <b>Ext. Modules</b> :    TextToSpeech Click
 * - <b>SW</b> :              MikroC PRO for ARM 4.7.1
 *
 * ### Test configuration 2 : ###
 * @par
 * - <b>MCU</b> :             PIC32MX795F512L
 * - <b>Dev. Board</b> :      EasyPIC Fusion v7
 * - <b>Oscillator</b> :      80 Mhz internal
 * - <b>Ext. Modules</b> :    TextToSpeech Click
 * - <b>SW</b> :              MikroC PRO for PIC32 3.6.0
 *
 * ### Test configuration 3 : ###
 * @par
 * - <b>MCU</b> :             FT900Q
 * - <b>Dev. Board</b> :      EasyFT90x v7
 * - <b>Oscillator</b> :      100 Mhz internal
 * - <b>Ext. Modules</b> :    TextToSpeech Click
 * - <b>SW</b> :              MikroC PRO for FT90x 1.2.1
 *
 */

/**
 * @page SCH Schematic
 * ![Schematic](../tts_sch.png)
 */

/**
 * @page PIN Pin Usage
 * ![Pins](../tts_pins.png)
 */

/**
 * @mainpage
 * ### General Desription ###
 *
 * @par
 * The S1V30120 is a Speech Synthesis IC that provides a cost effective
 * solution for adding Text-To-Speech ( TTS ) and ADPCM speech processing
 * applications to a range of portable devices. The highly integrated design
 * reduces overall system cost and time-to-market. The S1V30120 contains all
 * the required analogue codecs, memory, and EPSON - supplied embedded
 * algorithms. All applications are controlled over a single serial interface
 * ( SPI ) allowing control from a wide range of hosts and rapid integration
 * into existing products.
 *
 * ### Features ###
 *
 * @par
 * - Text To Speech Synthesis (TTS)
 *      + Fonix DECtalk v5, fully parametric speech synthesis
 *      + Languages: US English, Castilian Spanish, Latin American Spanish
 *      + Nine pre-defined voices
 *      + Sampling rate: 11.025kHz
 *
 * - Audio reproduction (ADPCM)
 *      + ADPCM decoding (in Epson’s original format)
 *      + Bit rate: 80kbps, 64kbps, 48kbps, 40kbps, 32kbps and 24kbps
 *      + Sampling rate: 16, 8 kHz
 *
 * - Host interface
 *      + Synchronous serial interface (SPI interface is supported)
 *      + Command control
 *
 * - 16-bit full-digital amplifier
 *      + Sampling rate (fs): 16, 11.025 and 8 kHz
 *      + Digital Input: 16 bits
 *      + Operating voltage: 3.3/1.8V
 *
 * - Clock
 *      + 32.768KHz
 *
 * - Package
 *      + 64-pin TQFP (10mm x 10mm) with 0.5mm-pitch pins
 *
 * - Supply voltage
 *      + 3.3V (I/O power supply)
 *      + 1.8V (Core power supply)
 */
#ifndef TEXT_TO_SPEECH_HW_H_
#define TEXT_TO_SPEECH_HW_H_
/******************************************************************************
* Includes
*******************************************************************************/
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
/******************************************************************************
* Preprocessor Constants
*******************************************************************************/
/**
 * Time needed for module to gets into the boot mode after reset. */
#define RESET_TO_BOOT_TIME                                      150
/**
 * Time needed for module to gets from boot mode into main work mode. */
#define BOOT_TO_MAIN_MODE                                       150
/**
 * Time needed for module to enter the standby mode */
#define STBY_MODE_ENTERY                                         45
/**
 * Max response time in ms - if there is no response after this amount
 * of time user should restart the device */
#define MAX_RESPONSE_TIME                                       500
/**
 * Maximum message size in main mode. Recomended size is 2116 but
 * if needed it can be resized to save the RAM space. Note that
 * MAIN_MESSAGE_MAX must be greater than BOOT_MESSAGE_MAX. This number decreased
 * by 7 also represents the maximum size of the string that can be sent in one 
 * command. */
#define MAIN_MESSAGE_MAX                                        1116
/**
 * Maximum message size in boot mode. Recomended size is 2048 but
 * if needed it can be resized to save RAM space. Note that BOOT_MESSAGE_MAX
 * must be smaller than MAIN_MESSAGE_MAX */
#define BOOT_MESSAGE_MAX                                        1048
/**
 * Maximum response message max */
#define RESP_MESSAGE_MAX                                        24
/**
 * Mark for the start of message */
#define START_MESSAGE                                           0xAA
/**
 * Pading data used to flash the channels */
#define PADDING_BYTE                                            0x00
/******************************************************************************
* Configuration Constants
*******************************************************************************/

/******************************************************************************
* Macros
*******************************************************************************/

/******************************************************************************
* Typedefs
*******************************************************************************/
/**
 * @struct ISC_REQ_t
 * @brief <h3> Request Message Structure </h3>
 *
 * @par
 * All messages that are received and sent by the device are termed ISC
 * ( Inter-System-Communication ) messages. An ISC message consists of a
 * fixed length header part and a variable length payload.
 */
typedef struct __attribute__((packed))
{
    /**
     * Header Part - Message length ( including header )
     * len[ 0 ] - LSB
     * len[ 1 ] - MSB */
    uint8_t     len[ 2 ];
    /**
     * Header Part - Message code ( index )
     * idx[ 0 ] - LSB
     * idx[ 1 ] - MSB */
    uint8_t     idx[ 2 ];
    /**
     * Payload Part - Message data */
    uint8_t     payload[ MAIN_MESSAGE_MAX ];

}ISC_REQ_t;

/**
 * @struct ISC_RESP_t
 * @brief <h3> Response Message Structure </h3>
 *
 * @par
 * All messages that are received and sent by the device are termed ISC
 * ( Inter-System-Communication ) messages. An ISC message consists of a
 * fixed length header part and a variable length payload.
 */
typedef struct __attribute__((packed))
{
    /**
     * Header Part - Message length ( including header )
     * len[ 0 ] - LSB
     * len[ 1 ] - MSB */
    uint8_t     len[ 2 ];
    /**
     * Header Part - Message code ( index )
     * idx[ 0 ] - LSB
     * idx[ 1 ] - MSB */
    uint8_t     idx[ 2 ];
    /**
     * Payload Part - Message data */
    uint8_t     payload[ RESP_MESSAGE_MAX ];

}ISC_RESP_t;

/******************************************************************************
* Variables
*******************************************************************************/

/******************************************************************************
* Function Prototypes
*******************************************************************************/
#ifdef __cplusplus
extern "C"{
#endif

/**
 * @brief <h3> HW Layer Initialization </h3>
 *
 * @par
 * Initalization of HW Layer, must be done before any other function.
 * This function also executes HAL layer initialization inside it self.
 */
void tts_hw_init( void );

/**
 * @brief <h3> Timer Tick </h3>
 *
 * @par
 * Should be placed inside the user defined interrupt routine that will call
 * this function every milisecond.
 */
void tts_tick_isr( void );

/**
 * @brief <h3> Mute Command </h3>
 *
 * @par
 * Mute using MUT pin
 *
 * @param[in] cmd ( true - mute / false - unmute )
 */
void tts_mute_cmd( bool cmd );

/**
 * @brief <h3> Request Parser </h3>
 *
 * @par
 * Preparing request for writing. Loads data into output buffer and executes
 * write function.
 *
 * @param[in] req - valid request code
 * @param[in] payload - data
 * @param[in] pl_len - data size
 */
void tts_parse_req( uint16_t req,
                    uint8_t *payload,
                    uint16_t pl_len );

/**
 * @brief <h3> Boot Image Request Parser </h3>
 *
 * @par
 * Function is used for the boot uploading of boot image
 *
 * @param[in] payload - boot image part
 * @param[in] pl_len - data size
 */
void tts_parse_boot_img( const uint8_t *payload,
                         uint16_t pl_len );

/**
 * @brief <h3> Speak Request Parser </h3>
 *
 * @par
 * Preparing speak request for writing. Provided word must be valid
 * null terminated string. Size of word must be shorter than
 * 2115 characters including null terminator.
 *
 * @param[in] req - speak request code
 * @param[in] flush_en - flash option
 * @param[in] word - char array
 * @param[in] word_len - char array length
 */
void tts_parse_speak_req( uint16_t req,
                          uint8_t flush_en,
                          char *word,
                          uint16_t word_len );

/**
 * @brief <h3> Response </h3>
 *
 * @par
 * Cheks and pads SPI for response. If there is a response than this
 * function calls read response functions and fills the input buffer
 * with received data.
 *
 */
void tts_get_resp( void );

/**
 * @brief <h3> Check Response Index </h3>
 *
 * @par
 * Compares received response code with code provided as argument. Returns
 * true if codes have same value, otherwise returns false.
 *
 * @param[in] idx - response code for comparing
 *
 * @retval TRUE - codes are same
 * @retval FLASE - codes are different
 */
bool tts_rsp_chk( uint16_t idx );

/**
 * @brief <h3> Response Index </h3>
 *
 * @par
 * Returns last received response code.
 *
 * @return ( 0x0000 ~ 0xFFFF )
 */
uint16_t tts_rsp_idx( void );

/**
 * @brief <h3> Response Data </h3>
 *
 * @par
 * Fill given buffer with last received response data if there is data at all.
 * Size of buffer must be big enough.
 *
 * @param[out] buffer
 */
void tts_rsp_data( uint8_t *buffer );

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* TEXT_TO_SPEECH_HW_H_ */

/*** End of File **************************************************************/