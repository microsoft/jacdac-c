#ifndef __JD_ALLOC_H
#define __JD_ALLOC_H

#include "jd_config.h"

// only required if app, or queuing mechanisms require dynamic allocation
void jd_alloc_init(void);
void *jd_alloc(uint32_t size);
void jd_alloc_stack_check(void);
void *jd_alloc_emergency_area(uint32_t size);

#endif