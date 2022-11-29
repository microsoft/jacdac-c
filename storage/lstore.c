#include "jd_storage.h"

#if JD_LSTORE

#include "ff/ff.h"
#include "ff/diskio.h"
#include "services/interfaces/jd_pins.h"

#ifdef PIN_SD_CS
#define JD_SD_PANIC 1
#else
#define JD_SD_PANIC 0
#endif

#ifndef JD_LSTORE_FF
#define JD_LSTORE_FF 1
#endif

#define NOT_IRQ()                                                                                  \
    if (target_in_irq())                                                                           \
    JD_PANIC()

#define SECTOR_SHIFT 9
#define SECTOR_SIZE (1 << SECTOR_SHIFT)

#define LOG(msg, ...) DMESG("sd: " msg, ##__VA_ARGS__)
// #define LOGV(msg, ...) DMESG("sd: " msg, ##__VA_ARGS__)
#define LOGV JD_NOLOG
#define CHK JD_CHK

STATIC_ASSERT(sizeof(jd_lstore_main_header_t) <= SECTOR_SIZE);
STATIC_ASSERT(JD_LSTORE_ENTRY_HEADER_SIZE == offsetof(jd_lstore_entry_t, data));

typedef struct {
    struct jd_lstore_ctx *parent;
    jd_lstore_block_header_t *block;
    jd_lstore_block_header_t *block_alt;
    // file properties
    uint32_t sector_off;
    uint32_t size;
    uint32_t header_blocks;
    uint32_t data_blocks;
    uint8_t block_shift; // block_size == 1 << block_shift
    volatile uint8_t needs_flushing;
    // block-common information
    uint32_t block_magic0;
    uint32_t block_magic1;
    uint32_t block_generation;
    // information about current block
    uint32_t block_ptr;
    uint32_t data_ptr;
} jd_lstore_file_t;

typedef struct jd_lstore_ctx {
    FATFS fs;
    uint32_t flush_timer;
    uint8_t panic_mode;
    uint8_t panic_char_ptr;
    uint8_t panic_max_char_ptr;
    jd_lstore_file_t logs[JD_LSTORE_NUM_FILES];
} jd_lstore_ctx_t;

static jd_lstore_ctx_t *ls_ctx;

#if JD_LSTORE_FF
static FRESULT check_contiguous_file(FIL *fp) {
    DWORD clst, clsz, step;
    FSIZE_t fsz;
    FRESULT fr;
    int cont = 0;

    fr = f_rewind(fp); /* Validates and prepares the file */
    if (fr != FR_OK)
        return fr;

#if FF_MAX_SS == FF_MIN_SS
    clsz = (DWORD)fp->obj.fs->csize * FF_MAX_SS; /* Cluster size */
#else
    clsz = (DWORD)fp->obj.fs->csize * fp->obj.fs->ssize;
#endif
    fsz = f_size(fp);
    if (fsz > 0) {
        clst = fp->obj.sclust - 1; /* A cluster leading the first cluster for first test */
        while (fsz) {
            step = (fsz >= clsz) ? clsz : (DWORD)fsz;
            fr = f_lseek(fp, f_tell(fp) + step); /* Advances file pointer a cluster */
            if (fr != FR_OK)
                return fr;
            if (clst + 1 != fp->clust)
                break; /* Is not the cluster next to previous one? */
            clst = fp->clust;
            fsz -= step; /* Get current cluster for next test */
        }
        if (fsz == 0)
            cont = 1; /* All done without fail? */
    }

    return cont ? FR_OK : FR_INVALID_OBJECT;
}

#if 0
static void list_files(jd_lstore_ctx_t *ctx) {
    FF_DIR dir;
    FILINFO finfo;

    CHK(f_opendir(&dir, "0:/"));
    for (;;) {
        CHK(f_readdir(&dir, &finfo));
        if (finfo.fname[0] == '\0')
            break;
        LOG("%s: %d bytes", finfo.fname, finfo.fsize);
    }
    CHK(f_closedir(&dir));
}
#endif
#endif

