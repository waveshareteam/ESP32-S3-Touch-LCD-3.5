#pragma once

#include "esp_lcd_touch_ft6336.h"
#include "esp_lcd_st7796.h"
#include "driver/i2c_master.h"

void esp_3inch5_display_port_init(esp_lcd_panel_io_handle_t *io_handle, esp_lcd_panel_handle_t *panel_handle, size_t max_transfer_sz);
void esp_3inch5_touch_port_init(esp_lcd_touch_handle_t *touch_handle, i2c_master_bus_handle_t bus_handle, uint16_t xmax, uint16_t ymax, uint16_t rotation);
void esp_3inch5_brightness_port_init(void);
void esp_3inch5_brightness_port_set(uint8_t brightness);