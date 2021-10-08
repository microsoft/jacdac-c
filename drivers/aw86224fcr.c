#include "jd_drivers.h"
#include "jd_services.h"

#define AW86224_FCR_ADDR             0x58

#define AW86224_FCR_RESET_REG           0x00
#define AW86224_FCR_RESET_VAL           0xaa

#define AW86224_FCR_PLAYCFG2_REG            0x07

#define AW86224_FCR_PLAYCFG3_REG            0x08
#define AW86224_FCR_PLAYCFG3_VAL_RTP        0x01

#define AW86224_FCR_PLAYCFG4_REG            0x09
#define AW86224_FCR_PLAYCFG4_VAL_GO        0x01
#define AW86224_FCR_PLAYCFG4_VAL_STOP        0x02

#define AW86224_FCR_RTPDATA_REG             0x32

#define AW86224_FCR_GLBRD5_REG             0x3f

#define AW86224_FCR_SYSCTRL1_REG            0x43
#define AW86224_FCR_SYSCTRL1_VAL_ENRAM      0x08

#define AW86224_FCR_SYSCTRL2_REG            0x44
#define AW86224_FCR_SYSCTRL2_VAL_12KHz      0x03

static void aw86224fcr_write_amplitude(uint8_t amplitude) {
    i2c_write_reg(AW86224_FCR_ADDR, AW86224_FCR_RTPDATA_REG, amplitude);
}

static void aw86224fcr_reset(void) {
    target_wait_us(5000);
    i2c_write_reg(AW86224_FCR_ADDR, AW86224_FCR_RESET_REG, AW86224_FCR_RESET_VAL);
    target_wait_us(5000);
}

static void aw86224fcr_init(void) {
    i2c_init();

    aw86224fcr_reset();

    // gain up!
    i2c_write_reg(AW86224_FCR_ADDR, AW86224_FCR_PLAYCFG2_REG, 0xff);

    // slow down the vibrations...
    i2c_write_reg(AW86224_FCR_ADDR, AW86224_FCR_SYSCTRL2_REG, AW86224_FCR_SYSCTRL2_VAL_12KHz);
    target_wait_us(100);

    // set real time play back (RTP)
    i2c_write_reg(AW86224_FCR_ADDR, AW86224_FCR_PLAYCFG3_REG, AW86224_FCR_PLAYCFG3_VAL_RTP);
    target_wait_us(100);

    // let's go!
    i2c_write_reg(AW86224_FCR_ADDR, AW86224_FCR_PLAYCFG4_REG, AW86224_FCR_PLAYCFG4_VAL_GO);
    target_wait_us(100);

    int retry = 0;

    // check the state machine of the part has moved into RTP mode.
    while (1) { 
        uint8_t glb_state = i2c_read_reg(AW86224_FCR_ADDR, AW86224_FCR_GLBRD5_REG);

        if (glb_state == 0b1000)
            break;

        retry++;

        if (retry > 500)
            hw_panic();
    }
}

const vibration_motor_api_t aw86224fcr = {
    .init = aw86224fcr_init,
    .write_amplitude = aw86224fcr_write_amplitude
};