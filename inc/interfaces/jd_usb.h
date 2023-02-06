// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef JD_USB_H
#define JD_USB_H

#include "jd_protocol.h"

#if JD_USB_BRIDGE

// true if any packets are pending in the queue
bool jd_usb_is_pending(void);
// both return 0 on success
int jd_usb_write_serial(const void *data, unsigned len);
unsigned jd_usb_serial_space(void);
int jd_usb_send_frame(void *frame);
void jd_usb_enable_serial(void);
void jd_usb_panic_enter(void);
void jd_usb_panic_flush(void);
void jd_usb_proto_process(void);

// USB interface
// Defined by USB stack, called by jd_usb when there is new data to be pulled
void jd_usb_pull_ready(void);
// can be implemented by USB stack
bool jd_usb_looks_connected(void);
// can be re-defined by the USB stack, called from jd_process_everything()
void jd_usb_process(void);

// Called by the USB stack, returns number of bytes to send over USB (placed in dst[])
int jd_usb_pull(uint8_t dst[64]);
// called by USB stack to process incoming USB data
void jd_usb_push(const uint8_t *buf, unsigned len);


#endif

void jd_net_disable_fwd(void);

#endif
