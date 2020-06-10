#ifndef __JD_ALLOC_H
#define __JD_ALLOC_H

#include "jd_config.h"

// This file outlines the APIs for allocator implementations.
// Allocator implementations are only required if an app or transmission/reception requires dynamic allocation
// Implementors can map these APIs onto custom malloc/free implementations
// A dummy implementation can be optionally compiled in, and specific function overriden (implementation/dummy/alloc.c)

/**
 * This function is invoked by a call to jd_init.
 **/
void jd_alloc_init(void);

/**
 * Any dynamic allocations made by the jacdac-c library will use this function.
 **/
void* jd_alloc(uint32_t size);

/**
 *  Any corresponding free calls made by the jacdac-c library will use this function.
 **/
void jd_free(void* ptr);

void jd_alloc_collision_check(void);
void *jd_alloc_emergency_area(uint32_t size);

#endif