// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef JD_APP_H
#define JD_APP_H

#include "jd_config.h"

/**
 * This function configures firmware for a given module.
 * It's typically the only function that changes between modules.
 */
void app_init_services(void);

/**
 * This is an optionally implementable callback invoked in main.c.
 * It allows apps to perform meta-level configuration detection.
 * This is useful for devices that may need to dynamically detect
 * hardware changes at runtime (e.g. XAC module).
 */
void app_process(void);

#if JD_INSTANCE_NAME
/**
 * Define in application to specify instance names for services.
 */
const char *app_get_instance_name(int service_idx);
#endif

#endif
