#include "jd_drivers.h"
#include "services/jd_services.h"

#define SPIFLASH_PAGE_SIZE 256

#if defined(SPI_RX) && defined(PIN_FLASH_CS)

#define check JD_ASSERT

// #define DBG DMESG
#define DBG(...) ((void)0)

static uint32_t _spiflash_num_bytes;

static int set_cmd(uint8_t *cmdBuf, int cmdcode, int addr) {
    cmdBuf[0] = cmdcode;
    if (addr == -1)
        return 1;
    cmdBuf[1] = addr >> 16;
    cmdBuf[2] = addr >> 8;
    cmdBuf[3] = addr >> 0;
    return 4;
}

static void send_cmd_ex(int cmdcode, int addr, void *resp, unsigned resp_size, unsigned skip) {
    pin_set(PIN_FLASH_CS, 0);
    uint8_t cmdBuf[4];
    sspi_tx(cmdBuf, set_cmd(cmdBuf, cmdcode, addr));
    cmdBuf[0] = 0;
    while (skip-- > 0)
        sspi_tx(cmdBuf, 1);
    if (resp_size)
        sspi_rx(resp, resp_size);
    pin_set(PIN_FLASH_CS, 1);
}

static void send_cmd(int cmdcode, int addr, void *resp, unsigned resp_size) {
    send_cmd_ex(cmdcode, addr, resp, resp_size, 0);
}

static void simple_cmd(int cmdcode, int addr) {
    send_cmd(cmdcode, addr, NULL, 0);
}

uint64_t spiflash_unique_id(void) {
    uint64_t r;
    send_cmd_ex(0x4B, -1, &r, sizeof(r), 4);
    return r;
}

static void spiflash_wait_busy(int waitMS) {
    uint8_t status;
    do {
        send_cmd(0x05, -1, &status, 1);
        // if (waitMS)
        //    target_wait_us(waitMS * 1000);
    } while (status & 0x01);
}

static void spiflash_write_enable(void) {
    simple_cmd(0x06, -1);
}

uint32_t spiflash_num_bytes(void) {
    return _spiflash_num_bytes;
}

typedef struct sfdp_header0 {
    uint8_t magic[4]; // "SFDP"
    uint8_t minor;
    uint8_t major;
    uint8_t num_headers;
    uint8_t unused;
} sfdp_header0_t;

typedef struct sfdp_header {
    uint8_t param_id;
    uint8_t minor;
    uint8_t major;
    uint8_t length; // in 32-bit words
    uint8_t pointer[3];
    uint8_t unused;
} sfdp_header_t;

void spiflash_read_sfdp(uint32_t addr, void *buffer, uint32_t len) {
    send_cmd_ex(0x5A, addr, buffer, len, 1);
}

static uint32_t get_num_bytes(void) {
    // TODO: send 0x9F command and return (1 << res[2]) - seems to work with everything
    sfdp_header0_t hd0;
    spiflash_read_sfdp(0, &hd0, sizeof(hd0));
    if (memcmp(hd0.magic, "SFDP", 4) != 0) {
        DMESG("SFDP: wrong magic");
        JD_PANIC();
    }
    sfdp_header_t hd;
    spiflash_read_sfdp(sizeof(hd0), &hd, sizeof(hd));
    uint32_t addr = (hd.pointer[2] << 16) | (hd.pointer[1] << 8) | hd.pointer[0];
    uint32_t v;
    spiflash_read_sfdp(addr + 4, &v, sizeof(v));

    return (v + 1) >> 3;
}

void spiflash_dump_sfdp(void) {
    uint64_t id = spiflash_unique_id();
    DMESG("flash ID %x %x", (unsigned)(id), (unsigned)(id >> 32));

    sfdp_header0_t hd0;
    spiflash_read_sfdp(0, &hd0, sizeof(hd0));
    if (memcmp(hd0.magic, "SFDP", 4) != 0) {
        DMESG("SFDP: wrong magic");
        return;
    }

    for (int i = 0; i <= hd0.num_headers; ++i) {
        sfdp_header_t hd;
        spiflash_read_sfdp(sizeof(hd0) + sizeof(hd) * i, &hd, sizeof(hd));
        uint32_t addr = (hd.pointer[2] << 16) | (hd.pointer[1] << 8) | hd.pointer[0];
        DMESG("SFDP: header %d id=%d ptr=%d len=%d", i, hd.param_id, (int)addr, hd.length);
        for (int j = 0; j < hd.length; ++j) {
            uint32_t v;
            spiflash_read_sfdp(addr + j * 4, &v, sizeof(v));
            DMESG("  %x", (unsigned)v);
        }
    }
}

void spiflash_read_bytes(uint32_t addr, void *buffer, uint32_t len) {
    check(addr + len <= _spiflash_num_bytes);
    check(addr <= _spiflash_num_bytes);
    send_cmd(0x03, addr, buffer, len);
}

void spiflash_write_bytes(uint32_t addr, const void *buffer, uint32_t len) {
    check(len <= SPIFLASH_PAGE_SIZE);
    check(addr / SPIFLASH_PAGE_SIZE == (addr + len - 1) / SPIFLASH_PAGE_SIZE);
    check(addr + len <= _spiflash_num_bytes);

    spiflash_write_enable();

    pin_set(PIN_FLASH_CS, 0);
    uint8_t cmdBuf[4];
    sspi_tx(cmdBuf, set_cmd(cmdBuf, 0x02, addr));
    sspi_tx(buffer, len);
    pin_set(PIN_FLASH_CS, 1);

    // the typical write time is under 1ms, so we don't bother with fiber_sleep()
    spiflash_wait_busy(0);
}

static void spiflash_erase_core(uint8_t cmd, uint32_t addr) {
    spiflash_write_enable();
    simple_cmd(cmd, addr);
    spiflash_wait_busy(10);
}

void spiflash_erase_4k(uint32_t addr) {
    check(addr < _spiflash_num_bytes);
    check((addr & (4 * 1024 - 1)) == 0);
    spiflash_erase_core(0x20, addr);
}

void spiflash_erase_32k(uint32_t addr) {
    check(addr < _spiflash_num_bytes);
    check((addr & (32 * 1024 - 1)) == 0);
    spiflash_erase_core(0x52, addr);
}

void spiflash_erase_64k(uint32_t addr) {
    check(addr < _spiflash_num_bytes);
    check((addr & (64 * 1024 - 1)) == 0);
    spiflash_erase_core(0xD8, addr);
}

void spiflash_erase_chip(void) {
    spiflash_erase_core(0xC7, -1);
}

static void set_high(int pin) {
    pin_set(pin, 1);
    pin_setup_output(pin);
}

void spiflash_init(void) {
    sspi_init(0, 0, 0);

    set_high(PIN_FLASH_CS);
    set_high(PIN_FLASH_HOLD);
    set_high(PIN_FLASH_WP);

#if 0
    spiflash_dump_sfdp();
#endif

    _spiflash_num_bytes = get_num_bytes();

    DMESG("SPI flash %d Mbytes", (int)((_spiflash_num_bytes + 511 * 1024) >> 20));
}

#endif