#include "jd_drivers.h"
#include "services/jd_services.h"

const accelerometer_api_t *i2c_accelerometers[] = {
    &accelerometer_lsm6ds, // STMicro LSM6DSxx
    &accelerometer_kxtj3,  // Kionix KXTJ3-1057
    &accelerometer_kx023,  // Kionix KX023-1025
    NULL,
};

const gyroscope_api_t *i2c_gyro[] = {
    &gyroscope_lsm6ds, // STMicro LSM6DSxx
    NULL,
};

const captouch_api_t *i2c_captouch[] = {
    &captouch_cap1298, // Microchip CAP1298
    NULL,
};

// there are temp sensors on pressure, CO2, and humidity sensors
const env_sensor_api_t *i2c_temperature[] = {
    &temperature_dps310,    // Infineon DPS310 (pressure)
    &temperature_lps33hwtr, // STMicro LPS33HW (pressure)
    &temperature_mpl3115a2, // NXP MPL3115A2 (pressure)
    &temperature_scd40,     // Sensirion SCD40 (CO2)
    &temperature_sht30,     // Sensirion SHT30 (humidity)
    &temperature_shtc3,     // Sensirion SHTC3 (humidity)
    &temperature_th02,      // Seeed (?) TH02 (humidity)
    NULL,
};

const env_sensor_api_t *i2c_pressure[] = {
    &pressure_dps310,    // Infineon DPS310
    &pressure_lps33hwtr, // STMicro LPS33HW
    &pressure_mpl3115a2, // NXP MPL3115A2
    NULL,
};

const env_sensor_api_t *i2c_humidity[] = {
    &humidity_scd40,
    &humidity_sht30, // Sensirion SHT30
    &humidity_shtc3, // Sensirion SHTC3
    &humidity_th02,  // Seeed (?) TH02
    NULL,
};

const env_sensor_api_t *i2c_co2[] = {
    &co2_scd40,  // Sensirion SCD40
    &eco2_sgp30, // Sensirion SGP30 (eCO2)
    NULL,
};

const env_sensor_api_t *i2c_tvoc[] = {
    &tvoc_sgp30, // Sensirion SGP30 (eCO2)
    NULL,
};

const env_sensor_api_t *i2c_illuminance[] = {
    &illuminance_ltr390uv, // LITE-ON LTR-390UV-01
    NULL,
};

const env_sensor_api_t *i2c_uvindex[] = {
    &uvindex_ltr390uv, // LITE-ON LTR-390UV-01
    NULL,
};

int jd_scan_i2c(const char *label, const sensor_api_t **apis, void (*init)(const sensor_api_t *)) {
    int num = 0;
    if (i2c_init() == 0) {
        unsigned i;
        for (i = 0; apis[i] != NULL; ++i) {
            if (apis[i]->is_present()) {
                init(apis[i]);
                num++;
            }
        }
        DMESG("I2C scanned %d %s, found %d", i, label, num);
    }
    return num;
}

int jd_scan_accelerometers(void) {
    return jd_scan_i2c("accel", i2c_accelerometers, accelerometer_init);
}

int jd_scan_gyroscopes(void) {
    return jd_scan_i2c("gyro", i2c_gyro, gyroscope_init);
}

int jd_scan_temperature(void) {
    return jd_scan_i2c("temp", i2c_temperature, temperature_init);
}

int jd_scan_pressure(void) {
    return jd_scan_i2c("pressure", i2c_pressure, barometer_init);
}

int jd_scan_humidity(void) {
    return jd_scan_i2c("humidity", i2c_humidity, humidity_init);
}

int jd_scan_co2(void) {
    return jd_scan_i2c("co2", i2c_co2, eco2_init);
}

int jd_scan_tvoc(void) {
    return jd_scan_i2c("tvoc", i2c_tvoc, tvoc_init);
}

int jd_scan_uvindex(void) {
    return jd_scan_i2c("uvi", i2c_uvindex, uvindex_init);
}

int jd_scan_illuminance(void) {
    return jd_scan_i2c("illum", i2c_illuminance, illuminance_init);
}

int jd_scan_all(void) {
    int r = 0;
    r += jd_scan_accelerometers();
    r += jd_scan_gyroscopes();
    r += jd_scan_temperature();
    r += jd_scan_humidity();
    r += jd_scan_pressure();
    r += jd_scan_co2();
    r += jd_scan_tvoc();
    r += jd_scan_uvindex();
    r += jd_scan_illuminance();
    return r;
}
