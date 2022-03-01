// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef JD_LORA_H
#define JD_LORA_H

#include "jd_config.h"
#include "jd_physical.h"

#if JD_LORA
void jd_lora_process(void);
void jd_lora_init(void);
int jd_lora_send(const void *data, uint32_t datalen);
bool jd_lora_in_timer(void);
#endif

#endif
