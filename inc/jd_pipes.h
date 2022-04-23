#pragma once

//
// Output pipes (data flowing from this device)
//

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

int jd_opipe_open(jd_opipe_desc_t *str, uint64_t device_id, uint16_t port_num);
int jd_opipe_open_cmd(jd_opipe_desc_t *str, jd_packet_t *cmd_pkt);
int jd_opipe_open_report(jd_opipe_desc_t *str, jd_packet_t *report_pkt);

// all these functions can return JD_PIPE_TRY_AGAIN
// can be optionally called before jd_opipe_write*(); if this return JD_PIPE_OK, then write also
// will
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

//
// Input pipes (data flowing to this device)
//

typedef struct jd_ipipe_desc jd_ipipe_desc_t;
typedef void (*jd_ipipe_handler_t)(jd_ipipe_desc_t *istr, jd_packet_t *pkt);
struct jd_ipipe_desc {
    jd_ipipe_handler_t handler;
    jd_ipipe_handler_t meta_handler;
    struct jd_ipipe_desc *next;
    uint16_t counter;
};
int jd_ipipe_open(jd_ipipe_desc_t *str, jd_ipipe_handler_t handler,
                  jd_ipipe_handler_t meta_handler);
void jd_ipipe_close(jd_ipipe_desc_t *str);
void jd_ipipe_handle_packet(jd_packet_t *pkt);
