// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef JD_CONFIG_H
#define JD_CONFIG_H

#include "jd_user_config.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#define JD_NOLOG(...) ((void)0)

// LOG(...) to be defined in particular .c files as either JD_LOG or JD_NOLOG

#if (__SIZEOF_POINTER__ == 8) || (__WORDSIZE == 64)
#define JD_64 1
#define JD_PTRSIZE 8
#elif (__SIZEOF_POINTER__ == 4) || (__WORDSIZE == 32)
#define JD_64 0
#define JD_PTRSIZE 4
#else
#error "can't determine pointer size"
#endif

#ifndef JD_CONFIG_TEMPERATURE
#define JD_CONFIG_TEMPERATURE 0
#endif

#ifndef JD_CONFIG_CONTROL_FLOOD
#define JD_CONFIG_CONTROL_FLOOD 1
#endif

#ifndef JD_CONFIG_WATCHDOG
#define JD_CONFIG_WATCHDOG 1
#endif

#ifndef JD_CONFIG_STATUS
#define JD_CONFIG_STATUS 1
#endif

#ifndef JD_CONFIG_IGNORE_STATUS
#define JD_CONFIG_IGNORE_STATUS 0
#endif

#ifndef JD_CONFIG_DEV_SPEC_URL
#define JD_CONFIG_DEV_SPEC_URL 0
#endif

#ifndef JD_RAW_FRAME
#define JD_RAW_FRAME 0
#endif

#ifndef JD_SIGNAL_RDWR
#define JD_SIGNAL_RDWR 0
#endif

#define CONCAT_1(a, b) a##b
#define CONCAT_0(a, b) CONCAT_1(a, b)

#ifndef STATIC_ASSERT
#define STATIC_ASSERT(e) enum { CONCAT_0(_static_assert_, __LINE__) = 1 / ((e) ? 1 : 0) };
#endif

#ifndef STATIC_ASSERT_EXT
#define STATIC_ASSERT_EXT(e, id)                                                                   \
    enum { CONCAT_0(_static_assert_, CONCAT_0(__LINE__, id)) = 1 / ((e) ? 1 : 0) };
#endif

// if running off external flash, put this function in RAM
#ifndef JD_FAST_FUNC
#define JD_FAST_FUNC /* */
#endif

#ifndef JD_EVENT_QUEUE_SIZE
#define JD_EVENT_QUEUE_SIZE 128
#endif

#ifndef JD_TIM_OVERHEAD
#define JD_TIM_OVERHEAD 12
#endif

// this is timing overhead (in us) of starting transmission
// see set_tick_timer() for how to calibrate this
#ifndef JD_WR_OVERHEAD
#define JD_WR_OVERHEAD 8
#endif

#ifndef JD_PHYSICAL
#define JD_PHYSICAL 1
#endif

#ifndef JD_CLIENT
#define JD_CLIENT 0
#endif

// pipes, jd_free(), jd_send_frame() by default on in client, off in server
#ifndef JD_PIPES
#define JD_PIPES JD_CLIENT
#endif

#ifndef JD_FREE_SUPPORTED
#define JD_FREE_SUPPORTED JD_CLIENT
#endif

#ifndef JD_DEVICESCRIPT
#define JD_DEVICESCRIPT JD_CLIENT
#endif

#ifndef JD_USB_BRIDGE
#define JD_USB_BRIDGE 0
#endif

#ifndef JD_NET_BRIDGE
#define JD_NET_BRIDGE 0
#endif

#ifndef JD_BRIDGE
#define JD_BRIDGE JD_USB_BRIDGE
#endif

#ifndef JD_USB_BRIDGE_SEND
#if JD_USB_BRIDGE
int jd_usb_send_frame(void *frame);
#define JD_USB_BRIDGE_SEND(f) jd_usb_send_frame(f)
#else
#define JD_USB_BRIDGE_SEND(...) ((void)0)
#endif
#endif

