#include <stdio.h>

#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "driver/gpio.h"

#include "driver/i2c_master.h"

#include "esp_io_expander_tca9554.h"

#include "lvgl.h"
#include "demos/lv_demos.h"

#include "esp_lvgl_port.h"

#include "lvgl_ui.h"

#include "esp_check.h"
#include "esp_log.h"

#include "iot_button.h"
#include "button_gpio.h"

#include "esp_axp2101_port.h"
#include "esp_camera_port.h"
#include "esp_es8311_port.h"
#include "esp_pcf85063_port.h"
#include "esp_qmi8658_port.h"
#include "esp_sdcard_port.h"
#include "esp_wifi_port.h"
#include "esp_3inch5_lcd_port.h"

#define EXAMPLE_PIN_I2C_SDA GPIO_NUM_8
#define EXAMPLE_PIN_I2C_SCL GPIO_NUM_7

#define EXAMPLE_PIN_BUTTON GPIO_NUM_0

#define EXAMPLE_DISPLAY_ROTATION 0

#if EXAMPLE_DISPLAY_ROTATION == 90 || EXAMPLE_DISPLAY_ROTATION == 270
#define EXAMPLE_LCD_H_RES 480
#define EXAMPLE_LCD_V_RES 320
#else
#define EXAMPLE_LCD_H_RES 320
#define EXAMPLE_LCD_V_RES 480
#endif

#define LCD_BUFFER_SIZE EXAMPLE_LCD_H_RES *EXAMPLE_LCD_V_RES / 8

#define I2C_PORT_NUM 0

static const char *TAG = "lvgl_image";

i2c_master_bus_handle_t i2c_bus_handle;

esp_lcd_panel_io_handle_t io_handle = NULL;
esp_lcd_panel_handle_t panel_handle = NULL;
esp_io_expander_handle_t expander_handle = NULL;
esp_lcd_touch_handle_t touch_handle = NULL;
lv_disp_drv_t disp_drv;

lv_display_t *lvgl_disp = NULL;
lv_indev_t *lvgl_touch_indev = NULL;

bool touch_test_done = false;
// sdmmc_card_t *card = NULL;

void i2c_bus_init(void);
void io_expander_init(void);
void lv_port_init(void);

lv_obj_t *img = NULL;

#define MAX_FILENAME_LENGTH 256
#define MAX_FILE_COUNT 100

char image_filenames[MAX_FILE_COUNT][MAX_FILENAME_LENGTH];
int bin_file_count = 0;

bool is_image_file(const char *filename)
{
    const char *ext = strrchr(filename, '.');
    if (ext != NULL)
    {
        if ((strcmp(ext, ".bin") == 0) || (strcmp(ext, ".jpg") == 0) || (strcmp(ext, ".jpeg") == 0) || (strcmp(ext, ".png") == 0))
            return true;
    }
    return false;
}

void read_image_files(const char *dir_path)
{
    lv_fs_dir_t dir;
    lv_fs_res_t res = lv_fs_dir_open(&dir, dir_path);
    if (res != LV_FS_RES_OK)
    {
        return;
    }
    char filename[MAX_FILENAME_LENGTH];
    while (lv_fs_dir_read(&dir, filename) == LV_FS_RES_OK && filename[0] != '\0')
    {
        printf(": %s \r\n", filename);
        if (is_image_file(filename))
        {
            if (bin_file_count < MAX_FILE_COUNT)
            {
                strncpy(image_filenames[bin_file_count], filename, MAX_FILENAME_LENGTH - 1);
                image_filenames[bin_file_count][MAX_FILENAME_LENGTH - 1] = '\0';
                bin_file_count++;
            }
            else
            {
                break;
            }
        }
    }
    lv_fs_dir_close(&dir);
}

void print_image_filenames()
{
    for (int i = 0; i < bin_file_count; i++)
    {
        printf("Found image file: %s \r\n", image_filenames[i]);
    }
}

static void img_callback(lv_timer_t *timer)
{
    char str_buf[300];
    static uint16_t img_index = 0;
    snprintf(str_buf, sizeof(str_buf), "S:images/%s", image_filenames[img_index]);
    lv_img_set_src(img, str_buf);
    if (++img_index >= bin_file_count)
    {
        img_index = 0;
    }
}

static void img_gesture_event_cb(lv_event_t *e)
{
    char str_buf[300];
    static int img_index = 0;

    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_GESTURE)
    {
        // printf("img_gesture_event_cb\r\n");
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());

        if (dir == LV_DIR_LEFT)
        {
            if (++img_index >= bin_file_count)
            {
                img_index = 0;
            }
            lv_indev_wait_release(lv_indev_get_act());
        }
        else if (dir == LV_DIR_RIGHT)
        {
            if (--img_index < 0)
            {
                img_index = bin_file_count - 1;
            }
            lv_indev_wait_release(lv_indev_get_act());
        }
        snprintf(str_buf, sizeof(str_buf), "S:images/%s", image_filenames[img_index]);
        lv_img_set_src(img, str_buf);
    }
}

