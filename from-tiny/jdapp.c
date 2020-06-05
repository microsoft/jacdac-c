#include "jdsimple.h"

#define MAX_SERV 32

static srv_t **services;
static uint8_t num_services;

static uint64_t maxId;
static uint32_t lastMax, lastDisconnectBlink;

struct srv_state {
    SRV_COMMON;
};

srv_t *srv_alloc(const srv_vt_t *vt) {
    // always allocate instances idx - it should be stable when we disable some services
    if (num_services >= MAX_SERV)
        jd_panic();
    srv_t *r = alloc(vt->state_size);
    r->vt = vt;
    r->service_number = num_services;
    services[num_services++] = r;

    return r;
}

void app_init_services() {
    srv_t *tmp[MAX_SERV + 1];
    uint16_t hashes[MAX_SERV];
    tmp[MAX_SERV] = (srv_t *)hashes; // avoid global variable
    services = tmp;
    ctrl_init();
    jdcon_init();
    init_services();
    services = alloc(sizeof(void *) * num_services);
    memcpy(services, tmp, sizeof(void *) * num_services);
}

void app_queue_annouce() {
    alloc_stack_check();

    uint32_t *dst =
        txq_push(JD_SERVICE_NUMBER_CTRL, JD_CMD_ADVERTISEMENT_DATA, NULL, num_services * 4);
    if (!dst)
        return;
    for (int i = 0; i < num_services; ++i)
        dst[i] = services[i]->vt->service_class;

#if 0
    static uint32_t pulsesample;
    static uint32_t pulse = 850 * 1000;

    if (should_sample(&pulsesample, 15 * 1000 * 1000)) {
        pwr_pin_enable(1);
        target_wait_us(pulse);
        pwr_pin_enable(0);
        jdcon_warn("P: %d t=%ds", pulse, now / 1000000);
        pulse = (pulse * 9) / 10;
        #if 1
        if(pulse<1000)
        pulse=1000;
        #endif
    }
#endif

}

static void handle_ctrl_tick(jd_packet_t *pkt) {
    if (pkt->service_command == JD_CMD_ADVERTISEMENT_DATA) {
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

void app_handle_packet(jd_packet_t *pkt) {
    if (!(pkt->flags & JD_FRAME_FLAG_COMMAND)) {
        if (pkt->service_number == 0)
            handle_ctrl_tick(pkt);
        return;
    }

    bool matched_devid = pkt->device_identifier == device_id();

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

    if (pkt->service_number < num_services) {
        srv_t *s = services[pkt->service_number];
        s->vt->handle_pkt(s, pkt);
    }
}

void app_process() {
    app_process_frame();

    if (should_sample(&lastDisconnectBlink, 250000)) {
        if (in_past(lastMax + 2000000)) {
            led_blink(5000);
        }
    }

    for (int i = 0; i < num_services; ++i) {
        services[i]->vt->process(services[i]);
    }

    txq_flush();
}

void dump_pkt(jd_packet_t *pkt, const char *msg) {
    DMESG("pkt[%s]; s#=%d sz=%d %x", msg, pkt->service_number, pkt->service_size,
          pkt->service_command);
}
