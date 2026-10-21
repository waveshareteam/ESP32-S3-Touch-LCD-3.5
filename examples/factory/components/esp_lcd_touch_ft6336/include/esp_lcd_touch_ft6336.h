/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ESP LCD touch: FT6336
 */

#pragma once

#include "esp_lcd_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create a new FT6336 touch driver
 *
 * @note The I2C communication should be initialized before use this function.
 *
 * @param io LCD/Touch panel IO handle
 * @param config: Touch configuration
 * @param out_touch: Touch instance handle
 * @return
 *      - ESP_OK                    on success
 *      - ESP_ERR_NO_MEM            if there is no memory for allocating main structure
 */
esp_err_t esp_lcd_touch_new_i2c_ft6336(const esp_lcd_panel_io_handle_t io, const esp_lcd_touch_config_t *config, esp_lcd_touch_handle_t *out_touch);

typedef enum {
    ESP_LCD_TOUCH_FT6336_GESTURE_NONE = 0x00,
    ESP_LCD_TOUCH_FT6336_GESTURE_MOVE_UP = 0x10,
    ESP_LCD_TOUCH_FT6336_GESTURE_MOVE_LEFT = 0x14,
    ESP_LCD_TOUCH_FT6336_GESTURE_MOVE_DOWN = 0x18,
    ESP_LCD_TOUCH_FT6336_GESTURE_MOVE_RIGHT = 0x1C,
    ESP_LCD_TOUCH_FT6336_GESTURE_DOUBLE_CLICK = 0x24,
    ESP_LCD_TOUCH_FT6336_GESTURE_ZOOM_IN = 0x48,
    ESP_LCD_TOUCH_FT6336_GESTURE_ZOOM_OUT = 0x49,
} esp_lcd_touch_ft6336_gesture_t;

esp_err_t esp_lcd_touch_ft6336_get_gesture(esp_lcd_touch_handle_t tp, esp_lcd_touch_ft6336_gesture_t *gesture);
/**
 * @brief Clear the stored gesture ID and verify register 0xD3 reads zero.
 * @return ESP_OK on verified clear, an I2C error, or ESP_ERR_INVALID_RESPONSE
 *         if the controller retains a nonzero ID. Do not treat a retained
 *         result as a new gesture.
 */
esp_err_t esp_lcd_touch_ft6336_clear_gesture(esp_lcd_touch_handle_t tp);
esp_err_t esp_lcd_touch_ft6336_set_gesture_enabled(esp_lcd_touch_handle_t tp, bool enabled);

/**
 * @brief I2C address of the FT6336 controller
 *
 */
#define ESP_LCD_TOUCH_IO_I2C_FT6336_ADDRESS (0x38)

/**
 * @brief Touch IO configuration structure
 *
 */
#define ESP_LCD_TOUCH_IO_I2C_FT6336_CONFIG()           \
    {                                       \
        .dev_addr = ESP_LCD_TOUCH_IO_I2C_FT6336_ADDRESS, \
        .control_phase_bytes = 1,           \
        .dc_bit_offset = 0,                 \
        .lcd_cmd_bits = 8,                  \
        .flags =                            \
        {                                   \
            .disable_control_phase = 1,     \
        },                                  \
        .scl_speed_hz = 400 * 1000          \
    }


#ifdef __cplusplus
}
#endif
