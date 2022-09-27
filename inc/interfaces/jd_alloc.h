// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef __JD_ALLOC_H
#define __JD_ALLOC_H

#include "jd_config.h"

// This file outlines the APIs for allocator implementations.
// Allocator implementations are only required if an app or transmission/reception requires dynamic
// allocation Implementors can map these APIs onto custom malloc/free implementations A dummy
// implementation can be optionally compiled in, and specific function overriden
// (implementation/dummy/alloc.c)

/**
 * This function is invoked by a call to jd_init.
 **/
void jd_alloc_init(void);

#if JD_JACSCRIPT && JD_HW_ALLOC
void jd_alloc_add_chunk(void *start, unsigned size);
#endif

/**
 * Any dynamic allocations made by the jacdac-c library will use this function.
 * It zeroes-out the memory and never returns NULL.
 **/
void *jd_alloc(uint32_t size);

/**
 * Return the amount of memory left to allocated.
 */
uint32_t jd_available_memory(void);

/**
 *  Any corresponding free calls made by the jacdac-c library will use this function.
 *
 *  Note: this is never used.
 **/
void jd_free(void *ptr);

void jd_alloc_stack_check(void);
void *jd_alloc_emergency_area(uint32_t size);

#endif