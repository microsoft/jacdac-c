#include "jd_thr.h"

#if JD_THR_PTHREAD

#define CHK JD_CHK

#pragma region mutexes
static pthread_mutexattr_t prio_inherit_attr;
static bool thr_inited;

int jd_thr_init_mutex(jd_mutex_t *mutex) {
    JD_ASSERT(thr_inited);
    CHK(pthread_mutex_init(mutex, &prio_inherit_attr));
    return 0;
}

void jd_thr_destroy_mutex(jd_mutex_t *mutex) {
    CHK(pthread_mutex_destroy(mutex));
}

void jd_thr_lock(jd_mutex_t *mutex) {
    CHK(pthread_mutex_lock(mutex));
}

void jd_thr_unlock(jd_mutex_t *mutex) {
    CHK(pthread_mutex_unlock(mutex));
}
#pragma endregion

jd_thread_t jd_thr_self(void) {
    return pthread_self();
}

#pragma region suspend / resume thread
typedef struct thread_cond {
    struct thread_cond *next;
    jd_thread_t thread;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    bool suspended;
} thread_cond_t;
static thread_cond_t *first_thread;
static pthread_mutex_t thread_mux = PTHREAD_MUTEX_INITIALIZER;

static thread_cond_t *lookup_thread_cond(jd_thread_t thr, bool create) {
    thread_cond_t *p;
    jd_thr_lock(&thread_mux);
    for (p = first_thread; p; p = p->next) {
        if (p->thread == thr)
            break;
    }
    if (p == NULL && create) {
        p = jd_alloc(sizeof(*p));
        jd_thr_init_mutex(&p->lock);
        pthread_cond_init(&p->cond, NULL);
        p->thread = thr;
        p->next = first_thread;
        first_thread = p;
    }
    jd_thr_unlock(&thread_mux);
    return p;
}

// note that these two are way simpler in an RTOS - it typically let's one just suspend/resume
// threads
void jd_thr_suspend_self(void) {
    thread_cond_t *c = lookup_thread_cond(pthread_self(), true);
    jd_thr_lock(&c->lock);
    JD_ASSERT(!c->suspended);
    c->suspended = true;
    do {
        CHK(pthread_cond_wait(&c->cond, &c->lock));
    } while (c->suspended);
    jd_thr_unlock(&c->lock);
}

void jd_thr_resume(jd_thread_t t) {
    thread_cond_t *c = lookup_thread_cond(pthread_self(), false);
    JD_ASSERT(c != NULL);
    jd_thr_lock(&c->lock);
    JD_ASSERT(c->suspended);
    c->suspended = false;
    CHK(pthread_cond_signal(&c->cond));
    jd_thr_unlock(&c->lock);
}
#pragma endregion

void jd_thr_init(void) {
    JD_ASSERT(!thr_inited);
    thr_inited = true;
    CHK(pthread_mutexattr_init(&prio_inherit_attr));
    CHK(pthread_mutexattr_setprotocol(&prio_inherit_attr, PTHREAD_PRIO_INHERIT));
}

static jd_thread_t process_thread;
static pthread_mutex_t process_mux = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t process_cond = PTHREAD_COND_INITIALIZER;
static bool process_pending;

void jd_thr_wake_main(void) {
    jd_thr_lock(&process_mux);
    process_pending = true;
    pthread_cond_signal(&process_cond);
    jd_thr_unlock(&process_mux);
}

static void process_worker(void *userdata) {
    for (;;) {
        jd_thr_lock(&process_mux);
        if (!process_pending) {
            struct timespec tenms = {.tv_sec = 0, .tv_nsec = 10 * 1000000};
            pthread_cond_timedwait(&process_cond, &process_mux, &tenms);
        }
        process_pending = false;
        jd_thr_unlock(&process_mux);

        jd_process_everything();
    }
}

void jd_thr_start_process_worker(void) {
    JD_ASSERT(process_thread == 0);
    process_thread = jd_thr_start_thread(process_worker, NULL);
}

jd_thread_t jd_thr_start_thread(void (*worker)(void *userdata), void *userdata) {
    jd_thread_t t;

    CHK(pthread_create(&t, NULL, (void *(*)(void *))worker, userdata));
    CHK(pthread_detach(t));

    return t;
}

#endif