static void read_sectors(jd_lstore_file_t *f, uint32_t sector_addr, void *dst,
                         uint32_t num_sectors) {
    jd_lstore_ctx_t *ctx = f->parent;
    JD_ASSERT(!ctx->panic_mode);
    CHK(ff_disk_read(ctx->fs.pdrv, dst, f->sector_off + sector_addr, num_sectors));
}

#if JD_SD_PANIC
static int sd_write_sector(uint32_t addr, const void *blk);
#endif

static void write_sectors(jd_lstore_file_t *f, uint32_t sector_addr, const void *src,
                          uint32_t num_sectors) {
    jd_lstore_ctx_t *ctx = f->parent;
    // LOG("wr sect %x %d", (unsigned)(f->sector_off + sector_addr), (int)num_sectors);
    if (ctx->panic_mode == 1) {
#if JD_SD_PANIC
        while (num_sectors > 0) {
            if (sd_write_sector(f->sector_off + sector_addr, src) != 0) {
                ctx->panic_mode = 3;
                return;
            }
            sector_addr++;
            src = (const uint8_t *)src + 512;
            num_sectors--;
        }
#endif
        return;
    } else if (ctx->panic_mode) {
        return;
    }

    CHK(ff_disk_write(ctx->fs.pdrv, src, f->sector_off + sector_addr, num_sectors));
}

static bool validate_header(jd_lstore_file_t *f, jd_lstore_main_header_t *hd) {
    if (hd->magic0 == JD_LSTORE_MAGIC0 && hd->magic1 == JD_LSTORE_MAGIC1) {
        if (hd->version != JD_LSTORE_VERSION) {
            LOG("wrong version %x", (unsigned)hd->version);
            return 0;
        }
        if (hd->sector_size != SECTOR_SIZE) {
            LOG("invalid sector size %u", (unsigned)hd->sector_size);
            return 0;
        }

        if (hd->sectors_per_block * hd->num_blocks > f->size / SECTOR_SIZE) {
            LOG("file too small for %u*%u sectors", (unsigned)hd->sectors_per_block,
                (unsigned)hd->num_blocks);
            return 0;
        }

        int sh = 0;
        while ((1U << sh) < hd->sectors_per_block)
            sh++;
        if (hd->sectors_per_block > 8 || (1U << sh) != hd->sectors_per_block) {
            LOG("invalid sector/block %u", (unsigned)hd->sectors_per_block);
            return 0;
        }

        f->block_shift = sh + SECTOR_SHIFT;

        return 1;
    } else {
        LOG("invalid magic");
        return 0;
    }
}

static void strcpy_n(char *dst, size_t sz, const char *src) {
    if (strlen(src) >= sz - 1)
        memcpy(dst, src, sz - 1);
    else
        strcpy(dst, src);
}

#define STRCPY(buf, src) strcpy_n(buf, sizeof(buf), src)

static inline unsigned block_size(jd_lstore_file_t *f) {
    return 1 << f->block_shift;
}

static inline unsigned block_data_size(jd_lstore_file_t *f) {
    return block_size(f) - JD_LSTORE_BLOCK_OVERHEAD;
}

static inline jd_lstore_block_footer_t *block_footer(jd_lstore_file_t *f) {
    return (void *)((uint8_t *)f->block + block_size(f) - sizeof(jd_lstore_block_footer_t));
}

static inline jd_lstore_block_footer_t *block_alt_footer(jd_lstore_file_t *f) {
    return (void *)((uint8_t *)f->block_alt + block_size(f) - sizeof(jd_lstore_block_footer_t));
}

static bool block_valid(jd_lstore_file_t *f) {
    LOGV("bl %x %x", f->block->block_magic0, f->block_magic0);
    if (f->block->block_magic0 == f->block_magic0 &&
        block_footer(f)->block_magic1 == f->block_magic1) {
        if (jd_crc32(f->block, block_size(f) - 4) == block_footer(f)->crc32)
            return 1;
        return 0;
    } else {
        return 0;
    }
}

