#include "jd_drivers.h"
#include "jd_services.h"
#include "board.h"

#define MCP_41010_WRITE_DATA 0x10
#define MCP_41010_SHUTDOWN 0x20
#define MCP_41010_POT_CHAN_SEL_0 0x01
#define MCP_41010_POT_CHAN_SEL_1 0x02

static uint8_t mcp_41010_data[] = {0, 0};

static volatile int tx_done = 0;
static void done_handler(void) {
    tx_done = 1;
}

static void spi_tx(void) {
    pin_set(PIN_CS, 0);

    target_wait_us(100);
    tx_done = 0;
    dspi_tx(mcp_41010_data, 2, done_handler);
    while (tx_done == 0)
        ;
    target_wait_us(100);

    pin_set(PIN_CS, 1);
}

static void mcp41010_set_wiper(uint8_t channel, uint8_t wiper_value) {
    // mcp41010 only has two channels.
    JD_ASSERT(channel < 2);

    // DMESG("SET WIPER[%d]: %d", channel, wiper_value);

    mcp_41010_data[0] = MCP_41010_WRITE_DATA | 1 << channel;
    mcp_41010_data[1] = wiper_value;

    spi_tx();
}

static void mcp41010_shutdown(uint8_t channel) {
    // mcp41010 only has two channels.
    JD_ASSERT(channel < 2);
    mcp_41010_data[0] = MCP_41010_SHUTDOWN | 1 << channel;
    mcp_41010_data[1] = 0; // data is do not care

    spi_tx();
}

static void mcp41010_init(void) {
    dspi_init(true, 0, 0);
    pin_setup_output(PIN_CS);
    pin_set(PIN_CS, 1);
}

const potentiometer_api_t mcp41010 = {
    .init = mcp41010_init, .set_wiper = mcp41010_set_wiper, .shutdown = mcp41010_shutdown};