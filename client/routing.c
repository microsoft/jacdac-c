#include "client_internal.h"

#define EVENT_CHECKING 1

typedef struct listener {
    struct listener *next;
    jd_client_event_handler_t handler;
    void *userdata;
} listener_t;
static listener_t *jd_client_listeners;

static uint32_t next_gc;
static uint8_t verbose_log = 0;

#define EXPIRES_USEC (2000 * 1000)
jd_device_t *jd_devices;

#if EVENT_CHECKING
static uint8_t event_scope;
#define EVENT_ENTER()                                                                              \
    do {                                                                                           \
        JD_ASSERT(event_scope == 0);                                                               \
        event_scope = 1;                                                                           \
    } while (0)
#define EVENT_LEAVE()                                                                              \
    do {                                                                                           \
        JD_ASSERT(event_scope == 1);                                                               \
        event_scope = 0;                                                                           \
    } while (0)
#define EVENT_CHECK() JD_ASSERT(event_scope == 1)
#else
#define EVENT_ENTER() ((void)0)
#define EVENT_LEAVE() ((void)0)
#define EVENT_CHECK() ((void)0)
#endif

void jd_client_log_event(int event_id, void *arg0, void *arg1) {
    jd_device_t *d = arg0;
    jd_device_service_t *serv = arg0;
    jd_packet_t *pkt = arg1;
    jd_register_query_t *reg = arg1;

    switch (event_id) {
    case JD_CLIENT_EV_PROCESS:
        // this is called very often, so don't print anything
        break;
    case JD_CLIENT_EV_DEVICE_CREATED:
        DMESG("device created: %s", d->short_id);
        break;
    case JD_CLIENT_EV_DEVICE_DESTROYED:
        DMESG("device destroyed: %s", d->short_id);
        break;
    case JD_CLIENT_EV_DEVICE_RESET:
        DMESG("device reset: %s", d->short_id);
        break;
    case JD_CLIENT_EV_REPEATED_EVENT_PACKET:
        if (verbose_log) {
            DMESG("serv %s/%d[%x] - repeated event cmd=%x", jd_service_parent(serv)->short_id,
                  serv->service_index, (unsigned)serv->service_class, pkt->service_command);
        }
        break;
    case JD_CLIENT_EV_SERVICE_PACKET:
        if (verbose_log) {
            DMESG("serv %s/%d[0x%x] - pkt cmd=%x sz=%d", jd_service_parent(serv)->short_id,
                  serv->service_index, (unsigned)serv->service_class, pkt->service_command,
                  pkt->service_size);
        }
        break;
    case JD_CLIENT_EV_NON_SERVICE_PACKET:
        if (verbose_log)
            DMESG("unbound pkt d=%s/%d cmd=%x sz=%d", d ? d->short_id : "n/a", pkt->service_index,
                  pkt->service_command, pkt->service_size);
        break;
    case JD_CLIENT_EV_BROADCAST_PACKET:
        // arg0 == NULL here
        if (verbose_log)
            DMESG("brd cmd=%x", pkt->service_command);
        break;
    case JD_CLIENT_EV_SERVICE_REGISTER_CHANGED:
        DMESG("serv %s/%d reg chg %x [sz=%d]", jd_service_parent(serv)->short_id,
              serv->service_index, reg->reg_code, reg->resp_size);
        break;
    }
}

void app_client_event_handler(int event_id, void *arg0, void *arg1) {
    // cause linker error if this is defined - use jd_client_subscribe()
}

void jd_client_subscribe(jd_client_event_handler_t handler, void *userdata) {
    listener_t *l = jd_alloc(sizeof(*l));
    l->next = jd_client_listeners;
    l->userdata = userdata;
    l->handler = handler;
    jd_client_listeners = l;
}

void jd_client_emit_event(int event_id, void *arg0, void *arg1) {
    EVENT_CHECK();
    EVENT_LEAVE();
    jd_client_log_event(event_id, arg0, arg1);
    for (listener_t *l = jd_client_listeners; l; l = l->next) {
        l->handler(l->userdata, event_id, arg0, arg1);
    }
    EVENT_ENTER();
}

void rolemgr_role_changed(jd_role_t *role) {
    EVENT_ENTER();
    jd_client_emit_event(JD_CLIENT_EV_ROLE_CHANGED, role->service, role);
    EVENT_LEAVE();
}

static bool fits_at(jd_device_t *existing, jd_device_t *fresh) {
    // sort self-device as first
    return !existing || fresh->device_identifier == jd_device_id() ||
           memcmp(&fresh->device_identifier, &existing->device_identifier,
                  sizeof(existing->device_identifier)) < 0;
}

