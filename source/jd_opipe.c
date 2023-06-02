#include "jd_protocol.h"
#include "jd_pipes.h"

// we don't lock and instead assume we're not running in an interrupt handler
#define LOCK()                                                                                     \
    if (target_in_irq())                                                                           \
    JD_PANIC()
#define UNLOCK() ((void)0)

#define ST_FREE 0x00
#define ST_OPEN 0x01
#define ST_CLOSED_UNSENT 0x80
#define ST_CLOSED_WAITING 0x82
#define ST_DROPPED 0x08

static jd_opipe_desc_t *opipes;

int _jd_tx_push_frame(jd_frame_t *f, int wait);

static int jd_opipe_send_close_pkt(jd_opipe_desc_t *str);

static void jd_opipe_unlink(jd_opipe_desc_t *str) {
    if (str->status == ST_FREE)
        return;
    if (opipes == str) {
        opipes = str->next;
    } else {
        for (jd_opipe_desc_t *ss = opipes; ss; ss = ss->next)
            if (ss->next == str) {
                ss->next = str->next;
                break;
            }
    }
    memset(str, 0, sizeof(*str));
}

int jd_opipe_open(jd_opipe_desc_t *str, uint64_t device_id, uint16_t port_num) {
    if (device_id == 0)
        return JD_PIPE_ERROR;

    jd_frame_t *f = &str->frame;

    LOCK();
    jd_opipe_unlink(str);
    f->device_identifier = device_id;
    f->flags = JD_FRAME_FLAG_COMMAND | JD_FRAME_FLAG_ACK_REQUESTED;
    str->counter = port_num << JD_PIPE_PORT_SHIFT;
    str->status = ST_OPEN;
    str->curr_retry = 0;
    str->next = opipes;
    opipes = str;
    UNLOCK();

    return JD_PIPE_OK;
}

int jd_opipe_open_cmd(jd_opipe_desc_t *str, jd_packet_t *pkt) {
    if (pkt->service_size < sizeof(jd_pipe_cmd_t))
        return JD_PIPE_ERROR;
    jd_pipe_cmd_t *sc = (jd_pipe_cmd_t *)pkt->data;
    return jd_opipe_open(str, sc->device_identifier, sc->port_num);
}

int jd_opipe_open_report(jd_opipe_desc_t *str, jd_packet_t *pkt) {
    if (pkt->service_size < sizeof(uint16_t))
        return JD_PIPE_ERROR;
    return jd_opipe_open(str, pkt->device_identifier, *(uint16_t *)pkt->data);
}

void jd_opipe_handle_packet(jd_packet_t *pkt) {
    if (pkt->service_index != JD_SERVICE_INDEX_CRC_ACK || (pkt->flags & JD_FRAME_FLAG_COMMAND))
        return;
    LOCK();
    for (jd_opipe_desc_t *str = opipes; str; str = str->next) {
        if (str->curr_retry && pkt->service_command == str->frame.crc &&
            str->frame.device_identifier == pkt->device_identifier) {
            str->curr_retry = 0;
            jd_reset_frame(&str->frame);
            if (str->status == ST_CLOSED_WAITING) {
                jd_opipe_unlink(str);
            } else if (str->status == ST_CLOSED_UNSENT) {
                jd_opipe_send_close_pkt(str);
            }
        }
    }
    UNLOCK();
}

void jd_opipe_process(void) {
    LOCK();
    for (jd_opipe_desc_t *str = opipes; str; str = str->next) {
        if (str->curr_retry && in_past(str->retry_time)) {
            if (str->curr_retry < JD_OPIPE_MAX_RETRIES + 1) {
                if (jd_send_frame(&str->frame) == 0) {
                    str->retry_time = now + ((4 * 1024) << str->curr_retry);
                    str->curr_retry++;
                } else {
                    // in case of ovf, give it some time
                    str->retry_time = now + (64 << 10);
                }
            } else if (str->curr_retry == JD_OPIPE_MAX_RETRIES + 1) {
                str->retry_time = now + 32 * 1024; // grace period for final ACK
                str->curr_retry++;
            } else {
                str->status = ST_DROPPED;
            }
        }
    }
    UNLOCK();
}

static void do_flush(jd_opipe_desc_t *str) {
    JD_ASSERT(str->curr_retry == 0);
    jd_compute_crc(&str->frame);
    str->curr_retry = 1;
    str->retry_time = now;
}

int jd_opipe_check_space(jd_opipe_desc_t *str, unsigned len) {
    if (str->status == ST_DROPPED)
        return JD_PIPE_TIMEOUT;
    if (str->status != ST_OPEN)
        return JD_PIPE_ERROR;
    if (str->curr_retry != 0)
        return JD_PIPE_TRY_AGAIN;

    if (str->frame.size + 4 + len <= JD_SERIAL_PAYLOAD_SIZE)
        return JD_PIPE_OK;

    do_flush(str);
    return JD_PIPE_TRY_AGAIN;
}

static int jd_opipe_write_ex(jd_opipe_desc_t *str, const void *data, unsigned len, int flags) {
    int r = jd_opipe_check_space(str, len);
    if (r)
        return r;

    void *trg = jd_push_in_frame(&str->frame, JD_SERVICE_INDEX_STREAM, str->counter | flags, len);
    JD_ASSERT(trg != NULL);
    memcpy(trg, data, len);
    str->counter =
        ((str->counter + 1) & JD_PIPE_COUNTER_MASK) | (str->counter & ~JD_PIPE_COUNTER_MASK);

    return JD_PIPE_OK;
}

int jd_opipe_flush(jd_opipe_desc_t *str) {
    if (str->status == ST_OPEN && str->curr_retry == 0 && str->frame.size == 0)
        return JD_PIPE_OK;
    return jd_opipe_check_space(str, 0x100000);
}

int jd_opipe_write_meta(jd_opipe_desc_t *str, const void *data, unsigned len) {
    return jd_opipe_write_ex(str, data, len, JD_PIPE_METADATA_MASK);
}

int jd_opipe_write(jd_opipe_desc_t *str, const void *data, unsigned len) {
    return jd_opipe_write_ex(str, data, len, 0);
}

static int jd_opipe_send_close_pkt(jd_opipe_desc_t *str) {
    str->status = ST_OPEN; // avoid error check in jd_opipe_check_space()
    int r = jd_opipe_write_ex(str, NULL, 0, JD_PIPE_CLOSE_MASK | JD_PIPE_METADATA_MASK);
    if (r == JD_PIPE_OK) {
        str->status = ST_CLOSED_WAITING;
        do_flush(str);
        return JD_PIPE_TRY_AGAIN;
    } else {
        str->status = ST_CLOSED_UNSENT;
        return r;
    }
}

int jd_opipe_close(jd_opipe_desc_t *str) {
    switch (str->status) {
    case ST_DROPPED:
        jd_opipe_unlink(str);
    // fall-through
    case ST_FREE:
        return JD_PIPE_OK;
    case ST_CLOSED_WAITING:
        return JD_PIPE_TRY_AGAIN;
    case ST_OPEN:
    case ST_CLOSED_UNSENT:
        return jd_opipe_send_close_pkt(str);
    default:
        JD_PANIC();
        return -1;
    }
}
