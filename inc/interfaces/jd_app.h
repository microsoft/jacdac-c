#pragma once

void app_process(void);
void app_init_services(void);
void app_handle_packet(jd_packet_t *pkt);
void app_process_frame(void);
void app_send_control(void);
int app_handle_frame(jd_frame_t *frame);