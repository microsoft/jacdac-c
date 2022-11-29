#include "color.h"
#include "interfaces/jd_sensor_api.h"
#include "tts.h"
#include "services/jd_services.h"
#include "tinyhw.h"

#ifdef MIKROBUS_AVAILABLE

void fatal_error( uint16_t *err ) {
    JD_PANIC();
}

void msg_blocked( uint16_t *req, uint16_t *err ) {
    DMESG("MSG BLOCKED %d ERR %d", *req, *err);
}

void text_to_speech_volume(uint8_t volume) {
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

static inline int scale_rate(uint32_t rate) {
    // 0x004A ~ 0x0259
    return 0;
}

static inline int scale_volume(uint8_t vol) {
    if (vol > 100)
        vol = 100;

    const float DB_MIN = -49;
    const float DB_MAX = 19;
    int retval = ((float)vol / 100) * (DB_MAX - DB_MIN) + DB_MIN;
    return retval;     
}

void text_to_speech_init(uint8_t volume, uint32_t rate, uint32_t pitch, char* language) {
    // chip select
    pin_setup_output(MIKROBUS_CS);
    pin_set(MIKROBUS_CS, 1);
    // mute
    pin_setup_output(MIKROBUS_AN);
    pin_set(MIKROBUS_AN, 0);
    // reset
    pin_setup_output(MIKROBUS_RST);
    pin_set(MIKROBUS_RST, 1);

    // data ready
    pin_setup_input(MIKROBUS_INT, PIN_PULL_NONE);

    sspi_init(1, 1, 1);

    tts_init();
    tts_setup();
    
    tts_msg_block_callback(msg_blocked);
    tts_fatal_err_callback(fatal_error);
    
    tts_config( 0x01, false, TTSV_US, 0x0100 );
    tts_volume_set(19);
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