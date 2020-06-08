#include "jd_protocol.h"
#include "interfaces/jd_hw.h"
#include "interfaces/jd_app.h"
#include "interfaces/jd_tx.h"
#include "jd_services.h"

void jd_init()
{
    tim_init();
    uart_init();
    jd_ctrl_init();
    jd_tx_init();
    app_init_services();
}