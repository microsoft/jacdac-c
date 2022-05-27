// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef __JD_FLASH_H
#define __JD_FLASH_H

// JD_FLASH_PAGE_SIZE is defined somewhere

void flash_program(void *dst, const void *src, uint32_t len);
void flash_erase(void *page_addr);
void flash_sync(void);

#endif