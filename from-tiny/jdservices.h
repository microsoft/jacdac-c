#ifndef __JDSERVICES_H
#define __JDSERVICES_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define JD_SERVICE_CLASS_CAPTOUCH 0x6cf77465

typedef struct {
    uint8_t flags;
    uint8_t numchannels; // up to 32
} jd_captouch_advertisement_data_t;

#define JD_CAPTOUCH_COMMAND_CALIBRATE 0x01
#define JD_CAPTOUCH_COMMAND_READ 0x02

typedef struct {
    uint32_t command;
    uint32_t channels;
} jd_captouch_command_t;

typedef struct {
    uint32_t channels;
    uint16_t readings[0];
} jd_captouch_reading_t;


#ifdef __cplusplus
}
#endif

#endif
