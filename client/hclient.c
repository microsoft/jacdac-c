#include "client_internal.h"
#include "jd_hclient.h"
#include "jd_thr.h"

#if JD_THR_ANY

static void TODO(void) {
    DMESG("not implemented yet");
    JD_PANIC();
}

typedef struct reg_query {
    struct reg_query *next;
    jd_thread_t asker;
    void *dst;
    uint16_t code;
    uint8_t dst_size;
    uint8_t ok;
} reg_query_t;

struct jdc_client {
    struct jdc_client *next;
    jd_mutex_t mutex; // for accessing 'service' etc
    jd_role_t *role;
    jd_device_service_t *service;
    void *userdata;
    reg_query_t *queries;
    jdc_event_cb_t event_cb;
    unsigned write_timeout;
    unsigned read_timeout;
    uint8_t read_flags;
    uint8_t write_flags;
    uint8_t streaming_counter;
};

static jdc_t client_list;
static jd_mutex_t client_mut;

static jd_thread_t callback_thread;

static void jdc_event_handler(void *_state, int event_id, void *arg0, void *arg1);

void jdc_wait_roles_bound(unsigned timeout_ms) {
    TODO();
}

static void callback_worker(void *userdata) {
    void (*cb)(void) = userdata;
    if (cb)
        cb();
    else
        jdc_wait_roles_bound(1000);
    // event queue
}

void jdc_start_threads(void (*init_cb)(void)) {
    jd_thr_init_mutex(&client_mut);
    jd_client_subscribe(jdc_event_handler, NULL);
    jd_thr_start_process_worker();
    callback_thread = jd_thr_start_thread(callback_worker, init_cb);
}

// event_cb can be set to NULL to disable
jdc_t jdc_create(uint32_t service_class, const char *name, jdc_event_cb_t event_cb) {
    jdc_t r = jd_alloc(sizeof(*r));
    jd_thr_init_mutex(&r->mutex);
    r->read_timeout = JDC_TIMEOUT_DEFAULT;
    r->write_timeout = JDC_TIMEOUT_DEFAULT;

    jd_thr_lock(&client_mut);
    r->role = jd_role_alloc(name, service_class);
    r->next = client_list;
    client_list = r;
    jd_thr_unlock(&client_mut);

    return r;
}

void jdc_destroy(jdc_t c) {
    TODO();
}

jdc_t jdc_create_derived(jdc_t parent, jdc_event_cb_t event_cb) {
    TODO();
    return NULL;
}

#pragma region accessors
void jdc_configure_read(jdc_t c, unsigned flags, unsigned timeout_ms) {
    c->read_flags = flags;
    c->read_timeout = timeout_ms;
}

void jdc_configure_write(jdc_t c, unsigned flags, unsigned timeout_ms) {
    c->write_flags = flags;
    c->read_timeout = timeout_ms;
}

void jdc_set_streaming(jdc_t c, bool enabled) {
    c->streaming_counter = enabled ? 1 : 0;
}

void jdc_set_userdata(jdc_t c, void *userdata) {
    c->userdata = userdata;
}

void *jdc_get_userdata(jdc_t c) {
    return c->userdata;
}

const char *jdc_get_name(jdc_t c) {
    return c->role->name;
}

uint32_t jdc_get_service_class(jdc_t c) {
    return c->role->service_class;
}

int jdc_get_binding_info(jdc_t c, uint64_t *device_id, uint8_t *service_index) {
    int r = 0;

    jd_thr_lock(&c->mutex);
    if (c->service) {
        if (device_id)
            *device_id = jd_service_parent(c->service)->device_identifier;
        if (service_index)
            *service_index = c->service->service_index;
    } else {
        if (device_id)
            *device_id = 0;
        if (service_index)
            *service_index = 0;
        r = JDC_STATUS_UNBOUND;
    }
    jd_thr_unlock(&c->mutex);

    return r;
}

bool jdc_is_bound(jdc_t c) {
    return c->service != NULL;
}
#pragma endregion

