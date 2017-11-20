/* 
 * File:   udp_transport.h
 * Author: ripx (ripx@me.com)
 */

#ifndef UDP_TRANSPORT_H
#define UDP_TRANSPORT_H

#include "esp_err.h"
#include "tcpip_adapter.h"

esp_err_t udp_transport_init(esp_interface_t _if);
void udp_transport_send_broadcast(void *data, const uint16_t data_length);

#endif /* UDP_TRANSPORT_H */