static int read_block(jd_lstore_file_t *f, uint32_t idx) {
    int sh = f->block_shift - SECTOR_SHIFT;
    JD_ASSERT(idx < f->data_blocks);
    read_sectors(f, (f->header_blocks + idx) << sh, f->block, 1 << sh);
    if (!block_valid(f)) {
        LOGV("invalid %d", idx);
        f->block->block_magic0 = 0;
        f->block->generation = 0;
        f->block->timestamp = 0;
        return -1;
    } else {
        LOGV("valid %d", idx);
    }
    return 0;
}

static void write_block(jd_lstore_file_t *f, uint32_t idx) {
    int sh = f->block_shift - SECTOR_SHIFT;
    JD_ASSERT(idx < f->data_blocks);
    write_sectors(f, (f->header_blocks + idx) << sh, f->block_alt, 1 << sh);
}

static bool block_lt(jd_lstore_block_header_t *a, jd_lstore_block_header_t *b) {
    return a->generation < b->generation ||
           (a->generation == b->generation && a->timestamp < b->timestamp);
}

static void find_boundry(jd_lstore_file_t *f) {
    int l = 0;
    int r = f->data_blocks - 1;
    read_block(f, l);
    jd_lstore_block_header_t start_hd = *f->block;
    if (!start_hd.block_magic0) {
        LOG("new file");
        f->block_generation = 1;
        f->block_ptr = 0;
        return;
    }

    // inv: bl[0] <= bl[l]
    // inv: bl[r+1] <= bl[0]
    while (l < r) {
        int m = (l + r) / 2 + 1;
        // inv: l < m <= r
        read_block(f, m);
        LOGV("%x < %x ? ", start_hd.generation, f->block->generation);
        if (block_lt(&start_hd, f->block)) {
            l = m;
        } else {
            r = m - 1;
        }
    }

    read_block(f, l);
    f->block_generation = f->block->generation + 1;
    JD_ASSERT(f->block_generation < 0xffffffff);
    if (l + 1 == (int)f->block_ptr)
        f->block_ptr = 0;
    else
        f->block_ptr = l + 1;
}

static uint32_t random_magic(void) {
    for (;;) {
        uint32_t r = jd_random();
        if (r != 0 && (r + 1) != 0)
            return r;
    }
}

static void fill_devinfo(jd_lstore_device_info_t *hd) {
    memset(hd, 0, sizeof(*hd));
    hd->device_id = jd_device_id();
    STRCPY(hd->firmware_name, app_get_dev_class_name());
    STRCPY(hd->firmware_version, app_get_fw_version());
}

static void mount_log(jd_lstore_file_t *f, const char *name, int bl_shift) {
    jd_lstore_main_header_t *hd = jd_alloc(SECTOR_SIZE);
    read_sectors(f, 0, hd, 1);
    if (!validate_header(f, hd)) {
        memset(hd, 0, SECTOR_SIZE);
        hd->magic0 = JD_LSTORE_MAGIC0;
        hd->magic1 = JD_LSTORE_MAGIC1;
        hd->version = JD_LSTORE_VERSION;
        hd->sector_size = SECTOR_SIZE;
        hd->sectors_per_block = 1 << bl_shift;
        hd->header_blocks = 1;
        f->block_shift = bl_shift + SECTOR_SHIFT;
        hd->num_blocks = f->size >> f->block_shift;
        hd->block_magic0 = random_magic();
        hd->block_magic1 = random_magic();
        fill_devinfo(&hd->devinfo);
        STRCPY(hd->purpose, name);
        LOG("writing new header for '%s'", name);
        write_sectors(f, 0, hd, 1);
        if (!validate_header(f, hd))
            JD_PANIC();
    }

    f->header_blocks = hd->header_blocks;
    f->data_blocks = hd->num_blocks - f->header_blocks;
    f->block_magic0 = hd->block_magic0;
    f->block_magic1 = hd->block_magic1;

    hd->purpose[sizeof(hd->purpose) - 1] = 0;
    LOG("mounted '%s' (%ukB) shift=%u", hd->purpose,
        (unsigned)(f->data_blocks << f->block_shift >> 10), (unsigned)f->block_shift);

    jd_free(hd);
    f->block = jd_alloc(block_size(f));

    find_boundry(f);

    // prep block for writing
    memset(f->block, 0, block_size(f));
    f->block->block_magic0 = f->block_magic0;
    block_footer(f)->block_magic1 = f->block_magic1;

    f->block_alt = jd_alloc(block_size(f));
    memcpy(f->block_alt, f->block, block_size(f));

    LOG("generation %u (ptr=%u)", (unsigned)f->block_generation, (unsigned)f->block_ptr);
}

