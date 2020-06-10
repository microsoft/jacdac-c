#pragma once

extern const char app_dev_class_name[];

void app_process(void);
void app_init_services(void);
void app_handle_packet(jd_packet_t *pkt);
void app_process_frame(void);
void app_announce_services(void);
int app_handle_frame(jd_frame_t *frame);
const char* app_get_device_class_name(void);
uint32_t app_get_device_class(void);