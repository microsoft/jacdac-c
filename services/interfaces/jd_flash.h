// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef __JD_FLASH_H
#define __JD_FLASH_H

// JD_FLASH_PAGE_SIZE is defined somewhere

void flash_program(void *dst, const void *src, uint32_t len);
void flash_erase(void *page_addr);
void flash_sync(void);

// key is limited to 15 characters

char *jd_settings_get(const char *key);
int jd_settings_set(const char *key, const char *val);

// returns the size of the item, or -1 when not found
// if space < return value, the dst might or might not have been modified
int jd_settings_get_bin(const char *key, void *dst, unsigned space);
// returns 0 on success
int jd_settings_set_bin(const char *key, const void *val, unsigned size);
uint8_t *jd_settings_get_bin_a(const char *key, unsigned *sizep);

#if JD_SETTINGS_LARGE
// Large settings take at least JD_FLASH_PAGE_SIZE of storage
// NULL when not found
const void *jd_settings_get_large(const char *key, unsigned *sizep);
// NULL on error
void *jd_settings_prep_large(const char *key, unsigned size);
// 0 on success; dst is jd_settings_prep_large(..., N) + off where 0 <= off < N
int jd_settings_write_large(void *dst, const void *src, unsigned size);
// flushes all pending jd_settings_write_large()
int jd_settings_sync_large(void);
int jd_settings_large_delete(const char *key);
#endif

void jd_settings_test(void);

#endif