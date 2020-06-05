#pragma once

#include "jdprofile.h"

#include "jdlow.h"
#include "host.h"
#include "lib.h"

// main.c
void led_set(int state);
void led_blink(int us);
void pwr_pin_enable(int en);

// jdapp.c
void app_process(void);
void app_init_services(void);

// txq.c
void txq_init(void);
void txq_flush(void);
int txq_is_idle(void);
void *txq_push(unsigned service_num, unsigned service_cmd, const void *data, unsigned service_size);
void txq_push_event_ex(srv_t *srv, uint32_t eventid, uint32_t arg);
static inline void txq_push_event(srv_t *srv, uint32_t eventid) {
    txq_push_event_ex(srv, eventid, 0);
}

void ctrl_process(srv_t *_state);
void ctrl_handle_packet(srv_t *_state, jd_packet_t *pkt);
void app_handle_packet(jd_packet_t *pkt);
void app_process_frame(void);

void dump_pkt(jd_packet_t *pkt, const char *msg);
