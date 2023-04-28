#include "interfaces/jd_usb.h"
#include "jacdac/dist/c/usbbridge.h"
#include "storage/jd_storage.h"

#if JD_USB_BRIDGE

static uint8_t usb_fwd_en;
static uint8_t usb_serial_en;
static uint8_t usb_frame_ptr;
static uint8_t usb_frame_gap;

static uint8_t usb_panic_mode;
static uint8_t serial_buf_ptr;
static uint8_t serial_buf_len;
static uint8_t serial_buf[64];

static jd_queue_t usb_queue;

static uint8_t usb_rx_ptr;
static uint8_t usb_rx_state;
static uint8_t usb_rx_was_magic;
static jd_frame_t usb_rx_buf;

static bool usb_is_connected;
static uint32_t dmesg_timer;
static uint32_t dmesg_ptr;

#define USB_ERROR(msg, ...) ERROR("USB: " msg, ##__VA_ARGS__)

#define SPACE() (64 - dp)
int jd_usb_pull(uint8_t dst[64]) {
    if (usb_panic_mode) {
        if (usb_panic_mode > 1)
            return 0;
        int len = serial_buf_ptr;
        serial_buf_ptr = 0;
        memcpy(dst, serial_buf, len);
        return len;
    }

    if (!usb_queue)
        return 0;

    const jd_frame_t *f;
    int dp = 0;

    for (;;) {
        if (usb_frame_gap == 1 && usb_frame_ptr == 0 && SPACE() >= 2) {
            usb_frame_gap = 2;
            dst[dp++] = JD_USB_BRIDGE_QBYTE_MAGIC;
            dst[dp++] = JD_USB_BRIDGE_QBYTE_FRAME_GAP;
        }

        f = jd_queue_front(usb_queue);
        if (!f)
            break;

        int frame_size = JD_FRAME_SIZE(f);

        if (usb_frame_ptr == 0) {
            if (SPACE() < 4)
                break;

            dst[dp++] = JD_USB_BRIDGE_QBYTE_MAGIC;
            dst[dp++] = JD_USB_BRIDGE_QBYTE_FRAME_START;
        }

        while (usb_frame_ptr < frame_size) {
            uint8_t b = ((const uint8_t *)f)[usb_frame_ptr];
            if (b == JD_USB_BRIDGE_QBYTE_MAGIC) {
                if (SPACE() < 2)
                    break;
                dst[dp++] = JD_USB_BRIDGE_QBYTE_MAGIC;
                dst[dp++] = JD_USB_BRIDGE_QBYTE_LITERAL_MAGIC;
            } else {
                if (SPACE() < 1)
                    return dp;
                dst[dp++] = b;
            }
            usb_frame_ptr++;
        }

        if (SPACE() < 2)
            break;

        dst[dp++] = JD_USB_BRIDGE_QBYTE_MAGIC;
        dst[dp++] = JD_USB_BRIDGE_QBYTE_FRAME_END;

        usb_frame_ptr = 0;
        jd_queue_shift(usb_queue);
    }

    if (dmesg_timer == 1 && !usb_panic_mode) {
        while (SPACE() >= 2) {
            if (serial_buf_ptr >= serial_buf_len) {
                serial_buf_len = jd_dmesg_read(serial_buf, sizeof(serial_buf), &dmesg_ptr);
                serial_buf_ptr = 0;
            }
            if (serial_buf_ptr >= serial_buf_len)
                break;

            uint8_t b = serial_buf[serial_buf_ptr++];
            if (b == JD_USB_BRIDGE_QBYTE_MAGIC) {
                dst[dp++] = JD_USB_BRIDGE_QBYTE_MAGIC;
                dst[dp++] = JD_USB_BRIDGE_QBYTE_LITERAL_MAGIC;
            } else {
                dst[dp++] = b;
            }
        }
    }

    return dp;
}

