#include "jd_drivers.h"
#include "services/jd_services.h"

#define MAX_DURATION 200

#define AW86224_FCR_ADDR 0x58

#define AW86224_FCR_RESET_REG 0x00
#define AW86224_FCR_RESET_VAL 0xaa

#define AW86224_FCR_CONTCFG1_REG 0x18
#define AW86224_FCR_CONTCFG2_REG 0x19
#define AW86224_FCR_CONTCFG3_REG 0x1A
// #define AW86224_FCR_CONTCFG4_REG 0x1B
#define AW86224_FCR_CONTCFG5_REG 0x1C
#define AW86224_FCR_CONTCFG6_REG 0x1D
#define AW86224_FCR_CONTCFG7_REG 0x1E
#define AW86224_FCR_DRV1_TIME_REG 0x1F // 4
#define AW86224_FCR_DRV2_TIME_REG 0x20 // 6
#define AW86224_FCR_BRK_TIME_REG 0x21  // 8
#define AW86224_FCR_CONTCFG11_REG 0x22

#define AW86224_FCR_PLAYCFG2_REG 0x07
#define AW86224_FCR_PLAYCFG3_REG 0x08

#define AW86224_FCR_PLAYCFG4_REG 0x09
#define AW86224_FCR_PLAYCFG4_VAL_GO 0x01
#define AW86224_FCR_PLAYCFG4_VAL_STOP 0x02

#define AW86224_FCR_RTPDATA_REG 0x32

#define AW86224_FCR_GLBRD5_REG 0x3f

#define AW86224_FCR_SYSCTRL1_REG 0x43
#define AW86224_FCR_SYSCTRL1_VAL_ENRAM 0x08

#define AW86224_FCR_SYSCTRL2_REG 0x44
#define AW86224_FCR_SYSCTRL2_VAL_12KHz 0x03

static void write_reg(uint8_t reg, uint8_t v) {
    if (i2c_write_reg(AW86224_FCR_ADDR, reg, v) != 0)
        JD_PANIC();
}

static void aw86224fcr_go(void) {
    // let's go!
    write_reg(AW86224_FCR_PLAYCFG4_REG, AW86224_FCR_PLAYCFG4_VAL_GO);
    target_wait_us(100);

#if 0
    int retry = 0;

    // check the state machine of the part has moved into RTP mode.
    while (1) {
        uint8_t glb_state = i2c_read_reg(AW86224_FCR_ADDR, AW86224_FCR_GLBRD5_REG);

        if (glb_state == 0b1000)
            break;

        target_wait_us(50);

        retry++;

        if (retry > 500)
            target_reset();
    }
#endif
}

static int aw86224fcr_write_amplitude(uint8_t amplitude, uint32_t duration_ms) {
    if (amplitude == 0)
        return 0;

    uint8_t glb_state = i2c_read_reg(AW86224_FCR_ADDR, AW86224_FCR_GLBRD5_REG) & 0xf;
    if (glb_state != 0)
        return -1; // already doing something

    if (duration_ms > MAX_DURATION)
        return -2;

    // 2.5ms per DRVx_TIME
    int v = (duration_ms * 51) >> 8;
    write_reg(AW86224_FCR_DRV1_TIME_REG, v);
    write_reg(AW86224_FCR_DRV2_TIME_REG, v);
    aw86224fcr_go();

    return 0;
}

static void aw86224fcr_reset(void) {
    target_wait_us(5000);
    i2c_write_reg(AW86224_FCR_ADDR, AW86224_FCR_RESET_REG, AW86224_FCR_RESET_VAL);
    target_wait_us(5000);
}

static void aw86224fcr_init(void) {
    i2c_init();

    aw86224fcr_reset();

    // set CONT mode
    write_reg(AW86224_FCR_PLAYCFG3_REG, 0b110);

    write_reg(AW86224_FCR_CONTCFG1_REG, 0b11100001); // 700Hz
    write_reg(AW86224_FCR_BRK_TIME_REG, 5);
}

const vibration_motor_api_t aw86224fcr = { //
    .init = aw86224fcr_init,
    .write_amplitude = aw86224fcr_write_amplitude,
    .max_duration = MAX_DURATION};