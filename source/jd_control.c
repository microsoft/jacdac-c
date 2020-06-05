#include "jdsimple.h"

// do not use _state parameter in this file - it can be NULL in bootloader mode

struct srv_state {
    SRV_COMMON;
};

static uint8_t id_counter;
static uint32_t nextblink;

static void identify(void) {
    if (!id_counter)
        return;
    if (!should_sample(&nextblink, 150000))
        return;

    id_counter--;
    led_blink(50000);
}

void ctrl_process(srv_t *_state) {
    identify();
}

static void send_value(jd_packet_t *pkt, uint32_t v) {
    txq_push(JD_SERVICE_NUMBER_CTRL, pkt->service_command, &v, sizeof(v));
}

void ctrl_handle_packet(srv_t *_state, jd_packet_t *pkt) {
    uint32_t v;

    switch (pkt->service_command) {
    case JD_CMD_ADVERTISEMENT_DATA:
        app_queue_annouce();
        break;

    case JD_CMD_CTRL_IDENTIFY:
        id_counter = 7;
        nextblink = now;
        identify();
        break;

    case JD_CMD_CTRL_RESET:
        target_reset();
        break;

    case (JD_CMD_GET_REG | JD_REG_CTRL_DEVICE_DESCRIPTION):
        txq_push(JD_SERVICE_NUMBER_CTRL, pkt->service_command, app_dev_class_name,
                 strlen(app_dev_class_name));
        break;

    case (JD_CMD_GET_REG | JD_REG_CTRL_DEVICE_CLASS):
        send_value(pkt, app_dev_info.device_class);
        break;

    case (JD_CMD_GET_REG | JD_REG_CTRL_BL_DEVICE_CLASS):
        send_value(pkt, bl_dev_info.device_class);
        break;

    case (JD_CMD_GET_REG | JD_REG_CTRL_TEMPERATURE):
        send_value(pkt, adc_read_temp());
        break;

    case (JD_CMD_GET_REG | JD_REG_CTRL_LIGHT_LEVEL):
        if (PIN_LED_GND != -1 && adc_can_read_pin(PIN_LED)) {
            pin_set(PIN_LED_GND, 1);
            pin_set(PIN_LED, 0);
            pin_setup_analog_input(PIN_LED);
            target_wait_us(2000);
            v = adc_read_pin(PIN_LED);
            pin_setup_output(PIN_LED);
            pin_set(PIN_LED_GND, 0);
            send_value(pkt, v);
        }
        break;
    }
}

SRV_DEF(ctrl, JD_SERVICE_CLASS_CTRL);
void ctrl_init() {
    SRV_ALLOC(ctrl);
}
