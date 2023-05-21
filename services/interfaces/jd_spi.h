// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef __JD_SPI_H
#define __JD_SPI_H

#if JD_SPI

// standard
#define JD_SPI_MODE_CPHA 0x01
#define JD_SPI_MODE_CPOL 0x02

typedef struct {
    // any of pins can be NO_PIN
    uint8_t miso;
    uint8_t mosi;
    uint8_t sck;
    uint8_t mode;
    uint32_t hz;
} jd_spi_cfg_t;

/* Can be called multiple times to change speed or pins. */
int jd_spi_init(const jd_spi_cfg_t *cfg);

/* Start SPI transaction. One (but not both) of `txdata`, `rxdata` can be NULL.
   When the the transaction completes, done_fn() is called, possibly in interrupt-like context.
   The buffers must remain valid until that point.
   The done_fn() may be called before jd_spi_xfer() returns.
   Inside done_fn(), jd_spi_is_ready() returns true, and jd_spi_xfer() can be called again.
   */
int jd_spi_xfer(const void *txdata, void *rxdata, unsigned numbytes, cb_t done_fn);

/* Check if the last SPI transaction finished. */
bool jd_spi_is_ready(void);

/* Return the biggest block size supported by jd_spi_xfer() */
unsigned jd_spi_max_block_size(void);

#endif

#endif