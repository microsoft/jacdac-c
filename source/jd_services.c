// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "jd_control.h"
#include "jd_util.h"
#include "interfaces/jd_tx.h"
#include "interfaces/jd_rx.h"
#include "interfaces/jd_hw.h"
#include "interfaces/jd_app.h"
#include "interfaces/jd_alloc.h"

// #define LOG JD_LOG
#define LOG JD_NOLOG

#define MAX_SERV 32

static srv_t **services;
static uint8_t num_services, reset_counter;

static uint64_t maxId;
static uint32_t lastMax, lastDisconnectBlink;

struct srv_state {
    SRV_COMMON;
};

#define REG_IS_SIGNED(r) ((r) <= 4 && !((r)&1))
static const uint8_t regSize[16] = {1, 1, 2, 2, 4, 4, 4, 8, 1};

int service_handle_register(srv_t *state, jd_packet_t *pkt, const uint16_t sdesc[]) {
    bool is_get = (pkt->service_command >> 12) == (JD_CMD_GET_REGISTER >> 12);
    bool is_set = (pkt->service_command >> 12) == (JD_CMD_SET_REGISTER >> 12);
    if (!is_get && !is_set)
        return 0;

    if (is_set && pkt->service_size == 0)
        return 0;

    int reg = pkt->service_command & 0xfff;

    if (reg >= 0xf00) // these are reserved
        return 0;

    if (is_set && (reg & 0xf00) == 0x100)
        return 0; // these are read-only

    uint32_t offset = 0;
    uint8_t bitoffset = 0;

    LOG("handle %x", reg);

    for (int i = 0; sdesc[i] != JD_REG_END; ++i) {
        uint16_t sd = sdesc[i];
        int tp = sd >> 12;
        int regsz = regSize[tp];

        if (tp == _REG_BYTES)
            regsz = sdesc[++i];

        if (!regsz)
            jd_panic();

        if (tp != _REG_BIT) {
            if (bitoffset) {
                bitoffset = 0;
                offset++;
            }
            int align = regsz < 4 ? regsz - 1 : 3;
            offset = (offset + align) & ~align;
        }

        LOG("%x:%d:%d", (sd & 0xfff), offset, regsz);

        if ((sd & 0xfff) == reg) {
            uint8_t *sptr = (uint8_t *)state + offset;
            if (is_get) {
                if (tp == _REG_BIT) {
                    uint8_t v = *sptr & (1 << bitoffset) ? 1 : 0;
                    jd_send(pkt->service_number, pkt->service_command, &v, 1);
                } else {
                    jd_send(pkt->service_number, pkt->service_command, sptr, regSize[tp]);
                }
                return -reg;
            } else {
                if (tp == _REG_BIT) {
                    LOG("bit @%d - %x", offset, reg);
                    if (pkt->data[0])
                        *sptr |= 1 << bitoffset;
                    else
                        *sptr &= ~(1 << bitoffset);
                } else if (regsz <= pkt->service_size) {
                    LOG("exact @%d - %x", offset, reg);
                    memcpy(sptr, pkt->data, regsz);
                } else {
                    LOG("too little @%d - %x", offset, reg);
                    memcpy(sptr, pkt->data, pkt->service_size);
                    int fill = !REG_IS_SIGNED(tp)
                                   ? 0
                                   : (pkt->data[pkt->service_size - 1] & 0x80) ? 0xff : 0;
                    memset(sptr + pkt->service_size, fill, regsz - pkt->service_size);
                }
                return reg;
            }
        }

        if (tp == _REG_BIT) {
            bitoffset++;
            if (bitoffset == 8) {
                offset++;
                bitoffset = 0;
            }
        } else {
            offset += regsz;
        }
    }

    return 0;
}

