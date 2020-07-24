#pragma once

#include "jd_config.h"
#include "jd_physical.h"
#include "jd_services.h"

#define JD_SERVICE_CLASS_CTRL 0x00000000

#define JD_SERVICE_NUMBER_CTRL 0x00
#define JD_SERVICE_NUMBER_MASK 0x3f
#define JD_SERVICE_NUMBER_CRC_ACK 0x3f
#define JD_SERVICE_NUMBER_STREAM 0x3e

#define JD_STREAM_COUNTER_MASK 0x001f
#define JD_STREAM_CLOSE_MASK 0x0020
#define JD_STREAM_METADATA_MASK 0x0040
#define JD_STREAM_PORT_SHIFT 7

#define JD_ADVERTISEMENT_0_COUNTER_MASK 0x0000000F
#define JD_ADVERTISEMENT_0_ACK_SUPPORTED 0x00000100

// Registers 0x001-0x07f - r/w common to all services
// Registers 0x080-0x0ff - r/w defined per-service
// Registers 0x100-0x17f - r/o common to all services
// Registers 0x180-0x1ff - r/o defined per-service
// Registers 0x200-0xeff - custom, defined per-service
// Registers 0xf00-0xfff - reserved for implementation, should not be on the wire

// this is either binary (0 or non-zero), or can be gradual (eg. brightness of neopixel)
#define JD_REG_INTENSITY 0x01
// the primary value of actuator (eg. servo angle)
#define JD_REG_VALUE 0x02
// enable/disable streaming
#define JD_REG_IS_STREAMING 0x03
// streaming interval in miliseconds
#define JD_REG_STREAMING_INTERVAL 0x04
// for analog sensors
#define JD_REG_LOW_THRESHOLD 0x05
#define JD_REG_HIGH_THRESHOLD 0x06
// limit power drawn; in mA
#define JD_REG_MAX_POWER 0x07

// eg. one number for light sensor, all 3 coordinates for accelerometer
#define JD_REG_READING 0x101

#define JD_CMD_GET_REG 0x1000
#define JD_CMD_SET_REG 0x2000

// Commands 0x000-0x07f - common to all services
// Commands 0x080-0xeff - defined per-service
// Commands 0xf00-0xfff - reserved for implementation
// enumeration data for CTRL, ad-data for other services
#define JD_CMD_ADVERTISEMENT_DATA 0x00
// event from sensor or on broadcast service
#define JD_CMD_EVENT 0x01
// request to calibrate sensor
#define JD_CMD_CALIBRATE 0x02
// request human-readable description of service
#define JD_CMD_GET_DESCRIPTION 0x03

// Commands specific to control service
// do nothing
#define JD_CMD_CTRL_NOOP 0x80
// blink led or otherwise draw user's attention
#define JD_CMD_CTRL_IDENTIFY 0x81
// reset device
#define JD_CMD_CTRL_RESET 0x82
// identifies the type of hardware (eg., ACME Corp. Servo X-42 Rev C)
#define JD_REG_CTRL_DEVICE_DESCRIPTION 0x180
// a numeric code for the string above; used to mark firmware images
#define JD_REG_CTRL_DEVICE_CLASS 0x181
// MCU temperature in Celsius
#define JD_REG_CTRL_TEMPERATURE 0x182
// 0x183 was light level reading - removed
// typically the same as JD_REG_CTRL_DEVICE_CLASS; the bootloader will respond to that code
#define JD_REG_CTRL_BL_DEVICE_CLASS 0x184
// a string describing firmware version; typically semver
#define JD_REG_CTRL_FIRMWARE_VERSION 0x185
// number of microseconds since boot, 64 bit
#define JD_REG_CTRL_MICROS_SINCE_BOOT 0x186

void jd_ctrl_init(void);
void jd_ctrl_process(srv_t *_state);
void jd_ctrl_handle_packet(srv_t *_state, jd_packet_t *pkt);
void dump_pkt(jd_packet_t *pkt, const char *msg);
