/* 
 * File:   wifi_config.c
 * Author: ripx (ALEXANDER BERDNIKOV)
 *
 * Created on July 10, 2017, 10:31 PM
 */

#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "wifi_config.h"

static const char *debug_tag = "WIFI_SETTINGS";
const char *nvs_namespace = "wifi_settings";

static nvs_handle handle;
static uint8_t is_nvs_inited = 0;

esp_err_t wifi_config_close(void) {
    if(is_nvs_inited > 0) {
        nvs_close(handle);
        is_nvs_inited = 1;
        return ESP_OK;
    }
    return ESP_FAIL;
}

esp_err_t wifi_config_erase_credentials(void) {
    esp_err_t error = ESP_FAIL;
    if(is_nvs_inited > 0) {
        error = nvs_erase_key(handle, "ssid");
        if(error != ESP_OK) {
            ESP_LOGW(debug_tag, "Wifi ssid value was not erased, NVS: %d", error);
            return ESP_FAIL;
        }

        error = nvs_erase_key(handle, "password");
        if(error != ESP_OK) {
            ESP_LOGW(debug_tag, "Wifi password value was not erased, NVS: %d", error);
            return ESP_FAIL;
        }
        
        error = nvs_commit(handle);
        if(error != ESP_OK) {
            ESP_LOGW(debug_tag, "Wifi credentials erase was not commited, NVS: %d", error);
            return ESP_FAIL;
        }
    }
    
    return error;
}

esp_err_t wifi_config_open(void) {
    esp_err_t error = nvs_flash_init();
    if(error != ESP_OK) {
        ESP_LOGW(debug_tag, "Failed to init NVS: %d", error);
        return ESP_FAIL;
    }
    
    error = nvs_open(nvs_namespace, NVS_READWRITE, &handle);
    if(error != ESP_OK) {
        ESP_LOGW(debug_tag, "Failed to open NVS: %d", error);
        return ESP_FAIL;
    }
    
    is_nvs_inited = 1;
    
    return ESP_OK;
}

esp_err_t wifi_config_read_credentials(char ssid[WIFI_SSID_MAX_LENGTH], char password[WIFI_PASSWORD_MAX_LENGTH]) {
    esp_err_t error = ESP_ERR_NOT_FOUND;
    
    if(is_nvs_inited > 0) {
        size_t length = WIFI_SSID_MAX_LENGTH;

        error = nvs_get_str(handle, "ssid", ssid, &length);
        if((error != ESP_OK) && (error != ESP_ERR_NVS_NOT_FOUND)) {
            ESP_LOGW(debug_tag, "Wifi ssid value was not found, NVS: %d", error);
            return ESP_ERR_NOT_FOUND;
        }
        length = WIFI_PASSWORD_MAX_LENGTH;
        error = nvs_get_str(handle, "password", password, &length);
        if((error != ESP_OK) && (error != ESP_ERR_NVS_NOT_FOUND)) {
            ESP_LOGW(debug_tag, "Wifi password value was not found, NVS: %d", error);
            password[0] = 0;
            //return ESP_ERR_NOT_FOUND;
            return ESP_OK;
        }
    }
    
    return error;
}

esp_err_t wifi_config_write_credentials(const char *ssid, const char *password) {
    esp_err_t error = ESP_FAIL;
    if(is_nvs_inited > 0) {
        error = nvs_set_str(handle, "ssid", ssid);
        if(error != ESP_OK) {
            ESP_LOGW(debug_tag, "Wifi ssid value was not saved, NVS: %d", error);
            return ESP_FAIL;
        }

        error = nvs_set_str(handle, "password", password);
        if(error != ESP_OK) {
            ESP_LOGW(debug_tag, "Wifi password value was not saved, NVS: %d", error);
            return ESP_FAIL;
        }
        
        error = nvs_commit(handle);
        if(error != ESP_OK) {
            ESP_LOGW(debug_tag, "Wifi credentials was not commited, NVS: %d", error);
            return ESP_FAIL;
        }
    }
    
    return error;
}