#include "jd_protocol.h"
#include "jd_pipes.h"

// TODO timeouts?

// we don't lock and instead assume we're not running in an interrupt handler
#define LOCK()                                                                                     \
    if (target_in_irq())                                                                           \
    JD_PANIC()
#define UNLOCK() ((void)0)

static jd_ipipe_desc_t *ipipes;

static void jd_ipipe_free(jd_ipipe_desc_t *str) {
    if (ipipes == str) {
        ipipes = str->next;
    } else {
        for (jd_ipipe_desc_t *ss = ipipes; ss; ss = ss->next)
            if (ss->next == str) {
                ss->next = str->next;
                break;
            }
    }
    str->counter = 0;
}

static bool is_free_port(int p) {
    if (!p)
        return false;
    for (jd_ipipe_desc_t *ss = ipipes; ss; ss = ss->next) {
        if ((ss->counter >> JD_PIPE_PORT_SHIFT) == p)
            return false;
    }
    return true;
}

int jd_ipipe_open(jd_ipipe_desc_t *str, jd_ipipe_handler_t handler,
                  jd_ipipe_handler_t meta_handler) {
    LOCK();
    jd_ipipe_free(str);
    str->handler = handler;
    str->meta_handler = meta_handler;
    for (;;) {
        int p = jd_random() & 511;
        if (is_free_port(p)) {
            str->counter = p << JD_PIPE_PORT_SHIFT;
            break;
        }
    }
    str->next = ipipes;
    ipipes = str;
    UNLOCK();
    return str->counter >> JD_PIPE_PORT_SHIFT;
}

void jd_ipipe_handle_packet(jd_packet_t *pkt) {
    if (pkt->service_index != JD_SERVICE_INDEX_STREAM || jd_is_report(pkt) ||
        pkt->device_identifier != jd_device_id())
        return;

    uint16_t cmd = pkt->service_command;
    int port = cmd >> JD_PIPE_PORT_SHIFT;
    for (jd_ipipe_desc_t *s = ipipes; s; s = s->next) {
        if ((s->counter >> JD_PIPE_PORT_SHIFT) == port) {
            if ((s->counter & JD_PIPE_COUNTER_MASK) == (cmd & JD_PIPE_COUNTER_MASK)) {
                s->counter = ((s->counter + 1) & JD_PIPE_COUNTER_MASK) |
                             (s->counter & ~JD_PIPE_COUNTER_MASK);
                if (cmd & JD_PIPE_METADATA_MASK) {
                    s->meta_handler(s, pkt);
                } else {
                    s->handler(s, pkt);
                }
                if (cmd & JD_PIPE_CLOSE_MASK) {
                    s->meta_handler(s, NULL); // indicate EOF
                    jd_ipipe_free(s);
                }
            }
            break;
        }
    }
}

void jd_ipipe_close(jd_ipipe_desc_t *str) {
    jd_ipipe_free(str);
}
