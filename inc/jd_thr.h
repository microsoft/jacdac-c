// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include "jd_protocol.h"

#if JD_THR_ANY

#if JD_THR_PTHREAD
#include <pthread.h>
typedef pthread_mutex_t jd_mutex_t;
typedef pthread_t jd_thread_t;

#elif JD_THR_AZURE_RTOS
#error "TODO"

#elif JD_THR_FREE_RTOS
#error "TODO"

#else
#error "no threading impl. defined"
#endif


// jd_mutex_t can be a struct
// jd_thread_t must be a pointer; it will be compared using '=='

void jd_thr_init(void);

void jd_thr_start_process_worker(void);
void jd_thr_wake_main(void);

jd_thread_t jd_thr_start_thread(void (*worker)(void *userdata), void *userdata);

// priority inheritance should be enabled if available
int jd_thr_init_mutex(jd_mutex_t *mutex);
void jd_thr_destroy_mutex(jd_mutex_t *mutex);
void jd_thr_lock(jd_mutex_t *mutex);
void jd_thr_unlock(jd_mutex_t *mutex);

jd_thread_t jd_thr_self(void);
void jd_thr_suspend_self(void);
void jd_thr_resume(jd_thread_t t);

#endif