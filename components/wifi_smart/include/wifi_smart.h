/* 
 * File:   wifi_smart.h
 * Author: ripx (ALEXANDER BERDNIKOV)
 *
 * Created on July 10, 2017, 10:33 PM
 */

#ifndef WIFI_SMART_H
#define WIFI_SMART_H

typedef esp_err_t (*wifi_smart_cb_t)(void);

esp_err_t wifi_smart_init(wifi_smart_cb_t cb);

#endif /* WIFI_SMART_H */

