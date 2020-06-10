#include "jd_protocol.h"

void jd_init()
{
    tim_init();
    jd_alloc_init();
    uart_init();
    jd_ctrl_init();
    jd_tx_init();
    app_init_services();
}