#ifndef JD_NET_BRIDGE_SEND
#if JD_NET_BRIDGE
int jd_net_send_frame(void *frame);
#define JD_NET_BRIDGE_SEND(f) jd_net_send_frame(f)
#else
#define JD_NET_BRIDGE_SEND(...) ((void)0)
#endif
#endif

#ifndef JD_BRIDGE_SEND
#define JD_BRIDGE_SEND(f)                                                                          \
    do {                                                                                           \
        JD_USB_BRIDGE_SEND(f);                                                                     \
        JD_NET_BRIDGE_SEND(f);                                                                     \
    } while (0)
#endif

#ifndef JD_SEND_FRAME
#define JD_SEND_FRAME (JD_CLIENT || JD_BRIDGE)
#endif

#ifndef JD_RX_QUEUE
#define JD_RX_QUEUE JD_SEND_FRAME
#endif

#if JD_DEVICESCRIPT

#ifndef JD_SEND_FRAME_SIZE
#define JD_SEND_FRAME_SIZE 1024
#endif

#ifndef JD_RX_QUEUE_SIZE
#define JD_RX_QUEUE_SIZE 2048
#endif

#ifndef JD_USB_QUEUE_SIZE
#define JD_USB_QUEUE_SIZE 2048
#endif

#else

#ifndef JD_SEND_FRAME_SIZE
#define JD_SEND_FRAME_SIZE 512
#endif

#ifndef JD_RX_QUEUE_SIZE
#define JD_RX_QUEUE_SIZE 512
#endif

#ifndef JD_USB_QUEUE_SIZE
#define JD_USB_QUEUE_SIZE 1024
#endif

#endif

#ifndef JD_LORA
#define JD_LORA 0
#endif

#ifndef JD_VERBOSE_ASSERT
#define JD_VERBOSE_ASSERT (JD_CLIENT || JD_LORA || JD_64)
#endif

#ifndef JD_ADVANCED_STRING
#define JD_ADVANCED_STRING (JD_CLIENT || JD_LORA || JD_64)
#endif

#ifndef JD_MS_TIMER
#define JD_MS_TIMER JD_CLIENT
#endif

#ifndef JD_LSTORE
#define JD_LSTORE JD_CLIENT
#endif

#ifndef JD_HAS_PWM_ENABLE
#define JD_HAS_PWM_ENABLE 1
#endif

#if JD_CONFIG_STATUS && defined(PIN_LED_R)
#define JD_COLOR_STATUS 1
#else
#define JD_COLOR_STATUS 0
#endif

#ifndef JD_LED_ERRORS
#define JD_LED_ERRORS JD_COLOR_STATUS
#endif

#if JD_LED_ERRORS
#define JD_ERROR_BLINK(x) jd_blink(x)
#else
#define JD_ERROR_BLINK(x) ((void)0)
#endif

#ifndef JD_WAKE_MAIN
#define JD_WAKE_MAIN() ((void)0)
#endif

#ifndef JD_INSTANCE_NAME
#define JD_INSTANCE_NAME 0
#endif

#ifndef JD_HANDLE_ALL_PACKETS
#define JD_HANDLE_ALL_PACKETS JD_DEVICESCRIPT
#endif

