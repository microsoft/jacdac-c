#ifndef JD_CONFIG_H
#define JD_CONFIG_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

// 255 minus size of the serial header, rounded down to 4
#define JD_SERIAL_PAYLOAD_SIZE 236
#define JD_SERIAL_FULL_HEADER_SIZE 16
// the COMMAND flag signifies that the device_identifier is the recipent
// (i.e., it's a command for the peripheral); the bit clear means device_identifier is the source
// (i.e., it's a report from peripheral or a broadcast message)
#define JD_FRAME_FLAG_COMMAND 0x01
// an ACK should be issued with CRC of this package upon reception
#define JD_FRAME_FLAG_ACK_REQUESTED 0x02
// the device_identifier contains target service class number
#define JD_FRAME_FLAG_IDENTIFIER_IS_SERVICE_CLASS 0x04
#define JD_FRAME_SIZE(pkt) ((pkt)->size + 12)

#endif