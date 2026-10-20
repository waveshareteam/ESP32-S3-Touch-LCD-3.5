#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch.h"
#include "esp_camera.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Physical pins come from the 3.5-inch 260911 factory project. */
#define BSP_I2C_NUM             I2C_NUM_0
#define BSP_I2C_SDA             GPIO_NUM_8
#define BSP_I2C_SCL             GPIO_NUM_7
#define BSP_I2C_FREQ_HZ         400000
#define BSP_LCD_H_RES           320
#define BSP_LCD_V_RES           480
#define BSP_LCD_SPI_HOST        SPI2_HOST
#define BSP_LCD_MOSI            GPIO_NUM_1
#define BSP_LCD_SCLK            GPIO_NUM_5
#define BSP_LCD_DC              GPIO_NUM_3
#define BSP_LCD_CS              GPIO_NUM_NC
#define BSP_LCD_RST             GPIO_NUM_NC
#define BSP_LCD_BACKLIGHT       GPIO_NUM_6
#define BSP_TOUCH_RST           GPIO_NUM_NC
#define BSP_TOUCH_INT           GPIO_NUM_NC
#define BSP_BUTTON_BOOT         GPIO_NUM_0
#define BSP_SD_CLK              GPIO_NUM_11
#define BSP_SD_CMD              GPIO_NUM_10
#define BSP_SD_D0               GPIO_NUM_9
#define BSP_SD_MOUNT_POINT      "/sdcard"
#define BSP_I2S_MCLK            GPIO_NUM_12
#define BSP_I2S_BCLK            GPIO_NUM_13
#define BSP_I2S_LRCLK           GPIO_NUM_15
#define BSP_I2S_DOUT            GPIO_NUM_16
#define BSP_I2S_DIN             GPIO_NUM_14
#define BSP_AUDIO_PA            GPIO_NUM_NC
#define BSP_CAMERA_XCLK         GPIO_NUM_38
#define BSP_CAMERA_PCLK         GPIO_NUM_41
#define BSP_CAMERA_VSYNC        GPIO_NUM_17
#define BSP_CAMERA_HREF         GPIO_NUM_18
#define BSP_CAMERA_D0           GPIO_NUM_45
#define BSP_CAMERA_D1           GPIO_NUM_47
#define BSP_CAMERA_D2           GPIO_NUM_48
#define BSP_CAMERA_D3           GPIO_NUM_46
#define BSP_CAMERA_D4           GPIO_NUM_42
#define BSP_CAMERA_D5           GPIO_NUM_40
#define BSP_CAMERA_D6           GPIO_NUM_39
#define BSP_CAMERA_D7           GPIO_NUM_21

typedef struct {
    esp_err_t i2c, expander, power, display, touch;
    esp_err_t button, audio, rtc, imu, sdcard, camera, wifi;
} bsp_init_status_t;

/* Call once from app_main before starting APPs. Required power/display errors
 * are returned; optional peripheral failures are logged and stored in status. */
esp_err_t bsp_init(void);
const bsp_init_status_t *bsp_get_init_status(void);
esp_err_t bsp_i2c_init(void);
i2c_master_bus_handle_t bsp_i2c_get_handle(void);
esp_err_t bsp_io_expander_init(void);
esp_err_t bsp_io_expander_read_input(bool *high);
esp_err_t bsp_power_init(void);

lv_display_t *bsp_display_start(void);
lv_display_t *bsp_display_get(void);
lv_indev_t *bsp_display_get_input_dev(void);
esp_lcd_touch_handle_t bsp_touch_get_handle(void);
/* Same APP-facing convention as RLCD: bool success, 0 waits indefinitely. */
bool bsp_display_lock(uint32_t timeout_ms);
void bsp_display_unlock(void);
esp_err_t bsp_display_brightness_set(int percent);
int bsp_display_brightness_get(void);
esp_err_t bsp_user_button_init(void);
uint32_t bsp_user_button_get_press_count(void);

typedef struct {
    bool connected, charging, vbus_present;
    uint16_t voltage_mv;
    int percentage;
} bsp_battery_info_t;
esp_err_t bsp_battery_get_info(bsp_battery_info_t *info);

typedef struct {
    uint16_t year;
    uint8_t month, day, hour, minute, second, weekday;
} bsp_sensor_datetime_t;
typedef struct {
    float accel_x, accel_y, accel_z; /* g */
    float gyro_x, gyro_y, gyro_z;    /* degrees/second */
    float temperature_c;
} bsp_imu_data_t;
esp_err_t bsp_rtc_init(void);
esp_err_t bsp_rtc_get_datetime(bsp_sensor_datetime_t *datetime);
esp_err_t bsp_rtc_set_datetime(const bsp_sensor_datetime_t *datetime);
esp_err_t bsp_imu_init(void);
esp_err_t bsp_imu_get_data(bsp_imu_data_t *data);

