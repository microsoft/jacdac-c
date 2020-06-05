#include "jdsimple.h"

/*
Timings, for 256 byte packet, 64MHz STM32G031:
  Hardware: 40us
  Software fast: 88us
  Software slow: 388us
*/

#if defined(CRC_POL_POL)
static uint8_t inited;
uint16_t crc16(const void *data, uint32_t size) {
    if (!inited) {
        LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_CRC);
        // reset value
        // LL_CRC_SetInputDataReverseMode(CRC, LL_CRC_INDATA_REVERSE_NONE);
        // LL_CRC_SetOutputDataReverseMode(CRC, LL_CRC_OUTDATA_REVERSE_NONE);
        LL_CRC_SetPolynomialCoef(CRC, 0x1021);
        LL_CRC_SetPolynomialSize(CRC, LL_CRC_POLYLENGTH_16B);
        inited = 1;
    }
    LL_CRC_SetInitialData(CRC, LL_CRC_DEFAULT_CRC_INITVALUE);
    const uint8_t *ptr = (const uint8_t *)data;
    while (size--)
        LL_CRC_FeedData8(CRC, *ptr++);
    return LL_CRC_ReadData16(CRC);
}
#else

#if 0
uint16_t crc16soft_slow(const void *data, uint32_t size) {
    const uint8_t *ptr = (const uint8_t *)data;
    uint16_t crc = 0xffff;
    while (size--) {
        uint8_t data = *ptr++;
        crc = crc ^ ((uint16_t)data << 8);
        for (int i = 0; i < 8; i++) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }
    return crc;
}
#endif

// https://wiki.nicksoft.info/mcu:pic16:crc-16:home
uint16_t crc16(const void *data, uint32_t size) {
    const uint8_t *ptr = (const uint8_t *)data;
    uint16_t crc = 0xffff;
    while (size--) {
        uint8_t data = *ptr++;
        uint8_t x = (crc >> 8) ^ data;
        x ^= x >> 4;
        crc = (crc << 8) ^ (x << 12) ^ (x << 5) ^ x;
    }
    return crc;
}
#endif