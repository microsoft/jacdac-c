#pragma once

void jd_rx_init(void);
int jd_rx_frame_received(jd_frame_t *frame) ;
jd_frame_t* jd_rx_get_frame(void);