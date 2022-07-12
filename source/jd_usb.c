#include "interfaces/jd_usb.h"

/*

Packets are sent over USB Serial (CDC).
The host shall set the CDC to 2Mbaud 8N1
(even though in some cases the USB interface is connected directly to the MCU and line settings are ignored).

The CDC line carries both Jacdac frames and serial logging output.
Jacdac frames have valid CRC and are framed by delimeter characters and possibly fragmented.

0xFE is used as a framing byte.
Note that bytes 0xF8-0xFF are always invalid UTF-8.
0xFF occurs relatively often in Jacdac packets.

The following sequences are supported:

0xFE 0xF9 - literal 0xFE
0xFE 0xFA - Jacdac frame start (BEGIN)
0xFE 0xFB - Jacdac frame fragment end (STOP)
0xFE 0xFC - Jacdac frame continuation start (CONT)
0xFE 0xFD - Jacdac frame final end (END)

Frames are either sent in one piece as:
BEGIN...END
Or they can be fragmented as follows:
BEGIN...STOP (CONT...STOP)* CONT...END

0xFE followed by any other byte:
* in serial, should be treated as literal 0xFE (and the byte as itself, unless it's 0xFE)
* in frame data, should terminate the current frame fragment,
  and ideally have all data (incl. fragment start) in the current frame fragment treated as serial

Maximum size of bytes in fragment is 64 including inital and final framing and quoting of bytes
inside.

*/

#if JD_USB_BRIDGE

#define JD_USB_CMD_DISABLE_PKTS 0x0080
#define JD_USB_CMD_ENABLE_PKTS 0x0081
#define JD_USB_CMD_NOT_UNDERSTOOD 0x00ff

#define JD_QBYTE_MAGIC 0xFE
#define JD_QBYTE_LITERAL_MAGIC 0xF9
#define JD_QBYTE_BEGIN 0xFA
#define JD_QBYTE_STOP 0xFB
#define JD_QBYTE_CONT 0xFC
#define JD_QBYTE_END 0xFD

#define JD_QBYTE_MAX_SIZE 64

static uint8_t usb_fwd_en;
static uint8_t usb_frame_ptr;
static jd_queue_t usb_queue;

static uint8_t usb_rx_ptr;
static uint8_t usb_rx_state;
static uint8_t usb_rx_was_magic;
static jd_frame_t usb_rx_buf;

#define USB_ERROR(msg, ...) ERROR("USB: " msg, ##__VA_ARGS__)

static int jd_usb_send(const uint8_t *data, unsigned len, unsigned space, uint8_t start_token) {
    uint8_t dst[JD_QBYTE_MAX_SIZE];
    unsigned sp = 0;
    unsigned dp = 0;
    dst[dp++] = JD_QBYTE_MAGIC;
    dst[dp++] = start_token;
    while (sp < len && dp < space - 2) {
        uint8_t c = data[sp];
        if (c == JD_QBYTE_MAGIC) {
            if (dp >= space - 3)
                break;
            dst[dp++] = JD_QBYTE_MAGIC;
            dst[dp++] = JD_QBYTE_LITERAL_MAGIC;
        } else {
            dst[dp++] = c;
        }
        sp++;
    }
    JD_ASSERT(dp <= space - 2);
    dst[dp++] = JD_QBYTE_MAGIC;
    dst[dp++] = sp == len ? JD_QBYTE_END : JD_QBYTE_STOP;

    if (jd_usb_tx(dst, dp) != 0) {
        // send failed; maybe someone else was sending as well?
        return 0;
    }

    return sp;
}

static void jd_usb_init(void) {
    if (!usb_queue) {
        usb_queue = jd_queue_alloc(JD_USB_QUEUE_SIZE);
        usb_frame_ptr = 0;
    }
}

bool jd_usb_is_pending(void) {
    return jd_queue_front(usb_queue) != NULL;
}

static void jd_usb_serial_cb(uint8_t b) {}

static int jd_usb_respond_to_processing_packet(uint16_t service_command) {
    jd_packet_t pkt;
    memset(&pkt, 0xff, sizeof(pkt));
    pkt._size = 4;
    pkt.service_size = 0;
    pkt.service_command = service_command;
    jd_frame_t *frame = (jd_frame_t *)&pkt;
    jd_compute_crc(frame);
    JD_WAKE_MAIN();
    return jd_queue_push(usb_queue, frame);
}

static void jd_usb_handle_processing_packet(jd_packet_t *pkt) {
    jd_usb_init();
    uint16_t cmd = pkt->service_command;
    switch (cmd) {
    case JD_USB_CMD_ENABLE_PKTS:
    case JD_USB_CMD_DISABLE_PKTS:
        usb_fwd_en = cmd & 1;
        jd_usb_respond_to_processing_packet(cmd);
        break;
    default:
        jd_usb_respond_to_processing_packet(JD_USB_CMD_NOT_UNDERSTOOD);
        break;
    }
}

