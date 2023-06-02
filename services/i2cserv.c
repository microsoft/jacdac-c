// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "interfaces/jd_pins.h"
#include "jacdac/dist/c/i2c.h"

struct srv_state {
    SRV_COMMON;
    uint8_t enabled;
};

REG_DEFINITION(            //
    i2cserv_regs,          //
    REG_SRV_COMMON,        //
    REG_U8(JD_I2C_REG_OK), //
)

void i2cserv_process(srv_t *state) {}

static void i2cserv_transaction(srv_t *state, jd_packet_t *pkt) {
    jd_i2c_transaction_t *d = (void *)pkt->data;
    int wrsize = pkt->service_size - 2;
    if (wrsize > 0) {
        if (i2c_write_ex(d->address & 0x7f, d->write_buf, wrsize, d->num_read > 0) != 0) {
            jd_respond_u8(pkt, JD_I2C_STATUS_NACK_ADDR);
            return;
        }
    }

    if (d->num_read > 0) {
        uint8_t rdbuf[d->num_read + 1];
        if (i2c_read_ex(d->address & 0x7f, rdbuf + 1, d->num_read) != 0) {
            jd_respond_u8(pkt, JD_I2C_STATUS_NACK_DATA);
        } else {
            rdbuf[0] = JD_I2C_STATUS_OK;
            jd_send(pkt->service_index, pkt->service_command, rdbuf, d->num_read + 1);
        }
    } else {
        jd_respond_u8(pkt, JD_I2C_STATUS_OK);
    }
}

void i2cserv_handle_packet(srv_t *state, jd_packet_t *pkt) {
    switch (pkt->service_command) {
    case JD_I2C_CMD_TRANSACTION:
        if (!state->enabled)
            jd_respond_u8(pkt, JD_I2C_STATUS_NO_I2C);
        else if (pkt->service_size >= 2)
            i2cserv_transaction(state, pkt);
        break;
    default:
        service_handle_register_final(state, pkt, i2cserv_regs);
        break;
    }
}

SRV_DEF(i2cserv, JD_SERVICE_CLASS_I2C);
void i2cserv_init(void) {
    SRV_ALLOC(i2cserv);
    if (i2c_init_() == 0)
        state->enabled = 1;
}
