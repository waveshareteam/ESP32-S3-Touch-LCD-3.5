#pragma once
#include <stdbool.h>
#include "esp_lcd_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief BSP touch configuration structure
 *
 */
typedef struct {
    void *dummy;    /*!< Prepared for future use. */
} bsp_touch_config_t;

/**
 * @brief Create new touchscreen
 *
 * If you want to free resources allocated by this function, you can use esp_lcd_touch API, ie.:
 *
 * \code{.c}
 * esp_lcd_touch_del(tp);
 * \endcode
 *
 * @param[in]  config    touch configuration
 * @param[out] ret_touch esp_lcd_touch touchscreen handle
 * @return
 *      - ESP_OK         On success
 *      - Else           esp_lcd_touch failure
 */
esp_err_t bsp_touch_new(const bsp_touch_config_t *config, esp_lcd_touch_handle_t *ret_touch);

/**
 * @brief Enable or disable FT6336 hardware gesture recognition (register 0xD0).
 *
 * Enabling clears the stored 0xD3 result and verifies it reads zero before
 * starting recognition. Switching 0xD0 alone does not clear the last gesture.
 * Disable when leaving a gesture-only screen to restore normal coordinate input.
 * Call from task context after touchscreen initialization; the foreground APP
 * owns this mode and must stop its reader before changing the mode.
 *
 * @return ESP_OK on success, ESP_ERR_INVALID_STATE if touch is unavailable,
 *         an I2C error, or ESP_ERR_INVALID_RESPONSE if the previous gesture
 *         could not be cleared and verified.
 */
esp_err_t bsp_touch_set_gesture_enabled(bool enabled);

/**
 * @brief Read the FT6336 double-click result from hardware register 0xD3.
 *
 * The controller recognizes gesture ID 0x24; no software timing or LVGL
 * click events are used. Enable gesture mode before polling this function.
 * Nonzero results are cleared and verified before reporting detection, so
 * the same stored result cannot be replayed on the next APP entry.
 * Poll from one task and latch detected events until the UI handles them.
 *
 * @param[out] detected True when the chip reports a double click.
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG for a null output,
 *         ESP_ERR_INVALID_STATE if touch is unavailable, an I2C error,
 *         or ESP_ERR_INVALID_RESPONSE if clearing the result is not confirmed.
 */
esp_err_t bsp_touch_get_double_click(bool *detected);

#ifdef __cplusplus
}
#endif