static jd_device_t *jd_device_alloc(jd_packet_t *announce) {
    int num_services = announce->service_size >> 2;
    int sz = sizeof(jd_device_t) + (num_services * sizeof(jd_device_service_t));
    jd_device_t *d = jd_alloc(sz);
    memset(d, 0, sz);
    d->num_services = num_services;
    d->device_identifier = announce->device_identifier;
    d->_expires = now + EXPIRES_USEC;
    d->_event_counter = 0xff;
    jd_device_short_id(d->short_id, d->device_identifier);

    uint32_t *services = (uint32_t *)announce->data;
    d->announce_flags = services[0] & 0xffff;
    for (int i = 0; i < num_services; ++i) {
        d->services[i].service_class = i == 0 ? 0 : services[i];
        d->services[i].service_index = i;
    }

    if (fits_at(jd_devices, d)) {
        d->next = jd_devices;
        jd_devices = d;
    } else {
        for (jd_device_t *q = jd_devices; q; q = q->next) {
            if (fits_at(q->next, d)) {
                d->next = q->next;
                q->next = d;
                break;
            }
        }
    }

    jd_client_emit_event(JD_CLIENT_EV_DEVICE_CREATED, d, announce);
    return d;
}

void jd_device_clear_queries(jd_device_t *d, uint8_t service_idx) {
    jd_register_query_t *tmp, *q = d->_queries, *prev = NULL;
    while (q) {
        if (service_idx == 0xff || q->service_index == service_idx) {
            if (q == d->_queries)
                d->_queries = q->next;
            else
                prev->next = q->next;
            tmp = q;
            q = q->next;
            if (tmp->resp_size > JD_REGISTER_QUERY_MAX_INLINE)
                jd_free(tmp->value.buffer);
            jd_free(tmp);
        } else {
            prev = q;
            q = q->next;
        }
    }
}

