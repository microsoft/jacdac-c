#pragma once

#define JD_PIPE_OK 0
#define JD_PIPE_TRY_AGAIN 1
#define JD_PIPE_TIMEOUT -1
#define JD_PIPE_ERROR -2

#define JD_OPIPE_MAX_RETRIES 4

// note that this is around 260 bytes
typedef struct jd_opipe_desc {
    // don't access members directly
    struct jd_opipe_desc *next;
    uint16_t counter;
    uint8_t status;
    uint8_t curr_retry;
    uint32_t retry_time;
    jd_frame_t frame;
} jd_opipe_desc_t;

int jd_opipe_open(jd_opipe_desc_t *str, jd_packet_t *pkt);

// all these functions can return JD_PIPE_TRY_AGAIN
// can be optionally called before jd_opipe_write*(); if this return JD_PIPE_OK, then write also will
int jd_opipe_check_space(jd_opipe_desc_t *str, unsigned len);
int jd_opipe_write(jd_opipe_desc_t *str, const void *data, unsigned len);
int jd_opipe_write_meta(jd_opipe_desc_t *str, const void *data, unsigned len);
// flush is automatic when buffer full, or on close
int jd_opipe_flush(jd_opipe_desc_t *str);
// it's OK to closed a closed stream
int jd_opipe_close(jd_opipe_desc_t *str);

// the be called from top-level
void jd_opipe_handle_packet(jd_packet_t *pkt);
void jd_opipe_process(void);

#if 0
typedef struct ipipe_desc ipipe_desc_t;
typedef void (*ipipe_handler_t)(ipipe_desc_t *istr, jd_packet_t *pkt);
struct ipipe_desc {
    ipipe_handler_t handler;
    ipipe_handler_t meta_handler;
    struct ipipe_desc *next;
    SemaphoreHandle_t sem;
    uint16_t counter;
};
int ipipe_open(ipipe_desc_t *str, ipipe_handler_t handler, ipipe_handler_t meta_handler);
void ipipe_close(ipipe_desc_t *str);
void ipipe_handle_pkt(jd_packet_t *pkt);
#endif