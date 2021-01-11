// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "jd_config.h"
#include "jd_service_classes.h"
#include "jd_physical.h"
#include "jd_services.h"

#define SENSOR_COMMON                                                                              \
    SRV_COMMON;                                                                                    \
    uint8_t streaming_samples;                                                                     \
    uint8_t got_query : 1;                                                                         \
    uint32_t streaming_interval;                                                                   \
    uint32_t next_streaming
#define REG_SENSOR_BASE REG_BYTES(JD_REG_PADDING, 16)

int sensor_handle_packet(srv_t *state, jd_packet_t *pkt);
int sensor_should_stream(srv_t *state);
int sensor_handle_packet_simple(srv_t *state, jd_packet_t *pkt, const void *sample,
                                uint32_t sample_size);
void sensor_process_simple(srv_t *state, const void *sample, uint32_t sample_size);
