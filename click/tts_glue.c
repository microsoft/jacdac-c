#include "color.h"
#include "interfaces/jd_sensor_api.h"
#include "tts.h"
#include "jd_services.h"

#ifdef MIKROBUS_AVAILABLE

void fatal_error( uint16_t *err ) {
    hw_panic();
}

void msg_blocked( uint16_t *req, uint16_t *err ) {
    DMESG("MSG BLOCKED %d ERR %d", *req, *err);
}

void text_to_speech_volume(uint32_t volume) {
    // todo scale to decibels gain
}

bool text_to_speech_language(char* langId) {
    // convert from string to compatible lang id
    return false;
}

void text_to_speech_rate(uint32_t rate) {
    // scale to corresponding api
}

void text_to_speech_resume(void) {
    tts_unpause();
}

void text_to_speech_pause(void) {
    tts_pause();
}

void text_to_speech_cancel(void) {
    tts_stop(false);
}

void text_to_speech_speak(char *phrase) {
    tts_speak(phrase);
}

void text_to_speech_init(void) {
    // chip select
    pin_setup_output(PIN_RX_CS);
    // mute
    pin_setup_output(PIN_AN);
    // reset
    pin_setup_output(PIN_RST);
    // data ready
    pin_setup_input(PIN_INT, 0);

    tts_init();
    tts_setup();
    
    tts_msg_block_callback(msg_blocked);
    tts_fatal_err_callback(fatal_error);
    
    tts_config( 0x01, false, TTSV_US, 0x0100 );
}

const speech_synth_api_t tts_click = {
    .init = text_to_speech_init,
    .speak = text_to_speech_speak,
    .set_volume=text_to_speech_volume,
    .set_rate=text_to_speech_rate,
    .set_language=text_to_speech_language,
    .pause=text_to_speech_pause,
    .resume=text_to_speech_resume,
    .cancel=text_to_speech_cancel
};

#endif