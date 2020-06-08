#pragma once

void jd_rx_init(void);
void jd_rx_flush(void);
int jd_rx_is_idle(void);
// void* jd_send(unsigned service_num, unsigned service_cmd, const void *data, unsigned service_size);
// void jd_send_event_ext(srv_t *srv, uint32_t eventid, uint32_t arg);
// static inline void jd_send_event(srv_t *srv, uint32_t eventid) {
//     jd_send_event_ext(srv, eventid, 0);
// }