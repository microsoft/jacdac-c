// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_services.h"
#include "jd_console.h"
#include "jacdac/dist/c/logger.h"

struct srv_state {
    SRV_COMMON;
    uint8_t minPri;
};

REG_DEFINITION(                         //
    jdcon_regs,                         //
    REG_SRV_COMMON,                     //
    REG_U8(JD_LOGGER_REG_MIN_PRIORITY), //
)

static srv_t *state_;

void jdcon_logv(int level, const char *format, va_list ap) {
    srv_t *ctx = state_;
    if (!ctx)
        return;
    if (level < ctx->minPri)
        return;
    char buf[100];
    jd_vsprintf(buf, sizeof(buf), format, ap);
    jd_send(ctx->service_index, JD_LOGGER_CMD_DEBUG + level, buf, strlen(buf));
}

void jdcon_log(const char *format, ...) {
    va_list arg;
    va_start(arg, format);
    jdcon_logv(JD_LOGGER_PRIORITY_LOG, format, arg);
    va_end(arg);
}

void jdcon_warn(const char *format, ...) {
    va_list arg;
    va_start(arg, format);
    jdcon_logv(JD_LOGGER_PRIORITY_WARNING, format, arg);
    va_end(arg);
}

void jdcon_process(srv_t *state) {}

void jdcon_handle_packet(srv_t *state, jd_packet_t *pkt) {
    service_handle_register_final(state, pkt, jdcon_regs);
}

SRV_DEF(jdcon, JD_SERVICE_CLASS_LOGGER);
void jdcon_init(void) {
    SRV_ALLOC(jdcon);
    state_ = state;
#ifdef JD_CONSOLE
    state->minPri = JD_LOGGER_PRIORITY_LOG;
#else
    state->minPri = JD_LOGGER_PRIORITY_WARNING;
#endif
}
