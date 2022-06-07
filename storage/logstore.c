#include "jd_protocol.h"

#include "ff/ff.h"
#include "ff/diskio.h"

#define LOG_SIZE (16 * 1024 * 1024)
#define NUM_LOGS 1
#define SECTOR_SIZE 512

#define LOG(msg, ...) DMESG("sd: " msg, ##__VA_ARGS__)
#define CHK JD_CHK

typedef struct {
    struct jd_logstore_ctx *parent;
    uint32_t sector_off;
    uint32_t size;
} jd_logstore_file_t;

typedef struct jd_logstore_ctx {
    FATFS fs;
    jd_logstore_file_t logs[NUM_LOGS];
} jd_logstore_ctx_t;

static jd_logstore_ctx_t *ls_ctx;

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

static void list_files(jd_logstore_ctx_t *ctx) {
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

static void read_log(jd_logstore_file_t *f, uint32_t sector_addr, void *dst, uint32_t num_sectors) {
    jd_logstore_ctx_t *ctx = f->parent;
    CHK(ff_disk_read(ctx->fs.pdrv, dst, f->sector_off + sector_addr, num_sectors));
}

static void write_log(jd_logstore_file_t *f, uint32_t sector_addr, const void *src,
                      uint32_t num_sectors) {
    jd_logstore_ctx_t *ctx = f->parent;
    CHK(ff_disk_write(ctx->fs.pdrv, src, f->sector_off + sector_addr, num_sectors));
}

void jd_init_logstore(void) {
    jd_logstore_ctx_t *ctx = jd_alloc(sizeof(*ctx));
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
        jd_logstore_file_t *lf = &ctx->logs[i];
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
}