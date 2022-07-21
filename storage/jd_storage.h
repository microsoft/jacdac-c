#pragma once

#include "jd_protocol.h"

// config
#ifndef JD_LSTORE_FLUSH_SECONDS
#define JD_LSTORE_FLUSH_SECONDS 10
#endif

#ifndef JD_LSTORE_FILE_SIZE
#define JD_LSTORE_FILE_SIZE (65 * 1024 * 1024)
#endif

#ifndef JD_LSTORE_NUM_FILES
#define JD_LSTORE_NUM_FILES 2
#endif

#if JD_LSTORE

// user-facing functions
void jd_lstore_init(void);
void jd_lstore_process(void);
int jd_lstore_append(unsigned logidx, unsigned type, const void *data, unsigned datasize);
int jd_lstore_append_frag(unsigned logidx, unsigned type, const void *data, unsigned datasize);
bool jd_lstore_is_enabled(void);
// should only be called before application exit and similar events
void jd_lstore_force_flush(void);

void jd_lstore_panic_print_char(char c);
void jd_lstore_panic_print_str(const char *s);
void jd_lstore_panic_flush(void);

// these are only used when !JD_LSTORE_FF
// create a file
// *size is minumum size on input, actual size on output
// *sector_off specifies offset to use when calling ff_disk_read/write()
// returns 0 on success
int jd_f_create(const char *name, uint32_t *size, uint32_t *sector_off);

#else

static inline void jd_lstore_init(void) {}
static inline void jd_lstore_force_flush(void) {}
static inline void jd_lstore_process(void) {}
static inline int jd_lstore_append(unsigned logidx, unsigned type, const void *data,
                                   unsigned datasize) {
    return -10;
}
static inline int jd_lstore_append_frag(unsigned logidx, unsigned type, const void *data,
                                        unsigned datasize) {
    return -10;
}
static inline bool jd_lstore_is_enabled(void) {
    return false;
}

#endif

#define JD_LSTORE_TYPE_DEVINFO 0x01
#define JD_LSTORE_TYPE_DMESG 0x02
#define JD_LSTORE_TYPE_LOG 0x03
#define JD_LSTORE_TYPE_JD_FRAME 0x04
#define JD_LSTORE_TYPE_PANIC_LOG 0x05

// file format
#define JD_LSTORE_MAGIC0 0x0a4c444a
#define JD_LSTORE_MAGIC1 0xb5d1841e
#define JD_LSTORE_VERSION 5

#define JD_LSTORE_BLOCK_OVERHEAD                                                                   \
    (sizeof(jd_lstore_block_header_t) + sizeof(jd_lstore_block_footer_t))

#define JD_LSTORE_ENTRY_HEADER_SIZE 4

typedef struct {
    uint64_t device_id;
    char firmware_name[64];
    char firmware_version[32];
    uint8_t reserved[32];
} jd_lstore_device_info_t;

typedef struct {
    uint32_t magic0;
    uint32_t magic1;
    uint32_t version;
    uint32_t sector_size;       // currently always 512
    uint32_t sectors_per_block; // typically 8
    uint32_t header_blocks;     // typically 1
    uint32_t num_blocks;        // including header block(s)
    uint32_t num_rewrites;      // how many times the whole log was rewritten (wrapped-around)
    uint32_t block_magic0;      // used in all blocks in this file
    uint32_t block_magic1;      // used in all blocks in this file
    uint32_t reserved[6];

    // meta-data about device
    jd_lstore_device_info_t devinfo;

    // meta-data about the log file
    char purpose[32];
    char comment[64];
    uint8_t reserved_log[32];
} jd_lstore_main_header_t;

typedef struct {
    uint32_t block_magic0;
    uint32_t generation;
    uint64_t timestamp; // in ms
    uint8_t data[0];
} jd_lstore_block_header_t;

typedef struct {
    uint32_t block_magic1;
    uint32_t crc32;
} jd_lstore_block_footer_t;

typedef struct {
    uint8_t type;
    uint8_t size;    // of 'data'
    uint16_t tdelta; // wrt to block timestamp
    uint8_t data[0];
} jd_lstore_entry_t;
