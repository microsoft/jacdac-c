// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_protocol.h"
#include "services/interfaces/jd_flash.h"

#ifdef JD_FSTOR_BASE_ADDR

#define LOG(fmt, ...) DMESG("fstor: " fmt, ##__VA_ARGS__)

#ifndef JD_FSTOR_TOTAL_SIZE
#define JD_FSTOR_TOTAL_SIZE (128 * 1024)
#endif

#ifndef JD_FSTOR_HEADER_PAGES
#define JD_FSTOR_HEADER_PAGES 1
#endif

#define FSTOR_PAGES (JD_FSTOR_TOTAL_SIZE / JD_FLASH_PAGE_SIZE)
#define FSTOR_MAGIC0 0x52545346
#define FSTOR_MAGIC1 0x5f35460a

#define FSTOR_HEADER_SIZE (JD_FSTOR_HEADER_PAGES * JD_FLASH_PAGE_SIZE)
#define FSTOR_KEYSIZE 15

#define FSTOR_DATA_PAGE_OFF (JD_FSTOR_HEADER_PAGES * 2)
#define FSTOR_DATA_PAGES (FSTOR_PAGES - FSTOR_DATA_PAGE_OFF)

typedef struct {
    char key[FSTOR_KEYSIZE + 1];
    uint32_t offset;
    uint32_t size;
} jd_fstor_entry_t;

typedef const jd_fstor_entry_t entry_t;

typedef struct {
    uint32_t magic0;
    uint32_t magic1;
    uint32_t generation;
    uint32_t reserved;
    jd_fstor_entry_t entries[];
} jd_fstor_header_t;

static const jd_fstor_header_t *settings;
static const uint8_t *fstor_base;
static entry_t *last_entry;
static const uint8_t *data_start;
static uint32_t used_data_pages[(FSTOR_DATA_PAGES + 31) / 32];

static void erase_pages(const void *base, unsigned num) {
    for (unsigned i = 0; i < JD_FSTOR_HEADER_PAGES; ++i)
        flash_erase((void *)((const uint8_t *)base + i * JD_FLASH_PAGE_SIZE));
}

static inline unsigned align_core(unsigned size, unsigned sz) {
    if (size == 0)
        return sz;
    return (size + sz - 1) & ~(sz - 1);
}

static inline bool data_page_used(unsigned off) {
    JD_ASSERT(off < FSTOR_DATA_PAGES);
    return (used_data_pages[off / 32] & (1 << (off & 31))) != 0;
}

static void mark_data_page(unsigned off, bool used) {
    if (data_page_used(off) != used)
        used_data_pages[off / 32] ^= 1 << (off & 31);
}

static inline unsigned align(unsigned size) {
    return align_core(size, 8);
}

static inline unsigned align_page(unsigned size) {
    return align_core(size, JD_FLASH_PAGE_SIZE);
}

static inline unsigned num_data_pages(unsigned size) {
    return align_core(size, JD_FLASH_PAGE_SIZE) / JD_FLASH_PAGE_SIZE;
}

static inline bool is_valid(entry_t *e) {
    return ~e->offset != 0;
}

static inline bool is_large(entry_t *e) {
    return e->offset >= 2 * FSTOR_HEADER_SIZE;
}

static bool key_ok(const char *key) {
    if (!key || strlen(key) > FSTOR_KEYSIZE) {
        LOG("invalid key: '%s'", key);
        return false;
    }
    return true;
}

static void mark_large(entry_t *e, bool used) {
    JD_ASSERT(is_large(e));
    unsigned off = (e->offset / JD_FLASH_PAGE_SIZE) - FSTOR_DATA_PAGE_OFF;
    unsigned cnt = num_data_pages(e->size);
    for (unsigned i = 0; i < cnt; ++i)
        mark_data_page(off + i, used);
}

static int free_space(void) {
    return data_start - (const uint8_t *)last_entry - sizeof(entry_t) * 2;
}