static int request_flush(jd_lstore_file_t *f) {
    JD_ASSERT(f->data_ptr != 0);
    if (f->needs_flushing)
        return -1;
    jd_lstore_block_header_t *tmp = f->block;
    f->block = f->block_alt;
    f->block_alt = tmp;
    f->needs_flushing = 1;
    LOG("flushing %u/%u @%d", (unsigned)f->data_ptr, block_data_size(f),
        (int)(f - f->parent->logs));
    f->data_ptr = 0;
    return 0;
}

static void flush_to_disk_core(jd_lstore_file_t *f) {
    block_alt_footer(f)->crc32 = jd_crc32(f->block_alt, block_size(f) - 4);
    LOGV("writing block %d g=%d", f->block_ptr, f->block_alt->generation);
    write_block(f, f->block_ptr++);
    // clear it for future use
    memset(f->block_alt->data, 0, block_data_size(f));

    if (f->block_ptr >= f->data_blocks) {
        if (!f->parent->panic_mode) {
            jd_lstore_main_header_t *hd = jd_alloc(SECTOR_SIZE);
            read_sectors(f, 0, hd, 1);
            hd->num_rewrites++;
            LOG("bumping rewrites to %u", (unsigned)hd->num_rewrites);
            write_sectors(f, 0, hd, 1);
            jd_free(hd);
        }
        f->block_ptr = 0;
    }

    f->needs_flushing = 0;
}

static void flush_to_disk(jd_lstore_file_t *f) {
    NOT_IRQ();

    int needs_flushing = 0;

    target_disable_irq();
    if (f->data_ptr != 0 && !f->needs_flushing)
        request_flush(f);
    if (f->needs_flushing) {
        JD_ASSERT(f->needs_flushing == 1);
        f->needs_flushing = 2;
        needs_flushing = 1;
    }
    target_enable_irq();

    if (!needs_flushing)
        return;

    flush_to_disk_core(f);
}

void jd_lstore_process(void) {
    jd_lstore_ctx_t *ctx = ls_ctx;

    if (!ctx || !ctx->logs[0].block)
        return;

    int force = jd_should_sample_ms(&ctx->flush_timer, JD_LSTORE_FLUSH_SECONDS * 1000);

    for (int i = 0; i < JD_LSTORE_NUM_FILES; ++i) {
        jd_lstore_file_t *lf = &ctx->logs[i];
        if (lf->needs_flushing || force)
            flush_to_disk(lf);
    }
}

void jd_lstore_force_flush(void) {
    jd_lstore_ctx_t *ctx = ls_ctx;

    if (!ctx || !ctx->logs[0].block)
        return;

    for (int k = 0; k < 2; ++k)
        for (int i = 0; i < JD_LSTORE_NUM_FILES; ++i) {
            jd_lstore_file_t *lf = &ctx->logs[i];
            flush_to_disk(lf);
        }
}

int jd_lstore_append_frag(unsigned logidx, unsigned type, const void *data, unsigned datasize) {
    while (datasize > 0xff) {
        int r = jd_lstore_append(logidx, type, data, 0xff);
        if (r)
            return r;
        datasize -= 0xff;
        data = (const uint8_t *)data + 0xff;
    }

    return jd_lstore_append(logidx, type, data, datasize);
}

