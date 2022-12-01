// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef __JD_FLASH_H
#define __JD_FLASH_H

// JD_FLASH_PAGE_SIZE is defined somewhere

void flash_program(void *dst, const void *src, uint32_t len);
void flash_erase(void *page_addr);
void flash_sync(void);

char *jd_settings_get(const char *key);
int jd_settings_set(const char *key, const char *val);

// returns the size of the item, or -1 when not found
// if space < return value, the dst might or might not have been modified
int jd_settings_get_bin(const char *key, void *dst, unsigned space);
// returns 0 on success
int jd_settings_set_bin(const char *key, const void *val, unsigned size);

#endif