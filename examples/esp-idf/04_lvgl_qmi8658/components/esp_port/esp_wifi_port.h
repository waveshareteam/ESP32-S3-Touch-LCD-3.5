#pragma once

#include <stdio.h>
#include "esp_wifi.h"

void esp_wifi_port_init(const char *ssid, const char *pass);
void esp_wifi_port_get_ip(char *ip);
bool esp_wifi_port_scan(wifi_ap_record_t *ap_info, uint16_t *scan_number, uint16_t scan_max_num);
esp_err_t esp_wifi_port_disconnect(void);
esp_err_t esp_wifi_port_connect(void);