static jd_lstore_entry_t *block_curr_ptr(jd_lstore_file_t *f) {
    return (void *)&f->block->data[f->data_ptr];
}

int jd_lstore_append(unsigned logidx, unsigned type, const void *data, unsigned datasize) {
    jd_lstore_ctx_t *ctx = ls_ctx;
    JD_ASSERT(logidx < JD_LSTORE_NUM_FILES);
    jd_lstore_file_t *f = &ctx->logs[logidx];
    if (!ctx || !f->block)
        return -10;

    jd_lstore_entry_t ent = {.type = type, .size = datasize};
    JD_ASSERT(ent.type == type);
    JD_ASSERT(ent.size == datasize);

    int res = 0;

    target_disable_irq();

    for (;;) {
        if (f->data_ptr == 0) {
            // starting new block
            f->block->timestamp = now_ms_long;
            f->block->generation = f->block_generation;
        }

        int64_t delta = now_ms_long - f->block->timestamp;

        if (delta < 0) {
            LOG("time went back!");
            f->block_generation++;
            // need flush
        } else {
            uint32_t new_data_ptr = f->data_ptr + JD_LSTORE_ENTRY_HEADER_SIZE + datasize;
            if (delta > 0xffff || new_data_ptr > block_data_size(f)) {
                // need flush
            } else {
                // normal path
                ent.tdelta = delta;
                memcpy(block_curr_ptr(f), &ent, sizeof(ent));
                if (datasize)
                    memcpy(block_curr_ptr(f)->data, data, datasize);
                f->data_ptr = new_data_ptr;
                break;
            }
        }

        res = request_flush(f);
        if (res) {
            // TODO store that there was overflow
            break;
        }
    }

    target_enable_irq();

    return res;
}

bool jd_lstore_is_enabled(void) {
    return ls_ctx && ls_ctx->logs[0].block;
}

void jd_lstore_init(void) {
    jd_lstore_ctx_t *ctx = jd_alloc(sizeof(*ctx));
    ls_ctx = ctx;

#if JD_LSTORE_FF
    const char *drv = "0:";

    FRESULT res = f_mount(&ctx->fs, drv, 1);
    if (res != FR_OK) {
        DMESG("failed to mount card (%d)", res);
        JD_PANIC();
    } else {
        unsigned kb = (ctx->fs.n_fatent >> 1) * ctx->fs.csize;
        LOG("mounted! %uMB", kb >> 10);
    }

    // list_files(ctx);

    FIL *fil = jd_alloc(sizeof(FIL));

    for (int i = 0; i < JD_LSTORE_NUM_FILES; ++i) {
        jd_lstore_file_t *lf = &ctx->logs[i];
        lf->parent = ctx;
        char *fn = jd_sprintf_a("0:/LOG_%d.JDL", i);
        int is_created = 0;

        CHK(f_open(fil, fn, FA_OPEN_ALWAYS | FA_READ | FA_WRITE));

        int contchk = FR_NO_FILE;

        if (fil->obj.objsize >= JD_LSTORE_FILE_SIZE) {
            contchk = check_contiguous_file(fil);
        }

        const char *st = "OK";

        if (contchk != 0) {
            LOG("will create r=%d st=%u sz=%u", contchk, (unsigned)fil->obj.sclust,
                (unsigned)fil->obj.objsize);
            CHK(f_close(fil));
            CHK(f_open(fil, fn, FA_CREATE_ALWAYS | FA_READ | FA_WRITE));
            CHK(f_expand(fil, JD_LSTORE_FILE_SIZE, 1));
            st = "created";
            is_created = 1;
        }

        lf->size = fil->obj.objsize;
        lf->sector_off = ctx->fs.database + ctx->fs.csize * (fil->obj.sclust - 2);

        LOG("%s st=%u [%x] sz=%u %s", fn, (unsigned)fil->obj.sclust, (unsigned)lf->sector_off,
            (unsigned)fil->obj.objsize, st);

        CHK(f_close(fil));
        jd_free(fn);

        if (is_created) {
            void *hd = jd_alloc(SECTOR_SIZE);
            write_sectors(lf, 0, hd, 1); // clear header just in case
            jd_free(hd);
        }
    }

    jd_free(fil);
#else
    for (int i = 0; i < JD_LSTORE_NUM_FILES; ++i) {
        jd_lstore_file_t *lf = &ctx->logs[i];
        lf->parent = ctx;
        char *fn = jd_sprintf_a("log_%d.jdl", i);

        lf->size = JD_LSTORE_FILE_SIZE;
        CHK(jd_f_create(fn, &lf->size, &lf->sector_off));

        LOG("%s off=[%x] sz=%u", fn, (unsigned)lf->sector_off, (unsigned)lf->size);

        jd_free(fn);
    }
#endif

    mount_log(&ctx->logs[0], "packets and serial", 3); // 4K
    mount_log(&ctx->logs[1], "data log", 0);           // 0.5K

    jd_lstore_device_info_t info;
    fill_devinfo(&info);

    for (int i = 0; i < JD_LSTORE_NUM_FILES; ++i)
        jd_lstore_append(i, JD_LSTORE_TYPE_DEVINFO, &info, sizeof(info));
}

