/* 
 * File:   udp_transport.c
 * Author: ripx (ripx@me.com)
 */

#include <stdio.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"
#include "tcpip_adapter.h"

#include "lwip/err.h"
#include "lwip/udp.h"

#define IP4_BROADCAST(ipaddr, if_ip) (ipaddr)->addr = (if_ip.ip.addr | (~if_ip.netmask.addr));
#define IP4_2_IP(ip4addr, ipaddr) (ipaddr)->u_addr.ip4 = ip4addr; (ipaddr)->type = IPADDR_TYPE_V4;

static const char *debug_tag = "UDP_TRANSPORT";

static int port = CONFIG_NETWORK_PORT;

static ip_addr_t broadcast_addr;
static tcpip_adapter_ip_info_t ip;

static struct udp_pcb *local_pcb;

static void udp_transport_data_received(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t _port) {
    
    if(p != NULL) {
        ESP_LOGI(debug_tag, "DATA RECEIVED FROM:"IPSTR":%d", IP2STR(ip_2_ip4(addr)), _port);
    }
}

esp_err_t udp_transport_init(esp_interface_t _if) {
    //Getting IP address of interface
    memset(&ip, 0, sizeof(tcpip_adapter_ip_info_t));
    
    if (tcpip_adapter_get_ip_info(_if, &ip) == 0) {
        ip4_addr_t ip4_broadcast_addr;
        IP4_BROADCAST(&ip4_broadcast_addr, ip);
        IP4_2_IP(ip4_broadcast_addr, &broadcast_addr);
        ESP_LOGI(debug_tag, "NETWORK BROADCAST ADDRESS:"IPSTR, IP2STR(&ip4_broadcast_addr));
        
        local_pcb = udp_new();
        
        udp_bind(local_pcb, IP_ADDR_ANY, port);
        udp_recv(local_pcb, udp_transport_data_received, NULL);
    }
    else {
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

void udp_transport_send_broadcast(void *data, const uint16_t data_length) {
    struct pbuf *payload_buffer = pbuf_alloc(PBUF_TRANSPORT, data_length, PBUF_REF);
    payload_buffer->payload = data;
    
    udp_sendto(local_pcb, payload_buffer, &broadcast_addr, port);
    
    pbuf_free(payload_buffer);
}