#pragma once

#ifdef BL
#define DEVICE_CLASS(dev_class, dev_class_name)                                                    \
    struct bl_info_block __attribute__((section(".devinfo"), used)) bl_info = {                    \
        .devinfo =                                                                                 \
            {                                                                                      \
                .magic = DEV_INFO_MAGIC,                                                           \
                .device_id = 0xffffffffffffffffULL,                                                \
                .device_class = dev_class,                                                         \
            },                                                                                     \
        .random_seed0 = 0xffffffff,                                                                \
        .random_seed1 = 0xffffffff,                                                                \
        .reserved0 = 0xffffffff,                                                                   \
        .reserved1 = 0xffffffff,                                                                   \
    };
#else
#define DEVICE_CLASS(dev_class, dev_class_name) const char app_dev_class_name[] = dev_class_name;
#endif

void app_process(void);
void app_init_services(void);
void app_handle_packet(jd_packet_t *pkt);
void app_process_frame(void);
void app_announce_services(void);
int app_handle_frame(jd_frame_t *frame);
const char* app_get_device_class_name(void);
uint32_t app_get_device_class(void);