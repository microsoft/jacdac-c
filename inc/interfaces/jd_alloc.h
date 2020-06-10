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

/**
 * Called by applications to check that the stack has not collided with the heap.
 **/
void jd_alloc_stack_check(void);

#endif