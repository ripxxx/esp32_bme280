/* 
 * File:   wifi_smart.c
 * Author: ripx (ALEXANDER BERDNIKOV)
 *
 * Created on July 10, 2017, 10:33 PM
 */

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"
#include "esp_smartconfig.h"

#include "wifi_smart.h"
#include "wifi_config.h"

static const char *debug_tag = "WIFI";

static EventGroupHandle_t wifi_event_group;
static wifi_smart_cb_t wifi_smart_cb;

static void wifi_smartconfig_callback(smartconfig_status_t status, void *pdata) {
    esp_err_t error;
    switch(status) {
        case SC_STATUS_WAIT:
            ESP_LOGI(debug_tag, "SC_STATUS_WAIT");
            break;
        case SC_STATUS_FIND_CHANNEL:
            ESP_LOGI(debug_tag, "SC_STATUS_FINDING_CHANNEL");
            break;
        case SC_STATUS_GETTING_SSID_PSWD:
            ESP_LOGI(debug_tag, "SC_STATUS_GETTING_SSID_PSWD");
            break;
        case SC_STATUS_LINK:
            ESP_LOGI(debug_tag, "SC_STATUS_LINK");
            wifi_config_t *wifi_config = pdata;
            ESP_LOGI(debug_tag, "WiFi credentials(ssid, password): %s, %s", wifi_config->sta.ssid, wifi_config->sta.password);
            wifi_config_write_credentials((char *)(wifi_config->sta.ssid), (char *)(wifi_config->sta.password));
            error = esp_wifi_disconnect();
            if(error != ESP_OK) {
                ESP_LOGW(debug_tag, "Failed to disconnect wifi: %d", error);
                return;
            }
            error = esp_wifi_set_config(ESP_IF_WIFI_STA, wifi_config);
            if(error != ESP_OK) {
                ESP_LOGW(debug_tag, "Failed to set wifi config: %d", error);
                return;
            }
            error = esp_wifi_connect();
            if(error != ESP_OK) {
                ESP_LOGW(debug_tag, "Failed to connect wifi: %d", error);
                return;
            }
            break;
        case SC_STATUS_LINK_OVER:
            ESP_LOGI(debug_tag, "SC_STATUS_LINK_OVER");
            if (pdata != NULL) {
                uint8_t phone_ip[4] = { 0 };
                memcpy(phone_ip, (uint8_t* )pdata, 4);
                ESP_LOGI(debug_tag, "Phone ip: %d.%d.%d.%d\n", phone_ip[0], phone_ip[1], phone_ip[2], phone_ip[3]);
            }
            xEventGroupSetBits(wifi_event_group, BIT1);
            break;
        default:
            break;
    }
}

static void wifi_smartconfig_task(void *parm) {
    esp_err_t error;
    EventBits_t event_bits;
    error = esp_smartconfig_set_type(SC_TYPE_ESPTOUCH);
    if(error != ESP_OK) {
        ESP_LOGW(debug_tag, "Failed to set for smart config: %d", error);
        return;
    }
    error = esp_smartconfig_start(wifi_smartconfig_callback);
    if(error != ESP_OK) {
        ESP_LOGW(debug_tag, "Failed to start smart config: %d", error);
        return;
    }
    while(1) {
        event_bits = xEventGroupWaitBits(wifi_event_group, (BIT0 | BIT1), true, false, portMAX_DELAY); 
        if(event_bits & BIT0) {
            ESP_LOGI(debug_tag, "Smart config wifi connected");
        }
        
        if(event_bits & BIT1) {
            ESP_LOGI(debug_tag, "Smart config finished");
            esp_smartconfig_stop();
            vTaskDelete(NULL);
        }
    }
}

static esp_err_t wifi_event_handler(void *ctx, system_event_t *event) {
    esp_err_t error;
    EventBits_t event_bits;
    static uint32_t sta_disconnected_counter = 0;
    switch(event->event_id) {
        case SYSTEM_EVENT_STA_START:
            event_bits = xEventGroupGetBits(wifi_event_group);
            if((event_bits & BIT2) == 0) {
                xTaskCreate(wifi_smartconfig_task, "wifi_smartconfig_task", 4096, NULL, 3, NULL);
            }
            else {
                esp_wifi_connect();
            }
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            xEventGroupSetBits(wifi_event_group, BIT0);
            xEventGroupSetBits(wifi_event_group, BIT2);
            error = wifi_config_close();
            if(error != ESP_OK) {
                ESP_LOGW(debug_tag, "Failed to close wifi config: %d", error);
                return ESP_FAIL;
            }
            if (wifi_smart_cb) {
                return (*wifi_smart_cb)();
            }
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            xEventGroupClearBits(wifi_event_group, BIT0);
            event_bits = xEventGroupGetBits(wifi_event_group);
            if(sta_disconnected_counter++ > 3) {
                sta_disconnected_counter = 0;
                xEventGroupClearBits(wifi_event_group, BIT2);
                xTaskCreate(wifi_smartconfig_task, "wifi_smartconfig_task", 4096, NULL, 3, NULL);
            }
            else {
                esp_wifi_connect();
            }
            break;
        default:
            break;
    }
    return ESP_OK;
}

esp_err_t wifi_smart_init(wifi_smart_cb_t cb) {
    wifi_smart_cb = cb;
    
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    
    esp_err_t error;
    
    error = esp_event_loop_init(wifi_event_handler, NULL);
    if(error != ESP_OK) {
        ESP_LOGW(debug_tag, "Failed to init event loop: %d", error);
        return ESP_FAIL;
    }
    
    error = nvs_flash_init();
    if(error != ESP_OK) {
        ESP_LOGW(debug_tag, "Failed to init NVS: %d", error);
        return ESP_FAIL;
    }
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    error = esp_wifi_init(&cfg);
    if(error != ESP_OK) {
        ESP_LOGW(debug_tag, "Failed to init esp wifi: %d", error);
        return ESP_FAIL;
    }
    
    error = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    if(error != ESP_OK) {
        ESP_LOGW(debug_tag, "Failed to set esp wifi storage: %d", error);
        return ESP_FAIL;
    }
    
    error = esp_wifi_set_mode(WIFI_MODE_STA);
    if(error != ESP_OK) {
        ESP_LOGW(debug_tag, "Failed to set esp wifi mode: %d", error);
        return ESP_FAIL;
    }
    
    char ssid[32], password[64];
    error = wifi_config_open();
    if(error != ESP_OK) {
        ESP_LOGW(debug_tag, "Failed to open wifi config: %d", error);
        return ESP_FAIL;
    }
    error = wifi_config_read_credentials(ssid, password);
    if(error == ESP_OK) {
        
        wifi_config_t wifi_config = {
            .sta = {
                .ssid = "",
                .password = "",
            },
        };
        
        //wifi_config_erase_credentials();
        
        memcpy(wifi_config.sta.ssid, ssid, sizeof(ssid));
        memcpy(wifi_config.sta.password, password, sizeof(password));
        ESP_LOGI(debug_tag, "WiFi credentials(ssid, password): %s, %s", wifi_config.sta.ssid, wifi_config.sta.password);
        
        error = esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
        if(error != ESP_OK) {
            ESP_LOGW(debug_tag, "Failed to configure esp wifi: %d", error);
            return ESP_FAIL;
        }
        xEventGroupSetBits(wifi_event_group, BIT2);
    }
    else {
        ESP_LOGI(debug_tag, "No wifi credentials was found: %d", error);
    }
    
    error = esp_wifi_start();
    if(error != ESP_OK) {
        ESP_LOGW(debug_tag, "Failed to start esp wifi: %d", error);
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