/*
 * Dump to SD card in panic handler.
 */

// http://elm-chan.org/docs/mmc/mmc_e.html
// http://chlazza.nfshost.com/sdcardinfo.html

#if JD_SD_PANIC

#define PANIC_LOG(msg, ...) DMESG("sdpanic: " msg, ##__VA_ARGS__)

static void flush_to_disk_in_panic(jd_lstore_file_t *f) {
    if (f->needs_flushing)
        flush_to_disk_core(f);
    if (f->data_ptr != 0 && !f->needs_flushing)
        request_flush(f);
    if (f->needs_flushing)
        flush_to_disk_core(f);
    f->parent->panic_char_ptr = 0;
    f->parent->panic_max_char_ptr = 0;
    f->block->timestamp = now_ms_long;
    f->block->generation = f->block_generation;
}

static int sd_get_resp(void) {
    int max = 32;
    while (max--) {
        uint8_t r = spi_bb_byte();
        if (r != 0xff)
            return r;
    }
    return -1;
}

static int sd_send_cmd(uint8_t cmdid, uint32_t arg) {
    uint8_t cmd[6];
    cmd[0] = 0x40 + cmdid;
    cmd[1] = arg >> 24;
    cmd[2] = arg >> 16;
    cmd[3] = arg >> 8;
    cmd[4] = arg >> 0;
    cmd[5] = jd_sd_crc7(cmd, 5);
    spi_bb_tx(cmd, sizeof(cmd));
    return sd_get_resp();
}

static int sd_read_data(void *dst, unsigned size) {
    int max = 128;
    while (max--) {
        if (spi_bb_byte() == 0xfe) {
            spi_bb_rx(dst, size);
            uint16_t crc;
            spi_bb_rx(&crc, 2);
            if (crc == jd_sd_crc16(dst, size))
                return 0;
            return -2;
        }
    }
    return -1;
}

static int sd_write_data(const void *data, unsigned size) {
    // add some padding and send start token
    spi_bb_tx("\xff\xff\xff\xff\xfe", 5);
    spi_bb_tx(data, size);
    uint16_t crc = jd_sd_crc16(data, size);
    spi_bb_tx(&crc, 2);
    return sd_get_resp();
}

static void cs_low(void) {
#ifdef JD_SD_CS_PULL_UP
    pin_setup_output(PIN_SD_CS);
#endif
    pin_set(PIN_SD_CS, 0);
}

static void cs_high(void) {
    pin_set(PIN_SD_CS, 1);
#ifdef JD_SD_CS_PULL_UP
    target_wait_us(5);
    pin_setup_input(PIN_SD_CS, PIN_PULL_UP);
#endif
}

