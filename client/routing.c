#include "jd_client.h"

static uint32_t next_gc;
static uint8_t verbose_log;

#define EXPIRES_USEC (2000 * 1000)
jd_device_t *jd_devices;

// https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
// this is not jd_hash_fnv1a()!
static uint32_t hash_fnv1(const void *data, unsigned len) {
    const uint8_t *d = (const uint8_t *)data;
    uint32_t h = 0x811c9dc5;
    while (len--) {
        h = (h * 0x1000193) ^ *d++;
    }
    return h;
}

static uint32_t mkcd_hash(const void *buf, size_t length, int bits) {
    if (bits < 1) {
        return 0;
    }

    uint32_t h = hash_fnv1(buf, length);

    if (bits >= 32) {
        return h;
    } else {
        return (h ^ (h >> bits)) & ((1 << bits) - 1);
    }
}

void jd_device_short_id(char short_id[5], uint64_t long_id) {
    uint32_t h = mkcd_hash(&long_id, sizeof(long_id), 30);
    short_id[0] = 'A' + (h % 26);
    short_id[1] = 'A' + ((h / 26) % 26);
    short_id[2] = '0' + ((h / (26 * 26)) % 10);
    short_id[3] = '0' + ((h / (26 * 26 * 10)) % 10);
    short_id[4] = 0;
}

void jd_client_log_event(int event_id, void *arg0, void *arg1) {
    jd_device_t *d = arg0;
    jd_packet_t *pkt = arg1;

    switch (event_id) {
    case JD_CLIENT_EV_DEVICE_CREATED:
        DMESG("device created: %s", d->short_id);
        break;
    case JD_CLIENT_EV_DEVICE_DESTROYED:
        DMESG("device destroyed: %s", d->short_id);
        break;
    case JD_CLIENT_EV_DEVICE_RESET:
        DMESG("device reset: %s", d->short_id);
        break;
    case JD_CLIENT_EV_DEVICE_PACKET:
        if (verbose_log)
            DMESG("device %s - pkt cmd=%x", d->short_id, pkt->service_command);
        break;
    case JD_CLIENT_EV_NON_DEVICE_PACKET:
        // arg0 == NULL here
        DMESG("unbound pkt cmd=%x", pkt->service_command);
        break;
    case JD_CLIENT_EV_BROADCAST_PACKET:
        // arg0 == NULL here
        if (verbose_log)
            DMESG("brd cmd=%x", pkt->service_command);
        break;
    }
}

void jd_client_emit_event(int event_id, void *arg0, void *arg1) {
    jd_client_log_event(event_id, arg0, arg1);
    // TODO
}

static jd_device_t *jd_device_alloc(jd_packet_t *announce) {
    int num_services = announce->service_size >> 2;
    jd_device_t *d = jd_alloc(sizeof(jd_device_t) + (num_services * 4));
    memset(d, 0, sizeof(jd_device_t));
    memcpy(d->services, announce->data, num_services * 4);
    d->num_services = num_services;
    d->device_identifier = announce->device_identifier;
    d->_expires = now + EXPIRES_USEC;
    jd_device_short_id(d->short_id, d->device_identifier);
    d->next = jd_devices;
    jd_devices = d;
    jd_client_emit_event(JD_CLIENT_EV_DEVICE_CREATED, d, announce);
    return d;
}

static void jd_device_free(jd_device_t *d) {
    jd_client_emit_event(JD_CLIENT_EV_DEVICE_DESTROYED, d, NULL);
    jd_free(d);
}

static void jd_device_unlink(jd_device_t *d) {
    if (d == jd_devices) {
        jd_devices = d->next;
    } else {
        for (jd_device_t *p = jd_devices; p; p = p->next) {
            if (p->next == d) {
                p->next = d->next;
                break;
            }
        }
    }
}

static void jd_device_gc(void) {
    jd_device_t *p, *q;
    while (jd_devices && in_past(jd_devices->_expires)) {
        p = jd_devices;
        jd_devices = p->next;
        jd_device_free(p);
    }
    for (p = jd_devices; p && p->next; p = p->next) {
        if (in_past(p->next->_expires)) {
            q = p->next;
            p->next = q->next;
            jd_device_free(q);
        }
    }
}

jd_device_t *jd_device_lookup(uint64_t device_identifier) {
    jd_device_t *p = jd_devices;
    while (p) {
        if (p->device_identifier == device_identifier)
            return p;
        p = p->next;
    }
    return NULL;
}

void jd_client_process(void) {
    if (jd_should_sample(&next_gc, 300 * 1000)) {
        jd_device_gc();
    }
}

void jd_client_handle_packet(jd_packet_t *pkt) {
    if (pkt->flags & JD_FRAME_FLAG_IDENTIFIER_IS_SERVICE_CLASS) {
        jd_client_emit_event(JD_CLIENT_EV_BROADCAST_PACKET, NULL, pkt);
        return;
    }

    jd_device_t *d = jd_device_lookup(pkt->device_identifier);
    if (pkt->flags & JD_FRAME_FLAG_COMMAND) {

    } else {
        // report
        if (pkt->service_number == 0 && pkt->service_command == 0) {
            if (!d)
                d = jd_device_alloc(pkt);
            uint32_t new_serv0 = *(uint32_t *)pkt->data;
            if (d->services[0] != new_serv0) {
                if ((d->services[0] & JD_CONTROL_ANNOUNCE_FLAGS_RESTART_COUNTER_STEADY) >
                    (new_serv0 & JD_CONTROL_ANNOUNCE_FLAGS_RESTART_COUNTER_STEADY)) {
                    jd_client_emit_event(JD_CLIENT_EV_DEVICE_RESET, d, pkt);
                    jd_device_unlink(d);
                    jd_device_free(d);
                    d = jd_device_alloc(pkt);
                } else {
                    d->services[0] = new_serv0;
                }
            }
            d->_expires = now + EXPIRES_USEC;
        }
    }

    if (d)
        jd_client_emit_event(JD_CLIENT_EV_DEVICE_PACKET, d, pkt);
    else
        jd_client_emit_event(JD_CLIENT_EV_NON_DEVICE_PACKET, d, pkt);
}
