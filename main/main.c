/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"

#include "lwip/err.h"
#include "lwip/udp.h"

#include "wifi_smart.h"
#include "bme280.h"

static const char *debug_tag = "UDP";

static void i2c_master_init() {
    i2c_config_t i2c_config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = 21,
        .scl_io_num = 22,
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

static void udp280_task(void *ignore) {
    struct udp_pcb *local_pcb = udp_new();
    struct udp_pcb *broadcast_pcb = udp_new();
    int port = 16901;
    char data[60] = "";
    ip_addr_t bip;
    
    struct bme280_t bme280 = {
        .bus_write = bme280_i2c_write,
        .bus_read = bme280_i2c_read,
        .dev_addr = BME280_I2C_ADDRESS1,
        .delay_msec = bme280_delay
    };

    int32_t result;
    int32_t raw_humidity;
    int32_t raw_pressure;
    int32_t raw_temperature;

    result = bme280_init(&bme280);
    ESP_LOGI(debug_tag, "BME280 Init: %d", result);
    result += bme280_set_oversamp_humidity(BME280_OVERSAMP_1X);
    ESP_LOGI(debug_tag, "BME280 Set Over Sampling for Humidity: %d", result);
    result += bme280_set_oversamp_pressure(BME280_OVERSAMP_16X);
    ESP_LOGI(debug_tag, "BME280 Set Over Sampling for Pressure: %d", result);
    result += bme280_set_oversamp_temperature(BME280_OVERSAMP_2X);
    ESP_LOGI(debug_tag, "BME280 Set Over Sampling for Temperature: %d", result);

    result += bme280_set_standby_durn(BME280_STANDBY_TIME_1_MS);
    ESP_LOGI(debug_tag, "BME280 Standby: %d", result);
    result += bme280_set_filter(BME280_FILTER_COEFF_16);
    ESP_LOGI(debug_tag, "BME280 Filter: %d", result);

    result += bme280_set_power_mode(BME280_NORMAL_MODE);
    ESP_LOGI(debug_tag, "BME280 Set Power Mode: %d", result);
    
    if (result == SUCCESS) {
        udp_bind(local_pcb, IP_ADDR_ANY, port);
        IP_ADDR4(&bip, 192, 168, 1, 255);

        struct pbuf * p = pbuf_alloc(PBUF_TRANSPORT, 60, PBUF_REF);
        double h = 0, pressure = 0, t = 0;
        
        while(true) {
            vTaskDelay(100/portTICK_PERIOD_MS);
            result = bme280_read_uncomp_pressure_temperature_humidity(&raw_pressure, &raw_temperature, &raw_humidity);

            if (result == SUCCESS) {
                h = bme280_compensate_humidity_double(raw_humidity);
                pressure = bme280_compensate_pressure_double(raw_pressure);
                t = bme280_compensate_temperature_double(raw_temperature);
                
                sprintf(data, "{\"t\": %.2f, \"h\": %.3f, \"p\": %.3f}", t, h, pressure);
                p->payload = data;
                udp_sendto(broadcast_pcb, p, &bip, port);
                ESP_LOGI(debug_tag, "Sending data...");
                ESP_LOGI(debug_tag, "Temperature: %.2f\nHumidity: %.3f\nPressure: %.3f", t, h, (pressure/100));
            }
            else {
                ESP_LOGW(debug_tag, "Measure error: %d", result);
            }
            
            vTaskDelay(10000/portTICK_PERIOD_MS);
        }
        
        pbuf_free(p);
    }
    else {
        ESP_LOGE(debug_tag, "Error: %d", result);
    }
    
    vTaskDelete(NULL);
}

static esp_err_t wifi_connected(void) {
    xTaskCreate(&udp280_task, "udp280_task", 2048, NULL, 6, NULL);
    return ESP_OK;
}

void app_main() {
    i2c_master_init();
    vTaskDelay(1000/portTICK_PERIOD_MS);
    wifi_smart_init(wifi_connected);
    
}
