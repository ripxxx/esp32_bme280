/* 
 * File:   main.c
 * Author: ripx (ripx@me.com)
 */

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"

#include "wifi_smart.h"
#include "bme280_sensor.h"
#include "udp_transport.h"

struct sensor_data_command_t {
    uint8_t value;
    uint8_t id[10];
    unsigned long index;
    double humidity;
    double pressure;
    double temperature;
} sensor_data_command;

static const char *debug_tag = "ESP32_BME280";

static void udp_task(void *ignore) {
    while(true) {
        
        udp_transport_send_broadcast(&sensor_data_command, sizeof(sensor_data_command));
        
        vTaskDelay(10000/portTICK_PERIOD_MS);
    }
    
    vTaskDelete(NULL);
}

static esp_err_t bme280_data_updated(struct bme280_sensor_data_t data, int32_t status) {
    //ESP_LOGI(debug_tag, "\nTemperature: %.2f\nHumidity: %.3f\nPressure: %.3f", data.temperature, data.humidity, (data.pressure/100));
    
    if(status == 0) {
        sensor_data_command.humidity = data.humidity;
        sensor_data_command.pressure = data.pressure;
        sensor_data_command.temperature = data.temperature;
        sensor_data_command.index++;
    }
    
    return ESP_OK;
}

static esp_err_t wifi_connected(void) {
    bme280_sensor_init(&bme280_data_updated);
    udp_transport_init(ESP_IF_WIFI_STA);
    
    xTaskCreate(&udp_task, "udp_task", 2048, NULL, 6, NULL);
    
    return ESP_OK;
}

void app_main() {
    wifi_smart_init(wifi_connected);
    
    sensor_data_command.value = 0x81;
    strncpy((char *)sensor_data_command.id, CONFIG_BME280_DEVICE_ID, sizeof(sensor_data_command.id));
    sensor_data_command.index = 0;
}