#define ERROR(msg, ...)                                                                            \
    do {                                                                                           \
        JD_LOG("! " msg, ##__VA_ARGS__);                                                           \
        JD_ERROR_BLINK(JD_BLINK_ERROR_GENERAL);                                                    \
    } while (0)

#define LINE_ERROR(msg, ...)                                                                       \
    do {                                                                                           \
        JD_LOG("! " msg, ##__VA_ARGS__);                                                           \
        JD_ERROR_BLINK(JD_BLINK_ERROR_LINE);                                                       \
    } while (0)
#define OVF_ERROR(msg, ...)                                                                        \
    do {                                                                                           \
        JD_LOG("! " msg, ##__VA_ARGS__);                                                           \
        JD_ERROR_BLINK(JD_BLINK_ERROR_OVF);                                                        \
    } while (0)

// Simple alloc doesn't support jd_free()
#ifndef JD_SIMPLE_ALLOC
#define JD_SIMPLE_ALLOC (!JD_FREE_SUPPORTED)
#endif

// If enabled, system memory allocator is not used
#ifndef JD_HW_ALLOC
#define JD_HW_ALLOC JD_SIMPLE_ALLOC
#endif

// Whether to implement jd_alloc() as allocating in the GC heap
#ifndef JD_GC_ALLOC
#define JD_GC_ALLOC (JD_HW_ALLOC && !JD_SIMPLE_ALLOC)
#endif

// If not JD_HW_ALLOC, how big should be the GC block(s)
#ifndef JD_GC_KB
#define JD_GC_KB 64
#endif

#ifndef JD_AES_SOFT
#define JD_AES_SOFT 1
#endif

#ifndef JD_SHORT_FLOAT
#define JD_SHORT_FLOAT 0
#endif

#ifndef JD_THR_PTHREAD
#define JD_THR_PTHREAD 0
#endif

#ifndef JD_THR_AZURE_RTOS
#define JD_THR_AZURE_RTOS 0
#endif

#ifndef JD_THR_FREE_RTOS
#define JD_THR_FREE_RTOS 0
#endif

#define JD_THR_ANY (JD_THR_PTHREAD || JD_THR_AZURE_RTOS || JD_THR_FREE_RTOS)

// settings stuff
#ifndef JD_SETTINGS_LARGE
#define JD_SETTINGS_LARGE JD_DEVICESCRIPT
#endif

#ifndef JD_FSTOR_HEADER_PAGES
// if JD_SETTINGS_LARGE, this is just the minimum
#define JD_FSTOR_HEADER_PAGES 1
#endif

#ifndef JD_FSTOR_TOTAL_SIZE
#if JD_SETTINGS_LARGE
#define JD_FSTOR_TOTAL_SIZE (128 * 1024)
#else
#define JD_FSTOR_TOTAL_SIZE (JD_FSTOR_HEADER_PAGES * 2 * JD_FLASH_PAGE_SIZE)
#endif
#endif

#ifndef JD_WIFI
#define JD_WIFI 0
#endif

#ifndef JD_NETWORK
#define JD_NETWORK JD_WIFI
#endif

#ifndef JD_DMESG_BUFFER_SIZE
#ifdef DEVICE_DMESG_BUFFER_SIZE
#define JD_DMESG_BUFFER_SIZE DEVICE_DMESG_BUFFER_SIZE
#else
#define JD_DMESG_BUFFER_SIZE 1024
#endif
#endif

#ifndef JD_DMESG_LINE_BUFFER
#define JD_DMESG_LINE_BUFFER (JD_ADVANCED_STRING ? 160 : 80)
#endif

#ifndef JD_FAST
#define JD_FAST /* */
#endif

#ifndef JD_DCFG
#ifdef JD_DCFG_BASE_ADDR
#define JD_DCFG 1
#else
#define JD_DCFG 0
#endif
#endif

#ifndef JD_MAX_SERVICES
#define JD_MAX_SERVICES 32
#endif

// hosted means running on a desktop-like machine (either native or with WASM)
#ifndef JD_HOSTED
#define JD_HOSTED (!JD_PHYSICAL)
#endif

#ifndef JD_I2C_HELPERS
#define JD_I2C_HELPERS 0
#endif

#ifndef JD_SD_PANIC
#define JD_SD_PANIC (JD_LSTORE && !JD_HOSTED)
#endif

#ifndef JD_HID
#define JD_HID 0
#endif

#ifndef JD_ANALOG
#define JD_ANALOG (!JD_HOSTED)
#endif

#ifndef JD_SPI
#define JD_SPI 0
#endif


#endif