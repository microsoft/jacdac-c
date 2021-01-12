// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_protocol.h"

typedef struct {
    uint32_t timestamp;
    uint8_t service_size;
    uint8_t service_number;
    uint16_t service_command;
    uint8_t data[0];
} ev_t;

struct event_info {
    ev_t *buffer;
    cb_t process;
    uint16_t qptr;
    uint8_t counter;
};
static struct event_info info;

static inline uint32_t ev_size(ev_t *ev) {
    return sizeof(ev_t) + ((ev->service_size + 3) & ~3);
}

static inline ev_t *next_ev(ev_t *ev) {
    return (ev_t *)((uint8_t *)ev + ev_size(ev));
}

static uint16_t next_event_cmd(uint32_t eventid) {
    if (eventid >> 24)
        jd_panic();
    info.counter++;
    return JD_CMD_EVENT_MK(info.counter, eventid);
}

static void ev_shift(void) {
    int words = ev_size(info.buffer) >> 2;
    uint32_t *dst = &info.buffer->timestamp;
    uint32_t *src = dst + words;
    info.qptr -= words << 2;
    while (words--)
        *dst++ = *src++;
}

static void do_process_event_queue(void) {
    // if info.process != NULL, then info.buffer has been initialized already
    ev_t *ev = info.buffer;
    while ((uint8_t *)ev - (uint8_t *)info.buffer < info.qptr) {
        if (ev->service_number == 0xff) {
            if (ev == info.buffer) {
                ev_shift();
                continue;
            }
        } else if (in_past(ev->timestamp)) {
            if (jd_send(ev->service_number & 0x7f, ev->service_command, ev->data,
                        ev->service_size) != 0)
                break;
            if (ev->service_number & 0x80) {
                if (ev == info.buffer) {
                    ev_shift();
                    continue;
                } else {
                    ev->service_number = 0xff; // not sure this can happen
                }
            } else {
                ev->timestamp += 50 * 1000;
                ev->service_number |= 0x80;
            }
        }

        ev = next_ev(ev);
    }
}

static void ev_init(void) {
    if (info.buffer)
        return;
    info.buffer = jd_alloc(JD_EVENT_QUEUE_SIZE);
    // this is only linked in, when the code uses event sending functions
    info.process = do_process_event_queue;
}

void jd_process_event_queue(void) {
    if (info.process)
        info.process();
}

void jd_send_event_ext(srv_t *srv, uint32_t eventid, const void *data, uint32_t data_bytes) {
    srv_common_t *state = (srv_common_t *)srv;

    uint16_t cmd = next_event_cmd(eventid);
    jd_send(state->service_number, cmd, data, data_bytes);

    ev_init();

    int reqlen = data_bytes + sizeof(ev_t);
    if (reqlen > JD_EVENT_QUEUE_SIZE)
        return; // too long to queue; shouldn't happen
    while (JD_EVENT_QUEUE_SIZE - info.qptr < (int)data_bytes)
        ev_shift();

    ev_t *ev = (ev_t *)((uint8_t *)info.buffer + info.qptr);
    ev->service_size = data_bytes;
    ev->service_command = cmd;
    ev->service_number = state->service_number;
    // no randomization; it's somewhat often to generate multiple events in the same tick
    // they will this way get the same re-transmission time, and thus be packed in one frame
    // on both re-transmissions
    ev->timestamp = now + 20 * 1000;
    info.qptr += ev_size(ev);
}
