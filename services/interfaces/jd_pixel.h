// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef __JD_PIXEL_H
#define __JD_PIXEL_H

void px_init(int light_type);
void px_tx(const void *data, uint32_t numbytes, uint8_t intensity, cb_t doneHandler);
#define PX_WORDS(NUM_PIXELS) (((NUM_PIXELS)*3 + 3) / 4)

#endif