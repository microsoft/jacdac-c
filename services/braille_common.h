#ifndef BRAILLE_COMMON_H

#include "jd_services.h"

bool BrGetPtn(uint16_t row, uint16_t col);
void BrSetPtn(uint16_t row, uint16_t col, bool state);
void BrClrPtn(void);
void BrClrAllDots(const hbridge_api_t *api);
void BrSetSingleDot(const hbridge_api_t *state, uint16_t row, uint16_t col, bool set);
void BrRfshPtn(const hbridge_api_t *api);
void BrSetAllDots(const hbridge_api_t *api);

void braille_send(const hbridge_api_t *api, const uint8_t *data);

#endif