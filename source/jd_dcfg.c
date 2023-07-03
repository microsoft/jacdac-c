// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_protocol.h"
#include "jd_dcfg.h"

#define LOG(fmt, ...) DMESG("dcfg: " fmt, ##__VA_ARGS__)
#define VLOG JD_NOLOG

STATIC_ASSERT(sizeof(dcfg_entry_t) == DCFG_ENTRY_SIZE);
STATIC_ASSERT(sizeof(dcfg_header_t) == DCFG_HEADER_SIZE);
STATIC_ASSERT(offsetof(dcfg_header_t, hash_jump) == DCFG_HEADER_HASH_JUMP_OFFSET);
STATIC_ASSERT(offsetof(dcfg_header_t, entries) == DCFG_HEADER_SIZE);

#ifdef JD_DCFG_BASE_ADDR

static bool dcfg_inited;

#define NUM_CFG 2
static const dcfg_header_t *configs[2];

#define user_config configs[0]
#define mfr_config configs[1]

typedef const dcfg_entry_t entry_t;

static void dcfg_init(void) {
    if (dcfg_inited)
        return;

    dcfg_inited = true;

    JD_ASSERT(!target_in_irq());

    const dcfg_header_t *hd = (void *)(JD_DCFG_BASE_ADDR);
    int err = dcfg_validate(hd);
    if (err == 0) {
        mfr_config = hd;
    } else {
        DMESG("! can't validate mfr config at %p, error %d", hd, err);
        return;
    }

    LOG("inited, %d entries, %u bytes", hd->num_entries, (unsigned)hd->total_bytes);
}

static uint16_t keyhash(const void *key, unsigned klen) {
    uint32_t h = jd_hash_fnv1a(key, klen);
    return h ^ (h >> 16);
}

// next 2020
#define CHECK(code, cond)                                                                          \
    do {                                                                                           \
        if (!(cond))                                                                               \
            return code;                                                                           \
    } while (0)

int dcfg_set_user_config(const dcfg_header_t *hd) {
    dcfg_init();        // init the mfr config first if needed
    user_config = NULL; // always reset it
    int err = dcfg_validate(hd);
    if (err == 0)
        user_config = hd;
    return err;
}

int dcfg_validate(const dcfg_header_t *hd) {
    CHECK(2000, hd != NULL);
    CHECK(2001, 0 == ((uintptr_t)hd & 3));

    CHECK(2002, hd->magic0 == DCFG_MAGIC0);
    CHECK(2003, hd->magic1 == DCFG_MAGIC1);

    unsigned num = hd->num_entries;
    entry_t *entries = hd->entries;
    const uint8_t *data_base = (const uint8_t *)hd;

    unsigned data_start = sizeof(dcfg_header_t) + sizeof(dcfg_entry_t) * (num + 1);
    unsigned total_bytes = hd->total_bytes;
    CHECK(2004, total_bytes < 128 * 1024);
    CHECK(2014, data_start <= total_bytes);

    CHECK(2015, entries[num].hash == 0xffff);
    CHECK(2016, entries[num].type_size == 0xffff);

    for (unsigned i = 0; i < DCFG_HASH_JUMP_ENTRIES; ++i) {
        unsigned idx = hd->hash_jump[i];
        CHECK(2017, idx <= num);
        CHECK(2018, (entries[idx].hash >> DCFG_HASH_SHIFT) >= i);
        CHECK(2019, idx == 0 || (entries[idx - 1].hash >> DCFG_HASH_SHIFT) < i);
    }

    if (num == 0)
        return 0;

    for (unsigned i = 0; i < num; ++i) {
        entry_t *e = &entries[i];
        unsigned klen = strlen(e->key);
        CHECK(2006, klen <= DCFG_KEYSIZE);
        CHECK(2007, keyhash(e->key, klen) == e->hash);
        CHECK(2008, i == 0 || entries[i - 1].hash <= entries[i].hash);

        unsigned size = dcfg_entry_size(e);
        switch (dcfg_entry_type(e)) {
        case DCFG_TYPE_U32:
        case DCFG_TYPE_I32:
            CHECK(2009, size == 0);
            break;
        case DCFG_TYPE_STRING:
        case DCFG_TYPE_BLOB:
            CHECK(2010, e->value >= data_start);
            CHECK(2011, e->value < total_bytes);
            CHECK(2012, e->value + size < total_bytes);
            CHECK(2013, data_base[e->value + size] == 0x00);
            break;
        }
    }

    return 0;
}

