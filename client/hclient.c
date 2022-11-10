#include "client_internal.h"
#include "jd_hclient.h"
#include "jd_thr.h"

struct jdc_client {
    struct jd_hclient *next;
    jd_mutex_t mutex; // for accessing 'service'
    uint32_t service_class;
    struct jd_device_service *service;
};

static jd_thread_t callback_thread;

static void callback_worker(void *userdata) {
    void (*cb)(void) = userdata;
    if (cb)
        cb();
    else
        jdc_wait_roles_bound(1000);
    // event queue
}

void jdc_start_threads(void (*init_cb)(void)) {
    jd_thr_start_process_worker();
    callback_thread = jd_thr_start_thread(callback_worker, init_cb);
}


