#include "jd_services.h"
#include "interfaces/jd_pins.h"
#include "jacdac/dist/c/speechsynthesis.h"

#define SPEAK_CMD 0x01
#define CANCEL_CMD 0x02
#define SET_LANG 0x04
#define SET_RATE 0x08
#define SET_PITCH 0x10
#define SET_VOLUME 0x10
static char phrase[200];

struct srv_state {
    SRV_COMMON;
    const speech_synth_api_t *api;
    char lang[20];
    uint32_t pitch;
    uint32_t rate;
    uint8_t enabled;
    uint8_t volume;
    uint8_t flags;
};

REG_DEFINITION(        //
    speech_synth_regs, //
    REG_SRV_COMMON,    //
    REG_U32(JD_REG_PADDING),
    REG_BYTES(JD_REG_PADDING, 20),           // don't know how to do strings properly yet
    REG_U32(JD_SPEECH_SYNTHESIS_REG_PITCH),  //
    REG_U32(JD_SPEECH_SYNTHESIS_REG_RATE),   //
    REG_U8(JD_SPEECH_SYNTHESIS_REG_ENABLED), //
    REG_U8(JD_SPEECH_SYNTHESIS_REG_VOLUME),  //
)

void speech_synth_process(srv_t *state) {
    if (state->flags & SPEAK_CMD) {
        state->api->speak(phrase);
        state->flags &= ~SPEAK_CMD;
    }

    if (state->flags & CANCEL_CMD) {
        state->api->cancel();
        state->flags &= ~CANCEL_CMD;
    }

    if (state->flags & SET_LANG) {
        state->api->set_language(state->lang);
        state->flags &= ~SET_LANG;
    }

    if (state->flags & SET_PITCH) {
        state->api->set_pitch(state->pitch);
        state->flags &= ~SET_PITCH;
    }

    if (state->flags & SET_VOLUME) {
        state->api->set_volume(state->volume);
        state->flags &= ~SET_VOLUME;
    }

    if (state->flags & SET_RATE) {
        state->api->set_rate(state->rate);
        state->flags &= ~CANCEL_CMD;
    }
}

// often underlying tts apis perform a synchronous wait, which can only happen in process.
// flag to process later
void handle_speak_cmd(srv_t *state, jd_packet_t *pkt) {
    if (!state->enabled)
        return;
    memset(phrase, 0, 200);
    memcpy(phrase, pkt->data, pkt->service_size);
    state->flags |= SPEAK_CMD;
}

void handle_cancel_cmd(srv_t *state, jd_packet_t *pkt) {
    state->flags |= CANCEL_CMD;
}

void speech_synth_handle_packet(srv_t *state, jd_packet_t *pkt) {
    switch (pkt->service_command) {
    case JD_SPEECH_SYNTHESIS_CMD_SPEAK:
        handle_speak_cmd(state, pkt);
        break;
    case JD_SPEECH_SYNTHESIS_CMD_CANCEL:
        handle_cancel_cmd(state, pkt);
        break;
    default:
        switch (service_handle_register_final(state, pkt, speech_synth_regs)) {
        case JD_SPEECH_SYNTHESIS_REG_LANG:
            state->flags |= SET_LANG;
            break;

        case JD_SPEECH_SYNTHESIS_REG_VOLUME:
            state->flags |= SET_VOLUME;
            break;

        case JD_SPEECH_SYNTHESIS_REG_PITCH:
            state->flags |= SET_PITCH;
            break;

        case JD_SPEECH_SYNTHESIS_REG_RATE:
            state->flags |= SET_RATE;
            break;
        }
    }
}

SRV_DEF(speech_synth, JD_SERVICE_CLASS_SPEECH_SYNTHESIS);
void speech_synthesis_init(const speech_synth_api_t *params) {
    SRV_ALLOC(speech_synth);
    state->api = params;
    state->flags = 0;
    state->pitch = 1 << 16;
    state->volume = 128;
    state->rate = 1 << 16;
    state->enabled = 1;
    memset(state->lang, 0, sizeof(state->lang));
    memcpy(state->lang, "en-US", sizeof("en-US"));
    state->api->init(state->volume, state->rate, state->pitch, state->lang);
}