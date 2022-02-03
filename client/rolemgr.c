// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "services/jd_services.h"
#include "jd_client.h"
#include "jacdac/dist/c/rolemanager.h"

#define AUTOBIND_MS 980

typedef struct role {
    struct role *next;
    uint32_t service_class;
    const char *name;
    jd_device_service_t *serv;
} role_t;

struct srv_state {
    SRV_COMMON;
    uint8_t auto_bind_enabled;
    uint8_t all_roles_allocated;
    uint8_t changed;
    role_t *roles;
    uint32_t next_autobind;
    uint32_t changed_timeout;
};

REG_DEFINITION(                                      //
    rolemgr_regs,                                    //
    REG_SRV_COMMON,                                  //
    REG_U8(JD_ROLE_MANAGER_REG_AUTO_BIND),           //
    REG_U8(JD_ROLE_MANAGER_REG_ALL_ROLES_ALLOCATED), //
)

static srv_t *_state;

static void rolemgr_changed(srv_t *state) {
    state->changed = 1;
    state->all_roles_allocated = 1;
    for (role_t *r = state->roles; r; r = r->next) {
        if (!r->serv) {
            state->all_roles_allocated = 0;
            break;
        }
    }
}

static void rolemgr_autobind(srv_t *state) {
    // TODO
}

static role_t *rolemgr_lookup(srv_t *state, const char *name) {
    // first a quick pass
    for (role_t *r = state->roles; r; r = r->next) {
        if (r->name == name)
            return r;
    }
    // next check by name
    for (role_t *r = state->roles; r; r = r->next) {
        if (strcmp(r->name, name) == 0)
            return r;
    }
    return NULL;
}

void rolemgr_process(srv_t *state) {
    if (jd_should_sample(&state->next_autobind, AUTOBIND_MS * 1000)) {
        rolemgr_autobind(state);
    }
    if (jd_should_sample(&state->changed_timeout, 50 * 1000)) {
        if (state->changed) {
            state->changed = 0;
            jd_send_event(state, JD_EV_CHANGE);
        }
    }
}

static void rolemgr_set_role(srv_t *state, jd_packet_t *pkt) {
    jd_role_manager_set_role_t *cmd = (jd_role_manager_set_role_t *)pkt->data;
    int name_len = pkt->service_size - sizeof(*cmd);
    for (role_t *r = state->roles; r; r = r->next) {
        if (memcmp(r->name, cmd->role, name_len) == 0) {
            if (cmd->device_id == 0) {
                r->serv = NULL;
            } else {
                jd_device_service_t *serv =
                    jd_device_get_service(jd_device_lookup(cmd->device_id), cmd->service_idx);
                if (serv)
                    r->serv = serv;
            }
            rolemgr_changed(state);
            break;
        }
    }
}

void rolemgr_handle_packet(srv_t *state, jd_packet_t *pkt) {
    switch (pkt->service_command) {
    case JD_ROLE_MANAGER_CMD_CLEAR_ALL_ROLES:
        for (role_t *r = state->roles; r; r = r->next)
            r->serv = NULL;
        rolemgr_changed(state);
        break;

    case JD_ROLE_MANAGER_CMD_SET_ROLE:
        rolemgr_set_role(state, pkt);
        break;

    case JD_ROLE_MANAGER_CMD_LIST_ROLES: // TODO

    default:
        service_handle_register_final(state, pkt, rolemgr_regs);
        break;
    }
}

SRV_DEF(rolemgr, JD_SERVICE_CLASS_ROLE_MANAGER);

void rolemgr_init(void) {
    SRV_ALLOC(rolemgr);
    _state = state;
    // wait with first autobind
    state->next_autobind = now + AUTOBIND_MS * 1000;
    rolemgr_changed(state);
}

void rolemgr_client_event(int event_id, void *arg0, void *arg1) {
    jd_device_t *dev = arg0;
    // jd_device_service_t *serv = arg0;
    // jd_packet_t *pkt = arg1;
    // jd_register_query_t *reg = arg1;

    srv_t *state = _state;
    if (!state)
        return;

    switch (event_id) {
    case JD_CLIENT_EV_DEVICE_DESTROYED: {
        int numdel = 0;
        for (role_t *r = state->roles; r; r = r->next) {
            if (r->serv && jd_service_parent(r->serv) == dev) {
                r->serv = NULL;
                numdel++;
            }
        }
        if (numdel)
            rolemgr_changed(state);
    } break;
    }
}

jd_device_service_t *rolemgr_get_binding(const char *name) {
    srv_t *state = _state;
    role_t *r = rolemgr_lookup(state, name);
    if (r)
        return r->serv;
    return NULL;
}

void rolemgr_add_role(const char *name, uint32_t service_class) {
    srv_t *state = _state;
    role_t *r = rolemgr_lookup(state, name);
    if (!r) {
        r = jd_alloc0(sizeof(struct role));
        r->next = state->roles;
        state->roles = r;
    }
    r->name = name;
    if (r->service_class != service_class) {
        r->service_class = service_class;
        r->serv = NULL;
    }
    rolemgr_changed(state);
}