static void jd_device_free(jd_device_t *d) {
    EVENT_LEAVE();
    rolemgr_device_destroyed(d);
    EVENT_ENTER();

    jd_client_emit_event(JD_CLIENT_EV_DEVICE_DESTROYED, d, NULL);
    jd_device_clear_queries(d, 0xff);
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

jd_device_service_t *jd_device_lookup_service(jd_device_t *dev, uint32_t service_class) {
    for (unsigned i = 0; i < dev->num_services; ++i)
        if (jd_device_get_service(dev, i)->service_class == service_class)
            return jd_device_get_service(dev, i);
    return NULL;
}

int jd_service_send_cmd(jd_device_service_t *serv, uint16_t service_command, const void *data,
                        size_t datasize) {
    jd_packet_t *pkt = jd_alloc(JD_SERIAL_FULL_HEADER_SIZE + datasize);
    pkt->flags = JD_FRAME_FLAG_COMMAND;
    pkt->device_identifier = jd_service_parent(serv)->device_identifier;
    pkt->service_index = serv->service_index;
    pkt->service_command = service_command;
    pkt->service_size = datasize;
    memcpy(pkt->data, data, datasize);
    int r = jd_send_pkt(pkt);
    jd_free(pkt);
    return r;
}

static jd_register_query_t *jd_service_query_lookup(jd_device_service_t *serv, int reg_code) {
    jd_register_query_t *q = jd_service_parent(serv)->_queries;
    int idx = serv->service_index;
    while (q) {
        if (q->reg_code == reg_code && (q->service_index & JD_SERVICE_INDEX_MASK) == idx)
            return q;
        q = q->next;
    }
    return NULL;
}

const jd_register_query_t *jd_service_query(jd_device_service_t *serv, int reg_code,
                                            int refresh_ms) {
    jd_register_query_t *q = jd_service_query_lookup(serv, reg_code);
    if (!q) {
        q = jd_alloc(sizeof(jd_register_query_t));
        memset(q, 0, sizeof(*q));
        jd_device_t *dev = jd_service_parent(serv);
        q->reg_code = reg_code;
        q->service_index = serv->service_index;
        q->next = dev->_queries;
        dev->_queries = q;
    }
    if (!jd_register_not_implemented(q) &&
        (!q->last_query_ms || (refresh_ms && in_past_ms(q->last_query_ms + refresh_ms)))) {
        jd_service_send_cmd(serv, JD_GET(reg_code), NULL, 0);
        q->last_query_ms = now_ms;
    }
    return q;
}

void jd_client_process(void) {
    EVENT_ENTER();
    if (jd_should_sample(&next_gc, 300 * 1000)) {
        jd_device_gc();
    }
    jd_client_emit_event(JD_CLIENT_EV_PROCESS, NULL, NULL);
    EVENT_LEAVE();
}

static void handle_register(jd_device_service_t *serv, jd_packet_t *pkt) {
    if (pkt->service_size >= 4 && pkt->service_command == JD_CMD_COMMAND_NOT_IMPLEMENTED) {
        uint16_t cmd = *(uint16_t *)pkt->data;
        if (JD_IS_GET(cmd)) {
            jd_register_query_t *q = jd_service_query_lookup(serv, JD_REG_CODE(cmd));
            if (q && !jd_register_not_implemented(q)) {
                q->service_index |= 0x80;
                jd_client_emit_event(JD_CLIENT_EV_SERVICE_REGISTER_NOT_IMPLEMENTED, serv, q);
            }
        }
    } else if (JD_IS_GET(pkt->service_command)) {
        jd_register_query_t *q = jd_service_query_lookup(serv, JD_REG_CODE(pkt->service_command));
        if (q) {
            int chg = 0;
            if (q->resp_size != pkt->service_size ||
                memcmp(pkt->data, jd_register_data(q), q->resp_size))
                chg = 1;

            if (pkt->service_size > JD_REGISTER_QUERY_MAX_INLINE) {
                if (q->resp_size != pkt->service_size) {
                    if (q->resp_size > JD_REGISTER_QUERY_MAX_INLINE)
                        jd_free(q->value.buffer);
                    q->value.buffer = jd_alloc(pkt->service_size);
                }
            }
            q->resp_size = pkt->service_size;
            memcpy((void *)jd_register_data(q), pkt->data, q->resp_size);
            if (chg)
                jd_client_emit_event(JD_CLIENT_EV_SERVICE_REGISTER_CHANGED, serv, q);
        }
    }
}

void jd_client_handle_packet(jd_packet_t *pkt) {
    EVENT_ENTER();

    if (pkt->flags & JD_FRAME_FLAG_IDENTIFIER_IS_SERVICE_CLASS) {
        jd_client_emit_event(JD_CLIENT_EV_BROADCAST_PACKET, NULL, pkt);
        EVENT_LEAVE();
        return;
    }

    jd_device_t *dev = jd_device_lookup(pkt->device_identifier);
    jd_device_service_t *serv = NULL;
    int ev_code = JD_CLIENT_EV_SERVICE_PACKET;

    if (dev) {
        int idx = pkt->service_index & JD_SERVICE_INDEX_MASK;
        if (idx < dev->num_services) {
            serv = &dev->services[idx];
        }
    }

    if (pkt->flags & JD_FRAME_FLAG_COMMAND) {

    } else {
        // report
        if (pkt->service_index == 0 && pkt->service_command == 0) {
            if (!dev)
                dev = jd_device_alloc(pkt);
            uint16_t new_flags = *(uint16_t *)pkt->data;
            if (dev->announce_flags != new_flags) {
                if ((dev->announce_flags & JD_CONTROL_ANNOUNCE_FLAGS_RESTART_COUNTER_STEADY) >
                    (new_flags & JD_CONTROL_ANNOUNCE_FLAGS_RESTART_COUNTER_STEADY)) {
                    // DMESG("rst %x -> %x", dev->announce_flags, new_flags);
                    jd_client_emit_event(JD_CLIENT_EV_DEVICE_RESET, dev, pkt);
                    jd_device_unlink(dev);
                    jd_device_free(dev);
                    dev = jd_device_alloc(pkt);
                } else {
                    dev->announce_flags = new_flags;
                }
            }
            serv = &dev->services[0]; // re-assign in case we (re)allocated dev
            dev->_expires = now + EXPIRES_USEC;
        }

        if (serv) {
            if (jd_is_event(pkt)) {
                int pec = (pkt->service_command >> JD_CMD_EVENT_COUNTER_SHIFT) &
                          JD_CMD_EVENT_COUNTER_MASK;
                int ec = dev->_event_counter;
                if (ec == 0xff) {
                    ec = pec - 1;
                }
                ec++;

                // how many packets ahead and behind current are we?
                int ahead = (pec - ec) & JD_CMD_EVENT_COUNTER_MASK;
                int behind = (ec - pec) & JD_CMD_EVENT_COUNTER_MASK;
                // ahead == behind == 0 is the usual case, otherwise
                // behind < 60 means this is an old event (or retransmission of something we already
                // processed) ahead < 5 means we missed at most 5 events, so we ignore this one and
                // rely on retransmission of the missed events, and then eventually the current
                // event
                if (ahead > 0 && (behind < 60 || ahead < 5)) {
                    ev_code = JD_CLIENT_EV_REPEATED_EVENT_PACKET;
                } else {
                    // we got our event
                    dev->_event_counter = pec;
                }
            }

            handle_register(serv, pkt);
        }
    }

    if (serv)
        jd_client_emit_event(ev_code, serv, pkt);
    else
        jd_client_emit_event(JD_CLIENT_EV_NON_SERVICE_PACKET, dev, pkt);
    EVENT_LEAVE();
}

void jd_pkt_setup_broadcast(jd_packet_t *dst, uint32_t service_class, uint16_t service_command) {
    memset(dst, 0, sizeof(*dst));
    dst->flags = JD_FRAME_FLAG_BROADCAST | JD_FRAME_FLAG_COMMAND;
    dst->device_identifier = JD_DEVICE_IDENTIFIER_BROADCAST_MARK | service_class;
    dst->service_command = service_command;
    dst->service_index = JD_SERVICE_INDEX_BROADCAST;
}