static jdc_t find_and_lock(jd_role_t *role, jd_device_service_t *serv) {
    jdc_t p;
    jd_thr_lock(&client_mut);
    for (p = client_list; p; p = p->next) {
        if (role && p->role != role)
            continue;
        if (serv && p->service != serv)
            continue;
        jd_thr_lock(&p->mutex);
        break;
    }
    jd_thr_unlock(&client_mut);
    return p;
}

jd_packet_t *jdc_dup_pkt(jd_packet_t *pkt) {
    TODO();
    return NULL;
}

static void jdc_emit_event(jdc_t c, uint16_t code, uint16_t subcode, void *arg) {
    if (!c || !c->event_cb)
        return;
    // add to queue
}

static void jdc_event_handler(void *_state, int event_id, void *arg0, void *arg1) {
    // jd_device_t *d = arg0;
    jd_device_service_t *serv = arg0;
    jd_packet_t *pkt = arg1;
    jd_register_query_t *reg = arg1;
    jd_role_t *role = arg1;

    jdc_t c = NULL;

    if (event_id == JD_CLIENT_EV_SERVICE_PACKET && !jd_is_report(pkt))
        return;

    switch (event_id) {
    case JD_CLIENT_EV_SERVICE_PACKET:
    case JD_CLIENT_EV_SERVICE_REGISTER_CHANGED:
    case JD_CLIENT_EV_SERVICE_REGISTER_NOT_IMPLEMENTED:
        c = find_and_lock(NULL, serv);
        break;
    case JD_CLIENT_EV_ROLE_CHANGED:
        c = find_and_lock(role, NULL);
        break;
    default:
        return;
    }

    if (!c)
        return;

    switch (event_id) {
    case JD_CLIENT_EV_SERVICE_PACKET:
        if (jd_is_event(pkt)) {
            jdc_emit_event(c, JDC_EV_SERVICE_EVENT, jd_event_code(pkt), pkt);
        } else if (jd_is_register_get(pkt)) {
            uint16_t code = JD_REG_CODE(pkt->service_command);
            jdc_emit_event(c, JDC_EV_REG_VALUE, code, pkt);
            if (code == JD_REG_READING)
                jdc_emit_event(c, JDC_EV_READING, code, pkt);
        } else if ((pkt->service_command >> 12) == 0) {
            jdc_emit_event(c, JDC_EV_ACTION_REPORT, pkt->service_command, pkt);
        }
        break;

    case JD_CLIENT_EV_ROLE_CHANGED:
        c->service = serv;
        if (serv)
            jdc_emit_event(c, JDC_EV_BOUND, 0, NULL);
        else
            jdc_emit_event(c, JDC_EV_UNBOUND, 0, NULL);
        break;

    case JD_CLIENT_EV_SERVICE_REGISTER_CHANGED:
    case JD_CLIENT_EV_SERVICE_REGISTER_NOT_IMPLEMENTED: {
        reg_query_t *rest = NULL, *next;
        for (reg_query_t *r = c->queries; r; r = next) {
            next = r->next;
            if (r->code == reg->reg_code) {
                if (event_id == JD_CLIENT_EV_SERVICE_REGISTER_NOT_IMPLEMENTED) {
                    r->ok = 0;
                } else {
                    r->ok = 1;
                    int sz = reg->resp_size;
                    if (sz > r->dst_size)
                        sz = r->dst_size;
                    r->dst_size = sz;
                    memcpy(r->dst, jd_register_data(reg), sz);
                }
                // the asker thread will jd_free() the data
                jd_thr_resume(r->asker);
            } else {
                r->next = rest;
                rest = r;
            }
        }
        c->queries = rest;
    } break;
    }

    jd_thr_unlock(&c->mutex);
}

int jdc_get_register(jdc_t c, uint16_t regcode, void *dst, unsigned size, unsigned refresh_ms) {
    int r = 0;
    jd_thr_lock(&c->mutex);
    if (!c->service) {
        r = JDC_STATUS_UNBOUND;
    } else {
        const jd_register_query_t *q = jd_service_query(c->service, regcode, refresh_ms);
        if (jd_register_not_implemented(q)) {
            r = JDC_STATUS_NOT_IMPL;
        } else {
            TODO();
        }
    }
    jd_thr_unlock(&c->mutex);
    return r;
}

#endif