// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef JD_DMESG_H
#define JD_DMESG_H

#include "jd_config.h"

#if JD_DMESG_BUFFER_SIZE > 0

struct CodalLogStore {
    volatile uint32_t ptr;
    char buffer[JD_DMESG_BUFFER_SIZE];
};
extern struct CodalLogStore codalLogStore;

__attribute__((format(printf, 1, 2))) void jd_dmesg(const char *format, ...);
void jd_vdmesg(const char *format, va_list ap);
void jd_dmesg_write(const void *data, unsigned len);

// read-out data from dmesg buffer
unsigned jd_dmesg_read(void *dst, unsigned space, uint32_t *state);

#ifndef DMESG
#define DMESG jd_dmesg
#endif

#else

#ifndef DMESG
#define DMESG(...) ((void)0)
#endif

#endif

#ifndef JD_LOG
#define JD_LOG DMESG
#endif

#endif