extern void lv_fs_fatfs_init(void);
extern "C" void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    i2c_bus_init();
    io_expander_init();
    esp_3inch5_display_port_init(&io_handle, &panel_handle, LCD_BUFFER_SIZE);
    esp_3inch5_touch_port_init(&touch_handle, i2c_bus_handle, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES, EXAMPLE_DISPLAY_ROTATION);
    esp_axp2101_port_init(i2c_bus_handle);
    vTaskDelay(pdMS_TO_TICKS(100));
    // esp_es8311_port_init(i2c_bus_handle);
    // esp_qmi8658_port_init(i2c_bus_handle);
    // esp_pcf85063_port_init(i2c_bus_handle);
    esp_sdcard_port_init();
    // esp_camera_port_init(I2C_PORT_NUM);
    // esp_wifi_port_init("WSTEST", "waveshare0755");

    esp_3inch5_brightness_port_init();
    esp_3inch5_brightness_port_set(80);
    lv_port_init();
    lv_fs_fatfs_init();

    // button_init();
    // touch_test();

    lv_obj_t *obj;
    char str_buf[300];
    printf("read_image_files\r\n");
    read_image_files("S:images");
    print_image_filenames();

    if (lvgl_port_lock(0))
    {
        // lv_demo_benchmark();
        // lv_demo_music();
        // lv_demo_widgets();
        // lvgl_ui_init();

        img = lv_img_create(lv_scr_act());
        lv_obj_set_size(img, 320, 480);
        lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);
        snprintf(str_buf, sizeof(str_buf), "S:images/%s", image_filenames[0]);
        printf("img_set_src: %s\r\n", str_buf);
        lv_img_set_src(img, str_buf);
        lv_obj_clear_flag(img, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(img, LV_OBJ_FLAG_GESTURE_BUBBLE);
        // lv_obj_add_event_cb(img, img_gesture_event_cb, LV_EVENT_ALL, NULL);
        lv_obj_add_event_cb(lv_scr_act(), img_gesture_event_cb, LV_EVENT_GESTURE, NULL);
        // lv_obj_set_gesture_dir(img, LV_DIR_LEFT | LV_DIR_RIGHT);
        // lv_timer_create(img_callback, 1000, NULL);

        lvgl_port_unlock();
    }
}

void i2c_bus_init(void)
{
    i2c_master_bus_config_t i2c_mst_config = {};
    i2c_mst_config.clk_source = I2C_CLK_SRC_DEFAULT;
    i2c_mst_config.i2c_port = (i2c_port_num_t)I2C_PORT_NUM;
    i2c_mst_config.scl_io_num = EXAMPLE_PIN_I2C_SCL;
    i2c_mst_config.sda_io_num = EXAMPLE_PIN_I2C_SDA;
    i2c_mst_config.glitch_ignore_cnt = 7;
    i2c_mst_config.flags.enable_internal_pullup = 1;

    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &i2c_bus_handle));
}

void io_expander_init(void)
{
    ESP_ERROR_CHECK(esp_io_expander_new_i2c_tca9554(i2c_bus_handle, ESP_IO_EXPANDER_I2C_TCA9554_ADDRESS_000, &expander_handle));
    ESP_ERROR_CHECK(esp_io_expander_set_dir(expander_handle, IO_EXPANDER_PIN_NUM_0 | IO_EXPANDER_PIN_NUM_1, IO_EXPANDER_OUTPUT));
    ESP_ERROR_CHECK(esp_io_expander_set_level(expander_handle, IO_EXPANDER_PIN_NUM_1, 0));
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_ERROR_CHECK(esp_io_expander_set_level(expander_handle, IO_EXPANDER_PIN_NUM_1, 1));
    ESP_ERROR_CHECK(esp_io_expander_set_level(expander_handle, IO_EXPANDER_PIN_NUM_0, 1));
    vTaskDelay(pdMS_TO_TICKS(100));
}

void lv_port_init(void)
{
    lvgl_port_cfg_t port_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_port_init(&port_cfg);
    ESP_LOGI(TAG, "Adding LCD screen");
    lvgl_port_display_cfg_t display_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .control_handle = NULL,
        .buffer_size = LCD_BUFFER_SIZE,
        .double_buffer = true,
        .trans_size = 0,
        .hres = EXAMPLE_LCD_H_RES,
        .vres = EXAMPLE_LCD_V_RES,
        .monochrome = false,
        .rotation = {
            .swap_xy = 0,
            .mirror_x = 1,
            .mirror_y = 0,
        },
        .flags = {
            .buff_dma = 0,
            .buff_spiram = 1,
            .sw_rotate = 1,
            .full_refresh = 0,
            .direct_mode = 0,
        },
    };

#if EXAMPLE_DISPLAY_ROTATION == 90
    display_cfg.rotation.swap_xy = 1;
    display_cfg.rotation.mirror_x = 1;
    display_cfg.rotation.mirror_y = 1;
    -
#elif EXAMPLE_DISPLAY_ROTATION == 180
    display_cfg.rotation.swap_xy = 0;
    display_cfg.rotation.mirror_x = 0;
    display_cfg.rotation.mirror_y = 1;

#elif EXAMPLE_DISPLAY_ROTATION == 270
    display_cfg.rotation.swap_xy = 1;
    display_cfg.rotation.mirror_x = 0;
    display_cfg.rotation.mirror_y = 0;
#endif

        lvgl_disp = lvgl_port_add_disp(&display_cfg);
    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = lvgl_disp,
        .handle = touch_handle,
    };
    lvgl_touch_indev = lvgl_port_add_touch(&touch_cfg);
}

