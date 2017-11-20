/* 
 * File:   bme280_sensor.c
 * Author: ripx (ripx@me.com)
 */

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"

#include "bme280_sensor.h"

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "bme280.h"

static const char *debug_tag = "BME280_SENSOR";

static struct bme280_sensor_data_t bme280_sensor_data;

static bme280_data_updated_fp bme280_data_updated;

static struct bme280_t bme280;

static void i2c_master_init() {
    i2c_config_t i2c_config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = CONFIG_BME280_SDA_PIN_NUMBER,
        .scl_io_num = CONFIG_BME280_SCL_PIN_NUMBER,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 1000000
    };
    
    i2c_param_config(I2C_NUM_0, &i2c_config);
    i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
}

static int8_t bme280_i2c_write(uint8_t device_address, uint8_t register_address, uint8_t *data, uint8_t date_length) {
	esp_err_t error;
        
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();

	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (device_address << 1) | I2C_MASTER_WRITE, true);

	i2c_master_write_byte(cmd, register_address, true);
	i2c_master_write(cmd, data, date_length, true);
	i2c_master_stop(cmd);

	error = i2c_master_cmd_begin(I2C_NUM_0, cmd, (10/portTICK_PERIOD_MS));
	i2c_cmd_link_delete(cmd);
        
        if (error == ESP_OK) {
		return (int8_t)SUCCESS;
	} else {
		return (int8_t)FAIL;
	}
}

static int8_t bme280_i2c_read(uint8_t device_address, uint8_t register_address, uint8_t *data, uint8_t date_length) {
    esp_err_t error;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, register_address, true);

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_address << 1) | I2C_MASTER_READ, true);

    if (date_length > 1) {
        i2c_master_read(cmd, data, (date_length-1), 0);
    }
    i2c_master_read_byte(cmd, (data+(date_length-1)), 1);
    i2c_master_stop(cmd);

    error = i2c_master_cmd_begin(I2C_NUM_0, cmd, (10/portTICK_PERIOD_MS));
    i2c_cmd_link_delete(cmd);

    if (error == ESP_OK) {
            return (int8_t)SUCCESS;
    } else {
            return (int8_t)FAIL;
    }
}

static void bme280_delay(uint32_t delay) {//delay milliseconds
    vTaskDelay(delay/portTICK_PERIOD_MS);
}

static void bme280_data_update_task() {
    int32_t result = 0;
    
    int32_t raw_humidity;
    int32_t raw_pressure;
    int32_t raw_temperature;

    while(true) {

        result = bme280_read_uncomp_pressure_temperature_humidity(&raw_pressure, &raw_temperature, &raw_humidity);

        if (result == SUCCESS) {
            bme280_sensor_data.humidity = bme280_compensate_humidity_double(raw_humidity);
            bme280_sensor_data.pressure = bme280_compensate_pressure_double(raw_pressure);
            bme280_sensor_data.temperature = bme280_compensate_temperature_double(raw_temperature);
        }

        bme280_data_updated(bme280_sensor_data, result);

        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

esp_err_t bme280_sensor_init(bme280_data_updated_fp fp) {
    bme280_data_updated = fp;
    
    //Initializing I2C master
    i2c_master_init();
    vTaskDelay(40/portTICK_PERIOD_MS);
    
    bme280.bus_write = bme280_i2c_write;
    bme280.bus_read = bme280_i2c_read;
    bme280.dev_addr = BME280_I2C_ADDRESS1;
    bme280.delay_msec = bme280_delay;
    
    int32_t result = 0;

    //Initializing BME280
    result = bme280_init(&bme280);
    if(result != SUCCESS) {
        ESP_LOGW(debug_tag, "Failed to initialize.");
        return ESP_FAIL;
    }
    vTaskDelay(40/portTICK_PERIOD_MS);
    
    //Setting oversampling registers
    result = ((bme280_set_oversamp_humidity(BME280_OVERSAMP_1X) != 0)? 1: 0);
    vTaskDelay(40/portTICK_PERIOD_MS);
    result += ((bme280_set_oversamp_pressure(BME280_OVERSAMP_16X) != 0)? 1: 0);
    vTaskDelay(40/portTICK_PERIOD_MS);
    result += ((bme280_set_oversamp_temperature(BME280_OVERSAMP_2X) != 0)? 1: 0);
    vTaskDelay(40/portTICK_PERIOD_MS);
    if(result != SUCCESS) {
        ESP_LOGW(debug_tag, "Failed to set oversampling registers.");
        return ESP_FAIL;
    }
    vTaskDelay(40/portTICK_PERIOD_MS);
    
    //Setting standby duration
    result = bme280_set_standby_durn(BME280_STANDBY_TIME_1_MS);
    if(result != SUCCESS) {
        ESP_LOGW(debug_tag, "Failed to set standby duration.");
        return ESP_FAIL;
    }
    vTaskDelay(40/portTICK_PERIOD_MS);
    
    //Setting filter coefficient
    result = bme280_set_filter(BME280_FILTER_COEFF_16);
    if(result != SUCCESS) {
        ESP_LOGW(debug_tag, "Failed to set filter.");
        return ESP_FAIL;
    }
    vTaskDelay(40/portTICK_PERIOD_MS);
    
    //Setting power mode - NORMAL
    result = bme280_set_power_mode(BME280_NORMAL_MODE);
    if(result != SUCCESS) {
        ESP_LOGW(debug_tag, "Failed to set power mode.");
        return ESP_FAIL;
    }
    vTaskDelay(40/portTICK_PERIOD_MS);

    xTaskCreate(&bme280_data_update_task, "bme280_data_update_task", 2048, NULL, 6, NULL);
    ESP_LOGI(debug_tag, "Initialized successfully.");
    return ESP_OK;
}