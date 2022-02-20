// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_protocol.h"
#include "jd_client.h"
#include "jd_pipes.h"

// #define LOG JD_LOG
#define LOG JD_NOLOG

#define MAX_SERV 32

#define IN_SERV_INIT 0xff
#define IN_SERV_SLEEP 0xfe

static srv_t **services;
static uint8_t num_services, reset_counter, packets_sent;
static uint8_t curr_service_process;
static uint32_t lastMax, lastDisconnectBlink, nextAnnounce;

struct srv_state {
    SRV_COMMON;
};

#define REG_IS_SIGNED(r) ((r) <= 4 && !((r)&1))
#define REG_IS_OPT(r) ((r) >= _REG_OPT8)
static const uint8_t regSize[16] = {1, 1, 2, 2, 4, 4, 4, 8, 1, 0, 1, 2, 4, JD_PTRSIZE};

static int is_zero(const uint8_t *p, uint32_t sz) {
    while (sz--)
        if (*p++ != 0x00)
            return 0;
    return 1;
}

int service_handle_register_final(srv_t *state, jd_packet_t *pkt, const uint16_t sdesc[]) {
    int r = service_handle_register(state, pkt, sdesc);
    if (r == 0)
        jd_send_not_implemented(pkt);
    return r;
}

int service_handle_register(srv_t *state, jd_packet_t *pkt, const uint16_t sdesc[]) {
    uint16_t cmd = pkt->service_command;
    bool is_get = JD_IS_GET(cmd);
    bool is_set = JD_IS_SET(cmd);
    if (!is_get && !is_set)
        return 0;

    if (is_set && pkt->service_size == 0)
        return 0;

    int reg = JD_REG_CODE(cmd);

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

        if (tp != _REG_BIT && tp != _REG_BYTES) {
            if (bitoffset) {
                bitoffset = 0;
                offset++;
            }
            int align = regsz < JD_PTRSIZE ? regsz - 1 : JD_PTRSIZE - 1;
            offset = (offset + align) & ~align;
        }

        LOG("%x:%d:%d", (sd & 0xfff), offset, regsz);

        if ((sd & 0xfff) == reg) {
            uint8_t *sptr = (uint8_t *)state + offset;
            if (is_get) {
                if (tp == _REG_BIT) {
                    uint8_t v = *sptr & (1 << bitoffset) ? 1 : 0;
                    jd_send(pkt->service_index, pkt->service_command, &v, 1);
                } else {
                    if (REG_IS_OPT(tp) && is_zero(sptr, regsz))
                        return 0;
                    jd_send(pkt->service_index, pkt->service_command, sptr, regsz);
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
                    int fill = !REG_IS_SIGNED(tp)                          ? 0
                               : (pkt->data[pkt->service_size - 1] & 0x80) ? 0xff
                                                                           : 0;
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
        jd_send(JD_SERVICE_INDEX_CRC_ACK, frame->crc, NULL, 0);
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
    r->service_index = num_services;
    // sleeping is allowed in service init
    curr_service_process = IN_SERV_INIT;
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
    curr_service_process = 0;
    services = jd_alloc(sizeof(void *) * num_services);
    memcpy(services, tmp, sizeof(void *) * num_services);

    // don't flash red initially
    lastDisconnectBlink = tim_get_micros() + 1000000;
}

void jd_services_packet_queued() {
    packets_sent++;
}

void jd_services_announce() {
    jd_alloc_stack_check();

    uint32_t dst[num_services];
    for (int i = 0; i < num_services; ++i)
        dst[i] = services[i]->vt->service_class;
    if (reset_counter < JD_CONTROL_ANNOUNCE_FLAGS_RESTART_COUNTER_STEADY)
        reset_counter++;

    uint16_t adflags = reset_counter;
#if JD_CONFIG_STATUS == 1
#ifdef PIN_LED_R
    adflags |= JD_CONTROL_ANNOUNCE_FLAGS_STATUS_LIGHT_RGB_FADE;
#else
    adflags |= JD_CONTROL_ANNOUNCE_FLAGS_STATUS_LIGHT_MONO;
#endif
#endif

    adflags |= JD_CONTROL_ANNOUNCE_FLAGS_SUPPORTS_ACK |
               JD_CONTROL_ANNOUNCE_FLAGS_SUPPORTS_BROADCAST |
               JD_CONTROL_ANNOUNCE_FLAGS_SUPPORTS_FRAMES;

    dst[0] = adflags | ((packets_sent + 1) << 16);

    if (jd_send(JD_SERVICE_INDEX_CONTROL, JD_CONTROL_CMD_SERVICES, dst, num_services * 4) == 0) {
        packets_sent = 0;
    }
}

static void handle_ctrl_tick(jd_packet_t *pkt) {
    if (pkt->service_command == JD_CONTROL_CMD_SERVICES) {
        // client? blink!
        if (pkt->service_size >= 4 && pkt->data[1] & (JD_CONTROL_ANNOUNCE_FLAGS_IS_CLIENT >> 8)) {
            lastMax = now;
            jd_status(JD_STATUS_CONNECTED);
        }
    }
}

__attribute__((weak)) void jd_app_handle_packet(jd_packet_t *pkt) {}
__attribute__((weak)) void jd_app_handle_command(jd_packet_t *pkt) {}

void jd_services_handle_packet(jd_packet_t *pkt) {
    jd_app_handle_packet(pkt);

#if JD_PIPES
    jd_opipe_handle_packet(pkt);
    jd_ipipe_handle_packet(pkt);
#endif

#if JD_CLIENT
    jd_client_handle_packet(pkt);
#endif

    if (!(pkt->flags & JD_FRAME_FLAG_COMMAND)) {
        if (pkt->service_index == 0)
            handle_ctrl_tick(pkt);
        return;
    }

    if (pkt->device_identifier == jd_device_id()) {
        jd_app_handle_command(pkt);
        if (pkt->service_index < num_services) {
            srv_t *s = services[pkt->service_index];
            s->vt->handle_pkt(s, pkt);
        }
    } else if (pkt->flags & JD_FRAME_FLAG_IDENTIFIER_IS_SERVICE_CLASS) {
        for (int i = 0; i < num_services; ++i) {
            srv_t *s = services[i];
            if (pkt->device_identifier == s->vt->service_class) {
                pkt->service_index = i;
                s->vt->handle_pkt(s, pkt);
            }
        }
    }
}

void jd_services_tick() {
    if (jd_should_sample(&nextAnnounce, 500000))
        jd_services_announce();

    if (jd_should_sample(&lastDisconnectBlink, 2000000)) {
        if (!lastMax || in_past(lastMax + 2000000)) {
            lastMax = 0;
            jd_status(JD_STATUS_DISCONNECTED);
        }
    }

    // do ctrl process regardless of sleep status
    services[0]->vt->process(services[0]);

    // while in sleep state, do not run any more nested process()
    if (curr_service_process != IN_SERV_SLEEP) {
        for (int i = 1; i < num_services; ++i) {
            curr_service_process = i;
            services[i]->vt->process(services[i]);
        }
        curr_service_process = 0;
    }

    jd_process_event_queue();

#if JD_CONFIG_STATUS == 1
    jd_status_process();
#endif

#if JD_CLIENT
    jd_client_process();
#endif

#if JD_PIPES
    jd_opipe_process();
#endif

    jd_tx_flush();
}

static void jd_process_everything_core(void) {
    jd_frame_t *fr = jd_rx_get_frame();
    if (fr)
        jd_services_process_frame(fr);

    jd_services_tick();
    app_process();
}

static void refresh_now(void) {
    uint64_t now_long = tim_get_micros();
    now = (uint32_t)now_long;
}

void jd_process_everything() {
    refresh_now();
    jd_process_everything_core();
}

void jd_services_sleep_us(uint32_t delta) {
    if (delta <= 1)
        return; // already done...

    uint8_t curr_serv = curr_service_process;
    // short sleeps and ones on init are always all right
    if (delta < 50 || curr_serv == IN_SERV_INIT) {
        target_wait_us(delta);
        return;
    }

    if (!curr_serv || curr_serv == IN_SERV_SLEEP) {
        // not allowed outside of regular (non-ctrl) service process() callback
        DMESG("sleep outside of process()");
        jd_panic();
    }
    curr_service_process = IN_SERV_SLEEP;
    refresh_now();
    uint32_t end_time = now + delta;
    while (in_future(end_time)) {
        jd_process_everything_core();
        refresh_now();
    }
    curr_service_process = curr_serv;
}

void dump_pkt(jd_packet_t *pkt, const char *msg) {
    LOG("pkt[%s]; s#=%d sz=%d %x", msg, pkt->service_index, pkt->service_size,
        pkt->service_command);
}

__attribute__((weak)) void app_process(void) {}