// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef JD_USB_H
#define JD_USB_H

#include "jd_protocol.h"

#if JD_USB_BRIDGE

int jd_usb_send_frame(void *frame);
// true if any packets are pending in the queue
bool jd_usb_is_pending(void);
// call from RX ISR
void jd_usb_process_rx(void);
// called from jd_process_everything()
void jd_usb_process_tx(void);

// Defined by application:
// estimate how much data next jd_usb_tx() can send
int jd_usb_tx_free_space(void);
// returns 0 on success
int jd_usb_tx(const void *data, unsigned len);
// indicate there will be no more jd_usb_tx() soon
int jd_usb_tx_flush(void);
// returns number of bytes read
int jd_usb_rx(void *data, unsigned len);

#endif

#endif
