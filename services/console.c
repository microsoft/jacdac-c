#include "jd_protocol.h"
#include "interfaces/jd_console.h"

#define PRI_DEBUG 0
#define PRI_LOG 1
#define PRI_WARNING 2
#define PRI_ERROR 3

#define CMD_MSG_START 0x80
#define REG_MIN_PRI 0x80

struct srv_state {
    SRV_COMMON;
    uint8_t minPri;
};

REG_DEFINITION(          //
    jdcon_regs,          //
    REG_SRV_BASE,        //
    REG_U8(REG_MIN_PRI), //
)

static srv_t *state_;

void jdcon_logv(int level, const char *format, va_list ap) {
    srv_t *ctx = state_;
    if (!ctx)
        return;
    if (level < ctx->minPri)
        return;
    char buf[100];
    codal_vsprintf(buf, sizeof(buf), format, ap);
    jd_send(ctx->service_number, CMD_MSG_START + level, buf, strlen(buf));
}

void jdcon_log(const char *format, ...) {
    va_list arg;
    va_start(arg, format);
    jdcon_logv(PRI_LOG, format, arg);
    va_end(arg);
}

void jdcon_warn(const char *format, ...) {
    va_list arg;
    va_start(arg, format);
    jdcon_logv(PRI_WARNING, format, arg);
    va_end(arg);
}

void jdcon_process(srv_t *state) {}

void jdcon_handle_packet(srv_t *state, jd_packet_t *pkt) {
    service_handle_register(state, pkt, jdcon_regs);
}

SRV_DEF(jdcon, JD_SERVICE_CLASS_LOGGER);
void jdcon_init(void) {
    SRV_ALLOC(jdcon);
    state_ = state;
#ifdef JD_CONSOLE
    state->minPri = PRI_LOG;
#else
    state->minPri = PRI_WARNING;
#endif
}
