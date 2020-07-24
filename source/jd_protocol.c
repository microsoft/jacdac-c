#include "jd_protocol.h"

void _jd_phys_start(void);

void jd_init()
{
    jd_alloc_init();
    jd_tx_init();
    jd_rx_init();
    jd_services_init();
    // if we don't start it explicitly, it will only start on incoming packet
    _jd_phys_start();
}