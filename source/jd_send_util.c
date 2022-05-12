// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_protocol.h"

int jd_respond_u8(jd_packet_t *pkt, uint8_t v) {
    return jd_send(pkt->service_index, pkt->service_command, &v, sizeof(v));
}

int jd_respond_u16(jd_packet_t *pkt, uint16_t v) {
    return jd_send(pkt->service_index, pkt->service_command, &v, sizeof(v));
}

int jd_respond_u32(jd_packet_t *pkt, uint32_t v) {
    return jd_send(pkt->service_index, pkt->service_command, &v, sizeof(v));
}

int jd_respond_empty(jd_packet_t *pkt) {
    return jd_send(pkt->service_index, pkt->service_command, "", 0);
}

int jd_respond_string(jd_packet_t *pkt, const char *str) {
    if (!str || !*str)
        return jd_respond_empty(pkt);
    return jd_send(pkt->service_index, pkt->service_command, str, strlen(str));
}

int jd_send_not_implemented(jd_packet_t *pkt) {
    // don't complain about broadcast packets
    if (pkt->flags & JD_FRAME_FLAG_IDENTIFIER_IS_SERVICE_CLASS)
        return 0;
    jd_system_command_not_implemented_report_t payload = {.service_command = pkt->service_command,
                                                          .packet_crc = pkt->crc};
    return jd_send(pkt->service_index, JD_CMD_COMMAND_NOT_IMPLEMENTED, &payload, sizeof(payload));
}

int jd_send_pkt(jd_packet_t *pkt) {
    pkt->_size = pkt->service_size + 4;
    jd_frame_t *f = (jd_frame_t *)pkt;
    jd_compute_crc(f);
    return jd_send_frame(f);
}

int jd_block_register(jd_packet_t *pkt, uint16_t reg_code) {
    if (pkt->service_command == JD_GET(reg_code) || pkt->service_command == JD_SET(reg_code)) {
        jd_send_not_implemented(pkt);
        return 1;
    }
    return 0;
}