static int sd_check(void) {
    int err = -1;
    cs_low();
    if (sd_send_cmd(9, 0) == 0) {
        uint8_t csd[16];
        if (sd_read_data(csd, sizeof(csd)) == 0)
            err = 0;
    }
    cs_high();
    return err;
}

static int sd_wait_busy(void) {
    int max = 10000;
    while (max--) {
        uint8_t b = spi_bb_byte();
        if (b == 0xff)
            return 0;
        target_wait_us(20);
    }
    return -1001;
}

static int sd_write_sector(uint32_t addr, const void *blk) {
    int err = -1000;
    cs_low();
    if (sd_send_cmd(24, addr) == 0) {
        int r = sd_write_data(blk, 512);
        if (r != 0xe5) {
            err = -r;
        } else {
            err = sd_wait_busy();
        }
    }
    cs_high();
    return err;
}

void jd_lstore_panic_flush(void) {
    jd_lstore_ctx_t *ctx = ls_ctx;
    if (!ctx || ctx->panic_mode != 1)
        return;
    for (int i = 0; i < JD_LSTORE_NUM_FILES; ++i) {
        jd_lstore_file_t *lf = &ctx->logs[i];
        if (i == 0 && ctx->panic_max_char_ptr > 0) {
            // force entry close
            jd_lstore_panic_print_char('\n');
        }
        flush_to_disk_in_panic(lf);
    }
}

void jd_lstore_panic_print_char(char ch) {
    jd_lstore_ctx_t *ctx = ls_ctx;
    if (!ctx || ctx->panic_mode > 1 || ch == '\r')
        return;

    if (!ctx->panic_mode) {
        ctx->panic_mode = 2;
        PANIC_LOG("init start");
        if (!ctx->logs[0].block) {
            PANIC_LOG("lstore invalid");
            return;
        }
        // wait a little for any SPI DMA activity to finish
        target_wait_us(8 << 10);
        // switch SPI to bit-bang mode
        spi_bb_init();

        pin_set(PIN_SD_CS, 1);
        target_wait_us(1 << 10);

        cs_low();
        int busy_cycles = 100;
        while (busy_cycles-- > 0 && pin_get(PIN_SD_MISO) == 0) {
            spi_bb_byte();
            target_wait_us(1 << 10);
        }
        cs_high();

        if (busy_cycles < 0) {
            PANIC_LOG("timeout busy");
            return;
        }

        if (sd_check() != 0) {
            PANIC_LOG("check failed");
            return;
        }

        ctx->panic_mode = 1; // OK!

        jd_lstore_panic_flush();

        PANIC_LOG("init OK");
    }

    if (ctx->panic_mode != 1)
        return;

    jd_lstore_file_t *f = &ctx->logs[0];

    bool is_endline = ch == '\n' && ctx->panic_char_ptr > 0;
    bool needs_flush = ctx->panic_char_ptr >= ctx->panic_max_char_ptr;

    if (needs_flush || is_endline) {
        if (ctx->panic_max_char_ptr > 0) {
            jd_lstore_entry_t ent = {
                .type = JD_LSTORE_TYPE_PANIC_LOG, .size = ctx->panic_char_ptr, .tdelta = 0};
            memcpy(block_curr_ptr(f), &ent, sizeof(ent));
            f->data_ptr += JD_LSTORE_ENTRY_HEADER_SIZE + ctx->panic_char_ptr;
        }

        // -8 is conservative
        int left = block_data_size(f) - f->data_ptr - JD_LSTORE_ENTRY_HEADER_SIZE - 8;

        // limit line breaking
        int min_data_len = block_size(f) >> 5;
        if (left < min_data_len) {
            flush_to_disk_in_panic(f);
            left = 0xff;
        }
        ctx->panic_char_ptr = 0;
        ctx->panic_max_char_ptr = left > 0xf0 ? 0xf0 : left;
    }

    if (!is_endline)
        block_curr_ptr(f)->data[ctx->panic_char_ptr++] = ch;
}

void jd_lstore_panic_print_str(const char *s) {
    while (*s)
        jd_lstore_panic_print_char(*s++);
}

#endif
#endif