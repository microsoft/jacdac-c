#include "jd_drivers.h"

#ifdef PIN_CS
#include "services/jd_services.h"

#define MCP_41010_WRITE_DATA 0x10
#define MCP_41010_SHUTDOWN 0x20
#define MCP_41010_POT_CHAN_SEL_0 0x01
#define MCP_41010_POT_CHAN_SEL_1 0x02

static void spi_tx(uint8_t *data, uint32_t len) {
    pin_set(PIN_CS, 0);
    sspi_tx(data, len);
    pin_set(PIN_CS, 1);
}

static void mcp41010_set_wiper(uint8_t channel, uint32_t wiper_value) {
    // mcp41010 only has two channels.
    JD_ASSERT(channel < 2);

    uint8_t data[] = {0, 0};
    // DMESG("SET WIPER[%d]: %d", channel, wiper_value);

    data[0] = MCP_41010_WRITE_DATA | (1 << channel);
    data[1] = wiper_value;

    spi_tx(data, 2);
}

static void mcp41010_shutdown(uint8_t channel) {
    // mcp41010 only has two channels.
    JD_ASSERT(channel < 2);

    uint8_t data[] = {0, 0};

    data[0] = MCP_41010_SHUTDOWN | (1 << channel);
    data[1] = 0; // data is do not care

    spi_tx(data, 2);
}

static void mcp41010_init(void) {
    sspi_init(true, 0, 0);
    pin_setup_output(PIN_CS);
    pin_set(PIN_CS, 1);
}

const dig_pot_api_t mcp41010 = {
    .init = mcp41010_init, .set_wiper = mcp41010_set_wiper, .shutdown = mcp41010_shutdown};
#endif