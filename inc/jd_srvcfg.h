#pragma once

#include "jd_dcfg.h"

#if JD_DCFG
char *jd_srvcfg_key(const char *key);
uint8_t jd_srvcfg_pin(const char *key);
int32_t jd_srvcfg_i32(const char *key, int32_t defl);
int32_t jd_srvcfg_u32(const char *key, int32_t defl);
bool jd_srvcfg_has_flag(const char *key);
srv_t *jd_srvcfg_last_service(void);

void jd_srvcfg_run(void);
const char *jd_srvcfg_instance_name(srv_t *srv);
int jd_srvcfg_variant(srv_t *srv);
#endif