void jd_services_process_frame(jd_frame_t *frame) {
    if (!frame)
        return;

    if (frame->flags & JD_FRAME_FLAG_ACK_REQUESTED && frame->flags & JD_FRAME_FLAG_COMMAND &&
        frame->device_identifier == jd_device_id()) {
        jd_send(JD_SERVICE_NUMBER_CRC_ACK, frame->crc, NULL, 0);
        jd_tx_flush(); // the app handling below can take time
    }

    for (;;) {
        jd_services_handle_packet((jd_packet_t *)frame);
        if (!jd_shift_frame(frame))
            break;
    }
}

srv_t *jd_allocate_service(const srv_vt_t *vt) {
    // always allocate instances idx - it should be stable when we disable some services
    if (num_services >= MAX_SERV)
        jd_panic();
    srv_t *r = jd_alloc(vt->state_size);
    r->vt = vt;
    r->service_number = num_services;
    services[num_services++] = r;

    return r;
}

void jd_services_init() {
    num_services = 0;
    srv_t *tmp[MAX_SERV];
    services = tmp;

    jd_ctrl_init();
#ifdef JD_CONSOLE
    jdcon_init();
#endif
    app_init_services();
    services = jd_alloc(sizeof(void *) * num_services);
    memcpy(services, tmp, sizeof(void *) * num_services);
}

void jd_services_announce() {
    jd_alloc_stack_check();

    uint32_t dst[num_services];
    for (int i = 0; i < num_services; ++i)
        dst[i] = services[i]->vt->service_class;
    if (reset_counter < 0xf)
        reset_counter++;
    dst[0] = reset_counter | JD_ADVERTISEMENT_0_ACK_SUPPORTED;
    jd_send(JD_SERVICE_NUMBER_CONTROL, JD_CONTROL_CMD_SERVICES, dst, num_services * 4);
}

static void handle_ctrl_tick(jd_packet_t *pkt) {
    if (pkt->service_command == JD_CONTROL_CMD_SERVICES) {
        // if we have not seen maxId for 1.1s, find a new maxId
        if (pkt->device_identifier < maxId && in_past(lastMax + 1100000)) {
            maxId = pkt->device_identifier;
        }

        // maxId? blink!
        if (pkt->device_identifier >= maxId) {
            maxId = pkt->device_identifier;
            lastMax = now;
            led_blink(50);
        }
    }
}

__attribute__((weak)) void jd_app_handle_packet(jd_packet_t *pkt) {}
__attribute__((weak)) void jd_app_handle_command(jd_packet_t *pkt) {}

void jd_services_handle_packet(jd_packet_t *pkt) {
    jd_app_handle_packet(pkt);

    if (!(pkt->flags & JD_FRAME_FLAG_COMMAND)) {
        if (pkt->service_number == 0)
            handle_ctrl_tick(pkt);
        return;
    }

    bool matched_devid = pkt->device_identifier == jd_device_id();

    if (pkt->flags & JD_FRAME_FLAG_IDENTIFIER_IS_SERVICE_CLASS) {
        for (int i = 0; i < num_services; ++i) {
            if (pkt->device_identifier == services[i]->vt->service_class) {
                pkt->service_number = i;
                matched_devid = true;
                break;
            }
        }
    }

    if (!matched_devid)
        return;

    jd_app_handle_command(pkt);

    if (pkt->service_number < num_services) {
        srv_t *s = services[pkt->service_number];
        s->vt->handle_pkt(s, pkt);
    }
}

void jd_services_tick() {
    if (jd_should_sample(&lastDisconnectBlink, 250000)) {
        if (in_past(lastMax + 2000000)) {
            led_blink(5000);
        }
    }

    for (int i = 0; i < num_services; ++i) {
        services[i]->vt->process(services[i]);
    }

    jd_process_event_queue();

    jd_tx_flush();
}

void dump_pkt(jd_packet_t *pkt, const char *msg) {
    LOG("pkt[%s]; s#=%d sz=%d %x", msg, pkt->service_number, pkt->service_size,
        pkt->service_command);
}
