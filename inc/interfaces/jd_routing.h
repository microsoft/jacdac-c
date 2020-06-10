#pragma once

// This file specifies APIs that perform the routing from the physical layer to applications
//

extern const char app_dev_class_name[];

/**
 *
 **/
void jd_services_tick(void);
void jd_services_init(void);
void jd_services_handle_packet(jd_packet_t *pkt);
void jd_services_process_frame(void);
void jd_services_announce(void);
uint32_t app_get_device_class(void);