typedef struct {
    bool mounted;
    uint64_t capacity_bytes, total_bytes, free_bytes;
} bsp_sdcard_info_t;
esp_err_t bsp_sdcard_mount(void);
/* Active APPs acquire/release a lease so Settings cannot unmount an in-use card. */
esp_err_t bsp_sdcard_acquire(void);
void bsp_sdcard_release(void);
esp_err_t bsp_sdcard_unmount(void);
esp_err_t bsp_sdcard_get_info(bsp_sdcard_info_t *info);

/* Idempotent, serialized initialization. Camera/AI workers may call it again
 * after a boot-time failure; successful initialization is reused. */
esp_err_t bsp_camera_init(void);
camera_fb_t *bsp_camera_get_frame(void);
void bsp_camera_return_frame(camera_fb_t *frame);
/* Toggle both sensor mirror axes, rotating the image 180 degrees.
 * Call from the camera worker between frames, not from a button/LVGL callback. */
esp_err_t bsp_camera_rotate_180(void);
bool bsp_camera_acquire(uint32_t timeout_ms);
void bsp_camera_release(void);
/* Fresh JPEG for AI; caller owns the returned buffer and frees it with free(). */
esp_err_t bsp_camera_capture_jpeg(uint8_t **jpeg, size_t *size);
/* Optional short preview at <=5 fps, followed by the exact RGB565 snapshot
 * used for JPEG encoding (final_frame=true). Frames are borrowed only during
 * the callback; copy what is needed and never call LVGL from the callback.
 * Return false to cancel. A null frame is a cancellation check only.
 * The camera lock/framebuffer is released before encoding and network I/O. */
typedef bool (*bsp_camera_preview_cb_t)(const camera_fb_t *frame, bool final_frame, void *context);
esp_err_t bsp_camera_capture_jpeg_with_preview(uint8_t **jpeg, size_t *size,
                                             bsp_camera_preview_cb_t preview, void *context);

/* ES8311 is one shared full-duplex codec, not an ES7210 echo-reference array.
 * One task acquires, opens, reads/writes, closes and releases a session.
 * Volume can be changed from another task; init does not play audio. */
esp_err_t bsp_audio_init(void);
bool bsp_audio_acquire(uint32_t timeout_ms);
void bsp_audio_release(void);
esp_err_t bsp_audio_open(uint32_t sample_rate, uint8_t channels);
esp_err_t bsp_audio_close(void);
esp_err_t bsp_audio_write(const void *data, size_t size);
/* Push silence through the complete TX ring before pausing/closing playback. */
esp_err_t bsp_audio_drain(void);
esp_err_t bsp_audio_read(void *data, size_t size);
esp_err_t bsp_audio_volume_set(int percent);
int bsp_audio_volume_get(void);

/* One shared ES8311. Ordinary APPs use the task-owned API above; Xiaozhi
 * holds one session across its control/input/playback tasks. Stop/join all
 * voice tasks before close/release. RX slots contain one physical microphone. */
typedef enum { BSP_AUDIO_OWNER_NONE, BSP_AUDIO_OWNER_APP, BSP_AUDIO_OWNER_XIAOZHI } bsp_audio_owner_t;
bsp_audio_owner_t bsp_audio_session_get_owner(void);
const char *bsp_audio_owner_name(bsp_audio_owner_t owner);
esp_err_t bsp_audio_session_acquire(bsp_audio_owner_t owner);
esp_err_t bsp_audio_session_release(bsp_audio_owner_t owner);
#define BSP_AUDIO_VOICE_CHANNELS 2
esp_err_t bsp_audio_voice_open(uint32_t sample_rate, bool capture);
esp_err_t bsp_audio_voice_read(void *pcm, size_t bytes, size_t *read, uint32_t timeout_ms);
esp_err_t bsp_audio_voice_write(void *pcm, size_t bytes, size_t *written, uint32_t timeout_ms);
esp_err_t bsp_audio_voice_mute(bool mute);
esp_err_t bsp_audio_voice_close(void);

/* Driver/netif initialization only. Credentials/scan/provisioning belong to APPs. */
esp_err_t bsp_wifi_init(void);
/* Start the initialized radio for scanning/provisioning. Does not connect. */
esp_err_t bsp_wifi_start(void);

#ifdef __cplusplus
}
#endif
