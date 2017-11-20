/* 
 * File:   bme280_sensor.h
 * Author: ripx (ripx@me.com)
 */

#ifndef BME280_SENSOR_H
#define BME280_SENSOR_H

#include "esp_err.h"

struct bme280_sensor_data_t {
    double humidity;
    double pressure;
    double temperature;
};

typedef esp_err_t (*bme280_data_updated_fp)(struct bme280_sensor_data_t, int32_t);

esp_err_t bme280_sensor_init(bme280_data_updated_fp fp);

#endif /* BME280_SENSOR_H */