static void jd_usb_handle_frame(void) {
    jd_frame_t *frame = &usb_rx_buf;
    uint32_t txSize = usb_rx_ptr;
    uint32_t declaredSize = JD_FRAME_SIZE(frame);
    if (txSize < declaredSize) {
        USB_ERROR("short frm");
        return;
    }

    uint16_t crc = jd_crc16((uint8_t *)frame + 2, declaredSize - 2);
    if (crc != frame->crc) {
        USB_ERROR("crc err");
        return;
    }

    if (frame->flags == 0xff && !(frame->device_identifier + 1)) {
        jd_usb_handle_processing_packet((jd_packet_t *)frame);
    } else {
        jd_rx_frame_received_loopback(frame);
    }
}

void jd_usb_process_rx(void) {
    uint8_t *rxbuf = (uint8_t *)&usb_rx_buf;
    for (;;) {
        uint8_t buf[64];
        int len = jd_usb_rx(buf, sizeof(buf));
        if (len <= 0)
            break;
        for (unsigned i = 0; i < len; ++i) {
            uint8_t c = buf[i];
            if (usb_rx_was_magic) {
                if (c == JD_QBYTE_MAGIC) {
                    if (usb_rx_state) {
                        USB_ERROR("dual magic");
                        // break out of frame state
                        usb_rx_state = 0;
                        // pass on accumulated data as serial
                        jd_usb_serial_cb(JD_QBYTE_MAGIC);
                        jd_usb_serial_cb(JD_QBYTE_BEGIN);
                        for (unsigned j = 0; j < usb_rx_ptr; ++j) {
                            jd_usb_serial_cb(rxbuf[j]);
                        }
                    }
                    // 'c' will be passed to jd_usb_serial_cb() below
                } else {
                    usb_rx_was_magic = 0;
                    switch (c) {
                    case JD_QBYTE_LITERAL_MAGIC:
                        c = JD_QBYTE_MAGIC;
                        break;
                    case JD_QBYTE_BEGIN:
                        if (usb_rx_ptr)
                            USB_ERROR("second begin");
                        usb_rx_ptr = 0;
                        // fallthrough
                    case JD_QBYTE_CONT:
                        if (usb_rx_state)
                            USB_ERROR("unfinished beg/cont");
                        usb_rx_state = c;
                        continue;
                    case JD_QBYTE_STOP:
                    case JD_QBYTE_END:
                        if (usb_rx_state) {
                            usb_rx_state = 0;
                            if (c == JD_QBYTE_END) {
                                jd_usb_handle_frame();
                                usb_rx_ptr = 0;
                            }
                        } else {
                            USB_ERROR("mismatched stop");
                        }
                        continue;
                    default:
                        if (usb_rx_state) {
                            USB_ERROR("invalid quote");
                            usb_rx_state = 0;
                        }
                        // either way, pass on directly
                        jd_usb_serial_cb(JD_QBYTE_MAGIC);
                        // c = c;
                        break;
                    }
                }
            } else if (c == JD_QBYTE_MAGIC) {
                usb_rx_was_magic = 1;
                continue;
            }

            if (usb_rx_state) {
                if (usb_rx_ptr >= sizeof(usb_rx_buf)) {
                    USB_ERROR("frame ovf");
                    usb_rx_state = 0;
                } else {
                    rxbuf[usb_rx_ptr++] = c;
                }
            } else {
                jd_usb_serial_cb(c);
            }
        }
    }
}

void jd_usb_process_tx(void) {
    if (!usb_queue)
        return;

    int needs_flush = 0;

    jd_frame_t *f;

    for (;;) {
        f = jd_queue_front(usb_queue);
        if (!f)
            break;
        int space = jd_usb_tx_free_space();
        if (space < 8) {
            // no space? try flushing and checking again
            jd_usb_tx_flush();
            space = jd_usb_tx_free_space();
            if (space < 8) {
                needs_flush = 0; // just flushed
                break;
            }
        }
        int frame_size = JD_FRAME_SIZE(f);
        int to_send = frame_size - usb_frame_ptr;
        JD_ASSERT(to_send > 0);
        int off = jd_usb_send((uint8_t *)f + usb_frame_ptr, to_send, space,
                              usb_frame_ptr == 0 ? JD_QBYTE_BEGIN : JD_QBYTE_CONT);
        // DMESG("send l=%d off=%d sp=%d uf=%d", to_send, off, space, usb_frame_ptr);
        if (off > 0)
            needs_flush = 1;
        usb_frame_ptr += off;
        if (usb_frame_ptr == frame_size) {
            jd_queue_shift(usb_queue);
            usb_frame_ptr = 0;
        }
    }

    if (needs_flush)
        jd_usb_tx_flush();
}

int jd_usb_send_frame(void *frame) {
    if (!usb_fwd_en)
        return 0;
    JD_WAKE_MAIN();
    // this is called from an ISR
    return jd_queue_push(usb_queue, frame);
}

#endif