static entry_t *get_entry(const dcfg_header_t *hd, const char *key) {
    if (!key || !hd)
        return NULL;
    unsigned len = strlen(key);
    if (len > DCFG_KEYSIZE)
        return NULL;

    unsigned hash = keyhash(key, len);
    unsigned hidx = hash >> DCFG_HASH_SHIFT;
    unsigned idx = hd->hash_jump[hidx];
    unsigned num = hd->num_entries;
    entry_t *entries = hd->entries;
    while (idx < num && entries[idx].hash <= hash) {
        if (entries[idx].hash == hash && memcmp(entries[idx].key, key, len) == 0)
            return &entries[idx];
        idx++;
    }
    return NULL;
}

entry_t *dcfg_get_entry(const char *key) {
    dcfg_init();
    for (unsigned i = 0; i < NUM_CFG; ++i) {
        entry_t *e = get_entry(configs[i], key);
        if (e)
            return e;
    }
    return NULL;
}

static int entry_idx(const dcfg_header_t *hd, entry_t *e) {
    if (!hd)
        return -1;
    unsigned idx = e - hd->entries;
    if (idx < hd->num_entries)
        return idx;
    return -1;
}

const dcfg_entry_t *dcfg_get_next_entry(const char *prefix, const dcfg_entry_t *previous) {
    dcfg_init();

    int plen = strlen(prefix);
    for (unsigned i = 0; i < NUM_CFG; ++i) {
        const dcfg_header_t *hd = configs[i];
        if (hd == NULL)
            continue;
        int idx = 0;
        if (previous) {
            idx = entry_idx(hd, previous);
            if (idx < 0)
                continue;
            idx++;
        }
        while (idx < hd->num_entries) {
            if (memcmp(prefix, hd->entries[idx].key, plen) == 0) {
                return &hd->entries[idx];
            }
            idx++;
        }
        previous = NULL;
    }
    return NULL;
}

int32_t dcfg_get_i32(const char *key, int32_t defl) {
    entry_t *e = dcfg_get_entry(key);
    switch (dcfg_entry_type(e)) {
    case DCFG_TYPE_U32:
        return e->value > 0x7fffffff ? defl : (int32_t)e->value;
    case DCFG_TYPE_I32:
        return (int32_t)e->value;
    default:
        return defl;
    }
}

uint8_t dcfg_get_pin(const char *key) {
    const char *lbl = dcfg_get_string(key, NULL);
    if (lbl != NULL)
        key = dcfg_idx_key("pins.", 0xffff, lbl);
    return (uint8_t)dcfg_get_i32(key, -1);
}

uint32_t dcfg_get_u32(const char *key, uint32_t defl) {
    entry_t *e = dcfg_get_entry(key);
    switch (dcfg_entry_type(e)) {
    case DCFG_TYPE_I32:
        return (int32_t)e->value < 0 ? defl : e->value;
    case DCFG_TYPE_U32:
        return e->value;
    default:
        return defl;
    }
}

const char *dcfg_get_string(const char *key, unsigned *sizep) {
    dcfg_init();

    entry_t *e = NULL;
    const void *base = NULL;
    for (unsigned i = 0; i < NUM_CFG; ++i) {
        e = get_entry(configs[i], key);
        if (e) {
            base = configs[i];
            break;
        }
    }

    switch (dcfg_entry_type(e)) {
    case DCFG_TYPE_BLOB:
    case DCFG_TYPE_STRING: {
        if (sizep)
            *sizep = dcfg_entry_size(e);
        const char *str = (const char *)base + e->value;
        return str;
    }
    default:
        if (sizep)
            *sizep = 0;
        return NULL;
    }
}

char *dcfg_idx_key(const char *prefix, unsigned idx, const char *suffix) {
    static char keybuf[DCFG_KEYSIZE + 1];

    unsigned ptr = 0;
    unsigned len;

    if (prefix) {
        len = strlen(prefix);
        if (len > DCFG_KEYSIZE - 1)
            return NULL;
        if (prefix != keybuf)
            memcpy(keybuf, prefix, len);
        ptr += len;
    }

    if (idx != 0xffff) {
        if (idx > 100)
            return NULL;
        keybuf[ptr++] = 0x80 + idx;
    }

    if (suffix) {
        len = strlen(suffix);
        if (ptr + len > DCFG_KEYSIZE)
            return NULL;
        memcpy(keybuf + ptr, suffix, len);
        ptr += len;
    }

    keybuf[ptr] = 0;
    return keybuf;
}

#endif
