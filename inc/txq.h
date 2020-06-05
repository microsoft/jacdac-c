#pragma once

void txq_init(void);
void txq_flush(void);
int txq_is_idle(void);
void *txq_push(unsigned service_num, unsigned service_cmd, const void *data, unsigned service_size);
void txq_push_event_ex(srv_t *srv, uint32_t eventid, uint32_t arg);
static inline void txq_push_event(srv_t *srv, uint32_t eventid) {
    txq_push_event_ex(srv, eventid, 0);
}