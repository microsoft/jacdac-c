/****************************************************************************
* Title                 :   Text to Speech HAL
* Filename              :   text_to_speech_hal.h
* Author                :   MSV
* Origin Date           :   01/02/2016
* Notes                 :   None
*****************************************************************************/
/**************************CHANGE LIST **************************************
*
*    Date    Software Version    Initials       Description
*  01/02/16       1.0.0             MSV        Module Created.
*
*****************************************************************************/
/**
 * @file text_to_speech_hal.h
 * @brief <h2> HAL layer </h2>
 *
 * @par
 * HAL layer for
 * <a href="http://www.mikroe.com">MikroElektronika's</a> TextToSpeech click
 * board.
 */
#ifndef TEXT_TO_SPEECH_HAL_H_
#define TEXT_TO_SPEECH_HAL_H_
/******************************************************************************
* Includes
*******************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
/******************************************************************************
* Preprocessor Constants
*******************************************************************************/
#define DUMMY_BYTE                                                  0x00
#define POR_TIME                                                    120
/******************************************************************************
* Configuration Constants
*******************************************************************************/

/******************************************************************************
* Macros
*******************************************************************************/
	
/******************************************************************************
* Typedefs
*******************************************************************************/

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
 * @brief <h3> CS High </h3>
 *
 * @par
 * Used by HW layer functions to set CS PIN high ( deselect )
 */
void tts_hal_cs_high( void );

/**
 * @brief <h3> CS Low </h3>
 *
 * @par
 * Used by HW layer functions to set CS PIN low ( selecet )
 */
void tts_hal_cs_low( void );

/**
 * @brief <h3> MUT High </h3>
 *
 * @par
 * Used by HW layer to set MUT PIN high ( mute )
 */
void tts_hal_mut_high( void );

/**
 * @brief <h3> MUT Low </h3>
 *
 * @par
 * Used by HW layer to set MUT PIN low ( unmute )
 */
void tts_hal_mut_low( void );

/**
 * @brief <h3> Hardware Reset </h3>
 *
 * @par
 * Resets the module via RST PIN
 */
void tts_hal_reset( void );

/**
 * @brief <h3> MSG Ready </h3>
 *
 * @par
 * Returns state of RDY PIN.
 *
 * @retval TRUE  - RDY high
 * @retval FALSE - RDY low
 */
bool tts_hal_msg_rdy( void );

/**
 * @brief <h3> HAL Initialization </h3>
 *
 * Hal layer initialization. Must be called before any other function.
 */
void tts_hal_init( void );

/**
 * @brief <h3> HAL Write </h3>
 *
 * @par
 * Writes data through SPI bus
 *
 * @note Function have no affect to the CS PIN state - chip select is
 * controled directly from HW layer.
 *
 * @param[in] buffer
 * @param[in] count
 */
void tts_hal_write( uint8_t *buffer,
                    uint16_t count );

void tts_hal_write_byte( uint8_t byte );

/**
 * @brief <h3> HAL Read </h3>
 *
 * @par
 * Reads data from SPI bus
 *
 * @note Function have no affect to the CS PIN state - chip select is
 * controled directly from HW layer
 *
 * @param[out] buffer
 * @param[in] count
 */
void tts_hal_read( uint8_t *buffer,
                   uint16_t count );

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* TEXT_TO_SPEECH_HAL_H_ */

/*** End of File **************************************************************/