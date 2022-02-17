// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include "jd_config.h"
#include "jd_physical.h"

#if JD_LORA
void jd_lora_process(void);
void jd_lora_init(void);
#else
#define jd_lora_process() ((void)0)
#define jd_lora_init() ((void)0)
#endif