static void oops(const char *msg) {
    LOG("mount failure: %s", msg);
    settings = NULL;
}

static bool has_later_copy(entry_t *e) {
    jd_fstor_entry_t tmp = *e;
    for (entry_t *q = e + 1; q <= last_entry; ++q) {
        if (strcmp(q->key, tmp.key) == 0)
            return true;
    }
    return false;
}

static void recompute_cache(void) {
    uint32_t minoff = ((const uint8_t *)settings + FSTOR_HEADER_SIZE) - fstor_base;
    entry_t *e = settings->entries;

    while (is_valid(e)) {
        if (e->offset < minoff)
            minoff = e->offset;
        e++;
        if ((const uint8_t *)e > fstor_base + minoff)
            return oops("last-ent");
    }

    last_entry = e - 1;
    data_start = fstor_base + minoff;

    const uint8_t *p = (const void *)e;
    while (p < data_start)
        if (*p++ != 0xff)
            return oops("non ff");

    memset(used_data_pages, 0, sizeof(used_data_pages));
    for (e = settings->entries; e <= last_entry; ++e) {
        if (is_large(e) && !has_later_copy(e))
            mark_large(e, true);
    }
}

static void write_header(unsigned gen) {
    jd_fstor_header_t hd = {
        .magic0 = FSTOR_MAGIC0,
        .magic1 = FSTOR_MAGIC1,
        .generation = gen,
    };
    flash_program((void *)settings, &hd, sizeof(hd));
    flash_sync();
}

static void jd_fstor_init(void) {
    if (settings)
        return;
    fstor_base = (const void *)(JD_FSTOR_BASE_ADDR);
    const jd_fstor_header_t *s0 = (const void *)fstor_base;
    const jd_fstor_header_t *s1 = (const void *)(fstor_base + FSTOR_HEADER_SIZE);
    if (s0->magic0 != FSTOR_MAGIC0 || s0->magic1 != FSTOR_MAGIC1)
        s0 = NULL;
    if (s1->magic0 != FSTOR_MAGIC0 || s1->magic1 != FSTOR_MAGIC1)
        s1 = NULL;
    if (s0 && s1) {
        settings = s0->generation > s1->generation ? s0 : s1;
    } else if (s0) {
        settings = s0;
    } else if (s1) {
        settings = s1;
    }

    if (settings)
        recompute_cache();

    if (!settings) {
        LOG("formatting now");
        settings = (const void *)fstor_base;
        erase_pages(settings, JD_FSTOR_HEADER_PAGES);
        write_header(1);
        recompute_cache();
        JD_ASSERT(settings != NULL);
    }

    LOG("mounted; %d free", free_space());
}

static void jd_fstor_gc(void) {
    LOG("gc");
    const jd_fstor_header_t *alt = (const void *)fstor_base;
    if (alt == settings)
        alt = (const void *)(fstor_base + FSTOR_HEADER_SIZE);
    erase_pages(alt, JD_FSTOR_HEADER_PAGES);

    entry_t *dst_e = alt->entries;
    const uint8_t *dst_data = (const uint8_t *)alt + FSTOR_HEADER_SIZE;

    for (entry_t *e = settings->entries; e <= last_entry; ++e) {
        if (has_later_copy(e))
            continue;
        jd_fstor_entry_t tmp = *e;
        dst_data -= align(tmp.size);
        flash_program((void *)dst_data, fstor_base + tmp.offset, tmp.size);
        tmp.offset = dst_data - fstor_base;
        flash_program((void *)dst_e, &tmp, sizeof(tmp));
        dst_e++;
    }

    unsigned newgen = settings->generation + 1;
    settings = alt;
    write_header(newgen);
    recompute_cache();
    JD_ASSERT(settings != NULL);

    LOG("gc done, %d free", free_space());
}