static void jd_usb_init(void) {
    if (!usb_queue) {
        usb_queue = jd_queue_alloc(JD_USB_QUEUE_SIZE);
        usb_frame_ptr = 0;
    }
}

bool jd_usb_is_pending(void) {
    return usb_queue && (jd_queue_front(usb_queue) != NULL || serial_buf_ptr < serial_buf_len);
}

static void jd_usb_serial_cb(uint8_t b) {}

static int jd_usb_respond_to_processing_packet(uint16_t service_command) {
    jd_packet_t pkt;
    memset(&pkt, 0, sizeof(pkt));
    jd_pkt_set_broadcast(&pkt, JD_SERVICE_CLASS_USB_BRIDGE);
    pkt._size = 4;
    pkt.service_command = service_command;
    jd_frame_t *frame = (jd_frame_t *)&pkt;
    jd_compute_crc(frame);
    JD_WAKE_MAIN();
    int r = jd_queue_push(usb_queue, frame);
    jd_usb_pull_ready();
    return r;
}

static void jd_usb_handle_processing_packet(jd_packet_t *pkt) {
    jd_usb_init();
    uint16_t cmd = pkt->service_command;
    switch (cmd) {
    case JD_USB_BRIDGE_CMD_ENABLE_PACKETS:
    case JD_USB_BRIDGE_CMD_DISABLE_PACKETS:
        usb_fwd_en = cmd & 1;
        if (usb_fwd_en)
            jd_net_disable_fwd();
        DMESG("usb: packets %d", usb_fwd_en);
        jd_usb_respond_to_processing_packet(cmd);
        break;

    case JD_USB_BRIDGE_CMD_ENABLE_LOG:
    case JD_USB_BRIDGE_CMD_DISABLE_LOG:
        usb_serial_en = cmd & 1;
        jd_usb_respond_to_processing_packet(cmd);
        break;

    default:
        jd_usb_respond_to_processing_packet(0xff);
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

    if (frame->flags & JD_FRAME_FLAG_BROADCAST && frame->flags & JD_FRAME_FLAG_COMMAND &&
        ((frame->device_identifier & 0xffffffff) == JD_SERVICE_CLASS_USB_BRIDGE)) {
        jd_usb_handle_processing_packet((jd_packet_t *)frame);
    } else {
        jd_net_disable_fwd();
        jd_send_frame_raw(frame);
    }
}

static void frame_error(void) {
    // break out of frame state
    usb_rx_state = 0;
    // pass on accumulated data as serial
    uint8_t *rxbuf = (uint8_t *)&usb_rx_buf;
    for (unsigned j = 0; j < usb_rx_ptr; ++j) {
        jd_usb_serial_cb(rxbuf[j]);
    }
    usb_rx_ptr = 0;
}

void jd_usb_push(const uint8_t *buf, unsigned len) {
    uint8_t *rxbuf = (uint8_t *)&usb_rx_buf;
    for (unsigned i = 0; i < len; ++i) {
        uint8_t c = buf[i];
        if (usb_rx_was_magic) {
            if (c == JD_USB_BRIDGE_QBYTE_MAGIC) {
                if (usb_rx_state) {
                    USB_ERROR("dual magic");
                    frame_error();
                    continue;
                }
                // 'c' will be passed to jd_usb_serial_cb() below
            } else {
                usb_rx_was_magic = 0;
                switch (c) {
                case JD_USB_BRIDGE_QBYTE_LITERAL_MAGIC:
                    c = JD_USB_BRIDGE_QBYTE_MAGIC;
                    break;
                case JD_USB_BRIDGE_QBYTE_FRAME_START:
                    if (usb_rx_ptr) {
                        USB_ERROR("second begin");
                        frame_error();
                    }
                    usb_rx_state = c;
                    continue;
                case JD_USB_BRIDGE_QBYTE_FRAME_END:
                    if (usb_rx_state) {
                        usb_rx_state = 0;
                        jd_usb_handle_frame();
                        usb_rx_ptr = 0;
                    } else {
                        USB_ERROR("mismatched stop");
                    }
                    continue;

                case JD_USB_BRIDGE_QBYTE_RESERVED:
                case JD_USB_BRIDGE_QBYTE_SERIAL_GAP:
                case JD_USB_BRIDGE_QBYTE_FRAME_GAP:
                    continue; // ignore

                default:
                    if (usb_rx_state) {
                        USB_ERROR("invalid quote");
                        frame_error();
                    }
                    // either way, pass on directly
                    jd_usb_serial_cb(JD_USB_BRIDGE_QBYTE_MAGIC);
                    // c = c;
                    break;
                }
            }
        } else if (c == JD_USB_BRIDGE_QBYTE_MAGIC) {
            usb_rx_was_magic = 1;
            continue;
        }

        if (usb_rx_state) {
            if (usb_rx_ptr >= sizeof(usb_rx_buf)) {
                USB_ERROR("frame ovf");
                frame_error();
            } else {
                rxbuf[usb_rx_ptr++] = c;
            }
        } else {
            jd_usb_serial_cb(c);
        }
    }
}

int jd_usb_send_frame(void *frame) {
    if (!usb_fwd_en)
        return 0;
    JD_WAKE_MAIN();
    // this can be called from an ISR
    int r = jd_queue_push(usb_queue, frame);
    if (r == 0) {
        if (usb_frame_gap == 2)
            usb_frame_gap = 0;
    } else {
        if (usb_frame_gap == 0)
            usb_frame_gap = 1;
    }
    jd_usb_pull_ready();
    return r;
}

void jd_usb_enable_serial(void) {
    jd_usb_init();
    usb_serial_en = 1;
}

void jd_usb_panic_print_char(char c) {
    if (usb_panic_mode > 1)
        return;

    if (usb_panic_mode == 0) {
        usb_fwd_en = 0;
        usb_serial_en = 0;

        // just to be sure
        serial_buf[0] = JD_USB_BRIDGE_QBYTE_MAGIC;
        serial_buf[1] = JD_USB_BRIDGE_QBYTE_FRAME_END;
        serial_buf_ptr = 2;
        usb_panic_mode = 1;
    }

    serial_buf[serial_buf_ptr++] = c;
    if (serial_buf_ptr < sizeof(serial_buf) && c != '\n')
        return;

    unsigned num_pull = 0;
    while (serial_buf_ptr > 0) {
        jd_usb_pull_ready();
        if (num_pull++ > 50000) {
            serial_buf_ptr = 0;
            usb_panic_mode = 2;
            break;
        }
    }
}

void jd_usb_panic_print_str(const char *s) {
    if (!s)
        s = "(null)";
    while (*s)
        jd_usb_panic_print_char(*s++);
}

void jd_usb_panic_start(void) {
    if (usb_panic_mode > 1)
        return;
    jd_usb_panic_print_char('\n');
}

#if JD_DMESG_BUFFER_SIZE > 0
void jd_usb_proto_process(void) {
    if (!usb_is_connected && jd_usb_looks_connected()) {
        usb_is_connected = 1;
        DMESG("usb: connected");
        dmesg_timer = now + (128 << 10);
    }

    if (dmesg_timer != 1 && in_past(dmesg_timer))
        dmesg_timer = 1;

    if (dmesg_timer == 1) {
        uint8_t buf[64];
        static uint32_t dmesg_ptr;
        int n = jd_dmesg_read(buf, sizeof(buf), &dmesg_ptr);
        if (n > 0)
            jd_lstore_append_frag(0, JD_LSTORE_TYPE_DMESG, buf, n);
    }
}
#else
void jd_usb_proto_process(void) {}
#endif

__attribute__((weak)) void jd_usb_flush_stdout(void) {}

__attribute__((weak)) void jd_usb_process(void) {}

__attribute__((weak)) void jd_net_disable_fwd(void) {}

__attribute__((weak)) bool jd_usb_looks_connected(void) {
    return true;
}

#endif