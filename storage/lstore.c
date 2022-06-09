#include "jd_protocol.h"

#include "ff/ff.h"
#include "ff/diskio.h"

#define LOG_SIZE (16 * 1024 * 1024)
#define NUM_LOGS 1
#define SECTOR_SHIFT 9
#define SECTOR_SIZE (1 << SECTOR_SHIFT)

#define LOG(msg, ...) DMESG("sd: " msg, ##__VA_ARGS__)
#define CHK JD_CHK

#define JD_LSTORE_MAGIC0 0x0a4c444a
#define JD_LSTORE_MAGIC1 0xb5d1841e
#define JD_LSTORE_VERSION 2

typedef struct {
    uint32_t magic0;
    uint32_t magic1;
    uint32_t version;
    uint32_t sector_size;       // currently always 512
    uint32_t sectors_per_block; // typically 8
    uint32_t header_blocks;     // typically 1
    uint32_t num_blocks;        // including header block
    uint32_t num_rewrites;      // how many times the whole log was rewritten (wrapped-around)
    uint32_t block_magic0;      // used in all blocks in this file
    uint32_t block_magic1;      // used in all blocks in this file
    uint32_t reserved[6];

    // meta-data about device
    uint64_t device_id;
    char firmware_name[64];
    char firmware_version[32];
    uint8_t reserved_dev[32];

    // meta-data about the log file
    char purpose[32];
    char comment[64];
    uint8_t reserved_log[32];
} jd_lstore_main_header_t;

typedef struct {
    uint32_t block_magic0;
    uint32_t block_magic1;
    uint32_t crc32;
} jd_lstore_block_header_t;

STATIC_ASSERT(sizeof(jd_lstore_main_header_t) <= SECTOR_SIZE);

typedef struct {
    struct jd_lstore_ctx *parent;
    uint32_t sector_off;
    uint32_t size;
    uint32_t num_blocks;
    uint8_t block_shift; // block_size == 1 << block_shift
} jd_lstore_file_t;

typedef struct jd_lstore_ctx {
    FATFS fs;
    uint8_t tmpbuf[SECTOR_SIZE];
    jd_lstore_file_t logs[NUM_LOGS];
} jd_lstore_ctx_t;

static jd_lstore_ctx_t *ls_ctx;

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

static void read_log(jd_lstore_file_t *f, uint32_t sector_addr, void *dst, uint32_t num_sectors) {
    jd_lstore_ctx_t *ctx = f->parent;
    CHK(ff_disk_read(ctx->fs.pdrv, dst, f->sector_off + sector_addr, num_sectors));
}

static void write_log(jd_lstore_file_t *f, uint32_t sector_addr, const void *src,
                      uint32_t num_sectors) {
    jd_lstore_ctx_t *ctx = f->parent;
    CHK(ff_disk_write(ctx->fs.pdrv, src, f->sector_off + sector_addr, num_sectors));
}

static bool validate_header(jd_lstore_file_t *f, jd_lstore_main_header_t *hd) {
    if (hd->magic0 == JD_LSTORE_MAGIC0 && hd->magic1 == JD_LSTORE_MAGIC1) {
        if (hd->version != JD_LSTORE_VERSION) {
            LOG("wrong version %x", hd->version);
            return 0;
        }
        if (hd->sector_size != SECTOR_SIZE) {
            LOG("invalid sector size %d", hd->sector_size);
            return 0;
        }

        if (hd->sectors_per_block * hd->num_blocks > f->size / SECTOR_SIZE) {
            LOG("file too small for %d*%d sectors", hd->sectors_per_block, hd->num_blocks);
            return 0;
        }

        int sh = 0;
        while ((1 << sh) < hd->sectors_per_block)
            sh++;
        if (hd->sectors_per_block > 8 || (1 << sh) != hd->sectors_per_block) {
            LOG("invalid sector/block %d", hd->sectors_per_block);
            return 0;
        }

        f->block_shift = sh + SECTOR_SHIFT;
        f->num_blocks = hd->num_blocks;

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

static void mount_log(jd_lstore_file_t *f, const char *name) {
    jd_lstore_ctx_t *ctx = f->parent;
    read_log(f, 0, ctx->tmpbuf, 1);
    jd_lstore_main_header_t *hd = (void *)ctx->tmpbuf;
    if (!validate_header(f, hd)) {
        memset(ctx->tmpbuf, 0, SECTOR_SIZE);
        hd->magic0 = JD_LSTORE_MAGIC0;
        hd->magic1 = JD_LSTORE_MAGIC1;
        hd->version = JD_LSTORE_VERSION;
        hd->sector_size = SECTOR_SIZE;
        hd->sectors_per_block = (1 << 3);
        f->block_shift = 3 + SECTOR_SHIFT;
        hd->num_blocks = f->size >> f->block_shift;
        f->num_blocks = hd->num_blocks;
        hd->block_magic0 = jd_random();
        hd->block_magic1 = jd_random();
        hd->device_id = jd_device_id();
        STRCPY(hd->firmware_name, app_dev_class_name);
        STRCPY(hd->firmware_version, app_fw_version);
        STRCPY(hd->purpose, name);
        LOG("writing new header for '%s'", name);
        write_log(f, 0, hd, 1);
    }

    hd->purpose[sizeof(hd->purpose) - 1] = 0;
    LOG("mounted '%s' (%dkB) sh=%d", hd->purpose, f->num_blocks << f->block_shift >> 10,
        f->block_shift);
}

void jd_init_lstore(void) {
    jd_lstore_ctx_t *ctx = jd_alloc(sizeof(*ctx));
    ls_ctx = ctx;

    const char *drv = "0:";

    FRESULT res = f_mount(&ctx->fs, drv, 1);
    if (res != FR_OK) {
        DMESG("failed to mount card (%d)", res);
        jd_panic();
    } else {
        uint32_t kb = (ctx->fs.n_fatent >> 1) * ctx->fs.csize;
        LOG("mounted! %dMB", kb >> 10);
    }

    list_files(ctx); // TODO remove me

    FIL *fil = jd_alloc(sizeof(FIL));

    for (int i = 0; i < NUM_LOGS; ++i) {
        jd_lstore_file_t *lf = &ctx->logs[i];
        lf->parent = ctx;
        char *fn = jd_sprintf_a("0:/LOG_%d.JDL", i);

        CHK(f_open(fil, fn, FA_OPEN_ALWAYS | FA_READ | FA_WRITE));

        int contchk = FR_NO_FILE;

        if (fil->obj.objsize >= LOG_SIZE) {
            contchk = check_contiguous_file(fil);
        }

        const char *st = "OK";

        if (contchk != 0) {
            LOG("will create r=%d st=%d sz=%d", contchk, fil->obj.sclust, fil->obj.objsize);
            CHK(f_close(fil));
            CHK(f_open(fil, fn, FA_CREATE_ALWAYS | FA_READ | FA_WRITE));
            CHK(f_expand(fil, LOG_SIZE, 1));
            st = "created";
        }

        LOG("%s st=%d sz=%d %s", fn, fil->obj.sclust, fil->obj.objsize, st);

        lf->size = fil->obj.objsize;
        lf->sector_off = ctx->fs.database + ctx->fs.csize * (fil->obj.sclust - 2);

        CHK(f_close(fil));
        jd_free(fn);
    }

    jd_free(fil);

    mount_log(&ctx->logs[0], "packets and serial");
}