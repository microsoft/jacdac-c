// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "jacdac/dist/c/settings.h"
#include "services/interfaces/jd_flash.h"

struct srv_state {
    SRV_COMMON;
};

REG_DEFINITION(     //
    settings_regs,  //
    REG_SRV_COMMON, //
)

#define MAX_KEY_SIZE 14

void settings_process(srv_t *state) {}

void settings_handle_packet(srv_t *state, jd_packet_t *pkt) {
    switch (pkt->service_command) {
    case JD_SETTINGS_CMD_DELETE:
    case JD_SETTINGS_CMD_SET:
    case JD_SETTINGS_CMD_GET: {
        char key[MAX_KEY_SIZE + 3];
        int ksz = pkt->service_size;
        if (ksz >= MAX_KEY_SIZE + 1)
            ksz = MAX_KEY_SIZE + 1;
        memcpy(key + 1, pkt->data, ksz);
        key[0] = ':';
        key[ksz + 1] = 0;
        ksz = strlen(key + 1);
        if (ksz > MAX_KEY_SIZE)
            return;

        uint8_t *data;
        int size;

        switch (pkt->service_command) {
        case JD_SETTINGS_CMD_GET:
            size = jd_settings_get_bin(key, NULL, 0);
            if (size < 0)
                size = 0;
            data = jd_alloc(ksz + 1 + size);
            memcpy(data, key + 1, ksz);
            jd_settings_get_bin(key, data + ksz + 1, size);
            jd_send(pkt->service_index, pkt->service_command, data, ksz + 1 + size);
            jd_free(data);
            break;

        case JD_SETTINGS_CMD_DELETE:
            jd_settings_set_bin(key, NULL, 0);
            break;

        case JD_SETTINGS_CMD_SET:
            size = pkt->service_size - ksz - 1;
            if (size < 0)
                size = 0;
            jd_settings_set_bin(key, pkt->data + ksz + 1, size);
            break;
        }

        break;
    }

    default:
        service_handle_register_final(state, pkt, settings_regs);
        break;
    }
}

SRV_DEF(settings, JD_SERVICE_CLASS_SETTINGS);
void settings_init(void) {
    SRV_ALLOC(settings);
}
