#include "jd_services.h"
#include "interfaces/jd_pins.h"
#include "jacdac/dist/c/speechsynthesis.h"

struct srv_state {
    SRV_COMMON;
    const speech_synth_api_t *api;
    char lang[20];
    uint8_t volume;
    uint32_t pitch;
    uint32_t rate;
};

REG_DEFINITION(                                   //
    tts_regs,                                   //
    REG_SRV_COMMON,                         //
    REG_BYTES(JD_SPEECH_SYNTHESIS_REG_LANG,20),                 //
    REG_U8(JD_SPEECH_SYNTHESIS_REG_VOLUME),                //
    REG_U32(JD_SPEECH_SYNTHESIS_REG_PITCH),                //
    REG_U32(JD_SPEECH_SYNTHESIS_REG_RATE),                //
)

void reflect_register_state(srv_t* state) {
}

void tts_process(srv_t * state) {
    (void)state;
}


void tts_handle_packet(srv_t *state, jd_packet_t *pkt) {
    switch (service_handle_register(state, pkt, tts_regs)) {
        case JD_SPEECH_SYNTHESIS_REG_LANG:
            reflect_register_state(state);
            break;
    }
}

SRV_DEF(tts, JD_SERVICE_CLASS_SPEECH_SYNTHESIS);
void speech_synthesis_init(const speech_synth_api_t *params) {
    SRV_ALLOC(tts);
}