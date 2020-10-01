// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef __JD_CONSOLE_H
#define __JD_CONSOLE_H

#include <stdarg.h>

void jdcon_logv(int level, const char *format, va_list ap);
void jdcon_log(const char *format, ...);
void jdcon_warn(const char *format, ...);

#endif