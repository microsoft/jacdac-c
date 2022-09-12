// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_protocol.h"

#if JD_SIMPLE_ALLOC

#define STACK_SIZE 512
#define STACK_BASE ((uintptr_t)&_estack)
#define HEAP_BASE ((uintptr_t)&_end)
#define HEAP_END (STACK_BASE - STACK_SIZE)
#define HEAP_SIZE (HEAP_END - HEAP_BASE)
#define HEAP_SIZE_WORDS ((HEAP_END - HEAP_BASE) / sizeof(void *))
#define HEAP_END_PTR (uintptr_t *)(STACK_BASE - STACK_SIZE)

#if JD_FREE_SUPPORTED
#define TAG_POS 24
#define USED_TAG 0x13
#define FREE_TAG 0x42
#define GET_TAG(p) ((p) >> TAG_POS)
#define MARK_FREE(p) ((p) | (FREE_TAG << TAG_POS))
#define MARK_USED(p) ((p) | (USED_TAG << TAG_POS))
#define BLOCK_SIZE(p) (((p) << 8) >> 8)
static uint32_t num_alloc;
#endif

extern uintptr_t _end;
extern uintptr_t _estack;

static uintptr_t *aptr;
static uint16_t maxStack;

void jd_alloc_stack_check(void) {
    uintptr_t *ptr = (uintptr_t *)(STACK_BASE - STACK_SIZE);
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

    aptr = (uintptr_t *)HEAP_BASE;
    int p = 0;
    int sz = (uintptr_t)&p - HEAP_BASE - 32;
    // seed PRNG with random RAM contents (later we add ADC readings)
    jd_seed_random(jd_hash_fnv1a((void *)HEAP_BASE, sz));
    memset((void *)HEAP_BASE, 0x33, sz);

#if JD_FREE_SUPPORTED
    *aptr = MARK_FREE(HEAP_SIZE_WORDS - 1);
#endif

    jd_alloc_stack_check();
}

void *jd_alloc(uint32_t size) {
    // convert size from bytes to words
    size = (size + 3) >> 2;

#if JD_FREE_SUPPORTED
    // if jd_free() is supported we check stack often at the beginning and less often later
    num_alloc++;
    if (num_alloc < 32 || (num_alloc & 31) == 0)
        jd_alloc_stack_check();

    // this borrows ideas from
    // https://github.com/lancaster-university/codal-core/blob/master/source/core/CodalHeapAllocator.cpp
    uintptr_t *block, *next;
    uintptr_t block_size;

    size++; // the block size down below includes the one word header

    for (block = aptr; block != HEAP_END_PTR; block += block_size) {
        block_size = BLOCK_SIZE(*block);

        if (GET_TAG(*block) == USED_TAG)
            continue;
        JD_ASSERT(GET_TAG(*block) == FREE_TAG);

        // We have a free block. Let's see if the subsequent ones are too. If so, we can merge...
        next = block + block_size;

        while (next != HEAP_END_PTR && GET_TAG(*next) == FREE_TAG) {
            JD_ASSERT(next < HEAP_END_PTR);

            // We can merge!
            block_size += BLOCK_SIZE(*next);
            *block = MARK_FREE(block_size);

            next = block + block_size;
        }

        // We have a free block. Let's see if it's big enough. If so, we have a winner.
        if (block_size >= size)
            break;
    }

    if (block == HEAP_END_PTR)
        jd_panic(); // OOM
    JD_ASSERT(block < HEAP_END_PTR);

    // if the size is close, mark the whole block as used
    if (block_size <= size + 1) {
        *block = MARK_USED(block_size);
    } else {
        // otherwise split it in two
        uintptr_t *split_block = block + size;
        *split_block = MARK_FREE(block_size - size);
        *block = MARK_USED(size);
    }
    void *r = block + 1;
    memset(r, 0x00, (size << 2) - 4);

#else
    jd_alloc_stack_check();
    void *r = aptr;
    aptr += size;
    if ((uintptr_t)aptr > HEAP_END)
        jd_panic();
    memset(r, 0x00, size << 2);
#endif

    return r;
}

#if JD_FREE_SUPPORTED
void jd_free(void *ptr) {
    if (ptr == NULL)
        return;
    JD_ASSERT(((uintptr_t)ptr & 3) == 0);
    JD_ASSERT((void *)aptr < ptr && ptr < (void *)HEAP_END);
    uintptr_t *p = ptr;
    p--;
    JD_ASSERT(GET_TAG(*p) == USED_TAG);
    *p = MARK_FREE(BLOCK_SIZE(*p));
    memset(p + 1, 0x47, (BLOCK_SIZE(*p) - 1) << 2);
}
#else
uint32_t jd_available_memory() {
    return HEAP_END - (uintptr_t)aptr;
}
#endif

void *jd_alloc_emergency_area(uint32_t size) {
    if (size > HEAP_SIZE)
        jd_panic();
    return (void *)HEAP_BASE;
}

#endif
