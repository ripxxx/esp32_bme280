/* 
 * File:   wifi_config.h
 * Author: ripx (ALEXANDER BERDNIKOV)
 *
 * Created on July 10, 2017, 10:31 PM
 */

#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#define WIFI_SSID_MAX_LENGTH 32
#define WIFI_PASSWORD_MAX_LENGTH 64

esp_err_t wifi_config_close(void);
esp_err_t wifi_config_erase_credentials(void);
esp_err_t wifi_config_open(void);
esp_err_t wifi_config_read_credentials(char ssid[WIFI_SSID_MAX_LENGTH], char password[WIFI_PASSWORD_MAX_LENGTH]);
esp_err_t wifi_config_write_credentials(const char *ssid, const char *password);

#endif /* WIFI_CONFIG_H */

