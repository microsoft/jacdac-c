// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_protocol.h"

#if JD_SIMPLE_ALLOC
static uintptr_t *aptr;
#endif

#if JD_HW_ALLOC
#ifndef JD_STACK_SIZE
#define JD_STACK_SIZE 512
#endif
#define STACK_BASE ((uintptr_t)&_estack)
#define HEAP_BASE ((uintptr_t)&_end)
#define HEAP_END (STACK_BASE - JD_STACK_SIZE)
#define HEAP_SIZE (HEAP_END - HEAP_BASE)

extern uintptr_t _end;
extern uintptr_t _estack;

static uint16_t maxStack;

void jd_alloc_stack_check(void) {
    uintptr_t *ptr = (uintptr_t *)(STACK_BASE - JD_STACK_SIZE);
    while (ptr < (uintptr_t *)STACK_BASE) {
        if (*ptr != 0x33333333)
            break;
        ptr++;
    }
    int sz = STACK_BASE - (uintptr_t)ptr;
    if (sz > maxStack)
        JD_LOG("stk:%d", maxStack = sz);
}

void jd_alloc_init(void) {
    JD_LOG("free:%d", HEAP_END - HEAP_BASE);

    int p = 0;
    int sz = (uintptr_t)&p - HEAP_BASE - 32;
    // seed PRNG with random RAM contents (later we add ADC readings)
    jd_seed_random(jd_hash_fnv1a((void *)HEAP_BASE, sz));
    memset((void *)HEAP_BASE, 0x33, sz);

    jd_alloc_stack_check();

#if JD_SIMPLE_ALLOC
    aptr = (uintptr_t *)HEAP_BASE;
#endif

#if JD_DEVICESCRIPT
    jd_alloc_add_chunk((void*)HEAP_BASE, HEAP_SIZE);
#endif

}
#endif

#if JD_SIMPLE_ALLOC
void *jd_alloc(uint32_t size) {
    // convert size from bytes to words
    size = (size + 3) >> 2;

    jd_alloc_stack_check();
    void *r = aptr;
    aptr += size;
    if ((uintptr_t)aptr > HEAP_END)
        JD_PANIC();
    memset(r, 0x00, size << 2);

    return r;
}

uint32_t jd_available_memory(void) {
    return HEAP_END - (uintptr_t)aptr;
}

void *jd_alloc_emergency_area(uint32_t size) {
    if (size > HEAP_SIZE)
        JD_PANIC();
    return (void *)HEAP_BASE;
}

#endif
