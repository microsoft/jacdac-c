// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_protocol.h"

#define STACK_SIZE 512
#define STACK_BASE ((uint32_t)&_estack)
#define HEAP_BASE ((uint32_t)&_end)
#define HEAP_END (STACK_BASE - STACK_SIZE)
#define HEAP_SIZE (HEAP_END - HEAP_BASE)

extern uint32_t _end;
extern uint32_t _estack;

static uint32_t *aptr;
static uint16_t maxStack;

void jd_alloc_stack_check(void) {
    uint32_t *ptr = (uint32_t *)(STACK_BASE - STACK_SIZE);
    while (ptr < (uint32_t *)STACK_BASE) {
        if (*ptr != 0x33333333)
            break;
        ptr++;
    }
    int sz = STACK_BASE - (uint32_t)ptr;
    if (sz > maxStack)
        JD_LOG("stk:%d", maxStack = sz);
}

void jd_alloc_init(void) {
    JD_LOG("free:%d", HEAP_END - HEAP_BASE);

    aptr = (uint32_t *)HEAP_BASE;
    int p = 0;
    int sz = (uint32_t)&p - HEAP_BASE - 32;
    // seed PRNG with random RAM contents (later we add ADC readings)
    jd_seed_random(jd_hash_fnv1a((void *)HEAP_BASE, sz));
    memset((void *)HEAP_BASE, 0x33, sz);

    jd_alloc_stack_check();
}

void *jd_alloc(uint32_t size) {
    jd_alloc_stack_check();
    size = (size + 3) >> 2;
    void *r = aptr;
    aptr += size;
    if ((uint32_t)aptr > HEAP_END)
        jd_panic();
    memset(r, 0, size << 2);
    return r;
}

uint32_t jd_available_memory() {
    return HEAP_END - (uint32_t)aptr;
}

void *jd_alloc_emergency_area(uint32_t size) {
    if (size > HEAP_SIZE)
        jd_panic();
    return (void *)HEAP_BASE;
}
