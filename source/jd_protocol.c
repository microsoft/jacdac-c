#include "jd_protocol.h"

void jd_init()
{
    tim_init();
    uart_init();
    ctrl_init();
    txq_init();
    app_init_services();
}