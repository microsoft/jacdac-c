#pragma once

// This file specifies APIs that perform the routing from the physical layer to applications
//

extern const char app_dev_class_name[];

/**
 *
 **/
void app_process(void);
void app_init_services(void);
void app_handle_packet(jd_packet_t *pkt);
void app_process_frame(void);
void app_announce_services(void);
const char* app_get_device_class_name(void);
uint32_t app_get_device_class(void);