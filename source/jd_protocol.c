// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_protocol.h"

void _jd_phys_start(void);

void jd_init() {
    jd_alloc_init();
    jd_tx_init();
    jd_rx_init();
    jd_services_init();
    // if we don't start it explicitly, it will only start on incoming packet
    _jd_phys_start();

#if JD_CONFIG_STATUS == 1
    jd_status_init();
#endif
    jd_status(JD_STATUS_STARTUP);
}

int jd_respond_u8(jd_packet_t *pkt, uint8_t v) {
    return jd_send(pkt->service_number, pkt->service_command, &v, sizeof(v));
}

int jd_respond_u16(jd_packet_t *pkt, uint16_t v) {
    return jd_send(pkt->service_number, pkt->service_command, &v, sizeof(v));
}

int jd_respond_u32(jd_packet_t *pkt, uint32_t v) {
    return jd_send(pkt->service_number, pkt->service_command, &v, sizeof(v));
}

int jd_send_not_implemented(jd_packet_t *pkt) {
    uint16_t payload[2] = {pkt->service_command, pkt->crc};
    return jd_send(pkt->service_number, 0x03, payload, 4);
}
