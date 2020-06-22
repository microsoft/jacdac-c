#include "jd_protocol.h"

void jd_init()
{
    jd_alloc_init();
    jd_services_init();
    jd_ctrl_init();
    jd_tx_init();
    jd_rx_init();
    app_init_services();
}