static entry_t *find_entry(const char *key) {
    jd_fstor_init();
    if (!key_ok(key))
        return NULL;
    for (entry_t *e = last_entry; e >= settings->entries; e--)
        if (strcmp(e->key, key) == 0)
            return e;
    return NULL;
}

int jd_settings_get_bin(const char *key, void *dst, unsigned space) {
    entry_t *e = find_entry(key);
    if (!e)
        return -1;
    if (is_large(e))
        return -2;
    if (space >= e->size)
        memcpy(dst, fstor_base + e->offset, space);
    return e->size;
}

int jd_settings_set_bin(const char *key, const void *val, unsigned size) {
    if (!key_ok(key))
        return -1;
    if (size > FSTOR_HEADER_SIZE - 200) {
        LOG("too large: %u", size);
        return -2;
    }

    entry_t *e = find_entry(key);
    if (e && !is_large(e) && e->size == size && memcmp(fstor_base + e->offset, val, size) == 0)
        return 0;

    if (e && is_large(e))
        mark_large(e, false);

    unsigned sizeoff = align(size);
    if (free_space() < sizeoff + sizeof(entry_t)) {
        jd_fstor_gc();
        if (free_space() < sizeoff + sizeof(entry_t)) {
            LOG("out of space; sz=%u", size);
            return -3;
        }
    }

    data_start -= sizeoff;
    flash_program((void *)data_start, val, size);

    jd_fstor_entry_t tmp = {
        .size = size,
        .offset = data_start - fstor_base,
    };
    memcpy(tmp.key, key, strlen(key));
    last_entry++;
    flash_program((void *)last_entry, &tmp, sizeof(tmp));
    flash_sync();

    JD_ASSERT((const uint8_t *)(last_entry + 1) <= data_start);

    return 0;
}

const void *jd_settings_get_large(const char *key, unsigned *sizep) {
    entry_t *e = find_entry(key);
    if (!e || !is_large(e))
        return NULL;
    if (sizep)
        *sizep = e->size;
    return fstor_base + e->offset;
}

static unsigned find_free_data(unsigned size) {
    unsigned pages = num_data_pages(size);
    unsigned off = 0;
    while (off < FSTOR_DATA_PAGES - pages) {
        for (unsigned i = 0; i < pages; ++i) {
            if (data_page_used(off + i)) {
                off += i + 1;
                break;
            }
            return (FSTOR_DATA_PAGE_OFF + off) * JD_FLASH_PAGE_SIZE;
        }
    }
    return 0;
}

void *jd_settings_prep_large(const char *key, unsigned size) {
    if (!key_ok(key))
        return NULL;

    entry_t *e = find_entry(key);
    if (e && is_large(e))
        mark_large(e, false);
    jd_fstor_entry_t tmp = {
        .size = size,
        .offset = find_free_data(size),
    };
    if (tmp.offset == 0) {
        LOG("no free pages; sz=%u", size);
        // if we marked previous entry as free, re-mark it a used
        if (e && is_large(e))
            mark_large(e, true);
        return NULL;
    }

    memcpy(tmp.key, key, strlen(key));
    if (free_space() < sizeof(tmp)) {
        jd_fstor_gc();
        if (free_space() < sizeof(tmp)) {
            LOG("no space for header");
            // no need to mark anything here - everything was re-computed by the GC
            return NULL;
        }
    }

    last_entry++;
    erase_pages(fstor_base + tmp.offset, num_data_pages(size));
    flash_program((void *)last_entry, &tmp, sizeof(tmp));
    flash_sync();

    return (void *)(fstor_base + tmp.offset);
}

int jd_settings_write_large(void *dst, const void *src, unsigned size) {
    flash_program(dst, src, size);
    return 0;
}

int jd_settings_sync_large() {
    flash_sync();
    return 0;
}

int jd_settings_large_delete(const char *key) {
    entry_t *e = find_entry(key);
    if (!e || !is_large(e))
        return -20;
    return jd_settings_set_bin(key, NULL, 0);
}

#endif
