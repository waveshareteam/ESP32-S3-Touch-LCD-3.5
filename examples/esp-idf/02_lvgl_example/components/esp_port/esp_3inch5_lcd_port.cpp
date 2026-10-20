

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/i2c_master.h"

#include "esp_lcd_touch_ft6336.h"
#include "esp_lcd_st7796.h"

#include "esp_check.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"

#include <esp_attr.h>
#include <esp_system.h>

#define EXAMPLE_SPI_HOST SPI2_HOST
#define EXAMPLE_LCD_PIXEL_CLOCK_HZ (80 * 1000 * 1000)

#define EXAMPLE_PIN_LCD_MISO GPIO_NUM_NC
#define EXAMPLE_PIN_LCD_MOSI GPIO_NUM_1
#define EXAMPLE_PIN_LCD_SCLK GPIO_NUM_5
#define EXAMPLE_PIN_LCD_CS GPIO_NUM_NC
#define EXAMPLE_PIN_LCD_DC GPIO_NUM_3
#define EXAMPLE_PIN_LCD_RST GPIO_NUM_NC
#define EXAMPLE_PIN_LCD_BL GPIO_NUM_6

#define EXAMPLE_PIN_TP_INT GPIO_NUM_NC
#define EXAMPLE_PIN_TP_RST GPIO_NUM_NC

#define LCD_BL_LEDC_TIMER LEDC_TIMER_1
#define LCD_BL_LEDC_MODE LEDC_LOW_SPEED_MODE
#define LCD_BL_LEDC_CHANNEL LEDC_CHANNEL_0
#define LCD_BL_LEDC_DUTY_RES LEDC_TIMER_10_BIT // Set duty resolution to 13 bits
#define LCD_BL_LEDC_DUTY (1024)                // Set duty to 50%. (2 ** 13) * 50% = 4096
#define LCD_BL_LEDC_FREQUENCY (5000)          // Frequency in Hertz. Set frequency at 5 kHz

static const char *TAG = "esp_3inch5_lcd_port";

RTC_NOINIT_ATTR uint32_t reset_flag = 0;

void soft_reset_once(void)
{
    if (esp_reset_reason() == ESP_RST_POWERON)
    {
        fflush(stdout);
        esp_restart();
    }
}

void esp_3inch5_display_port_init(esp_lcd_panel_io_handle_t *io_handle, esp_lcd_panel_handle_t *panel_handle, size_t max_transfer_sz)
{

    ESP_LOGI(TAG, "SPI BUS init");
    spi_bus_config_t buscfg = {};
    buscfg.sclk_io_num = EXAMPLE_PIN_LCD_SCLK;
    buscfg.mosi_io_num = EXAMPLE_PIN_LCD_MOSI;
    buscfg.miso_io_num = EXAMPLE_PIN_LCD_MISO;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;
    buscfg.max_transfer_sz = max_transfer_sz;

    ESP_ERROR_CHECK(spi_bus_initialize(EXAMPLE_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));
    // soft_reset_once();
    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_spi_config_t io_config = {};
    io_config.cs_gpio_num = EXAMPLE_PIN_LCD_CS;
    io_config.dc_gpio_num = EXAMPLE_PIN_LCD_DC;
    io_config.spi_mode = 0;
    io_config.pclk_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ;
    io_config.trans_queue_depth = 10;
    io_config.on_color_trans_done = NULL;
    io_config.user_ctx = NULL;
    io_config.lcd_cmd_bits = 8;
    io_config.lcd_param_bits = 8;
    // Attach the LCD to the SPI bus
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)EXAMPLE_SPI_HOST, &io_config, io_handle));
    soft_reset_once();

    esp_lcd_panel_dev_config_t panel_config = {};
    panel_config.reset_gpio_num = EXAMPLE_PIN_LCD_RST;
    panel_config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR;
    panel_config.bits_per_pixel = 16;

    
    esp_lcd_new_panel_st7796(*io_handle, &panel_config, panel_handle);
    ESP_ERROR_CHECK(esp_lcd_panel_reset(*panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(*panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(*panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(*panel_handle, true));
    
}

void esp_3inch5_touch_port_init(esp_lcd_touch_handle_t *touch_handle, i2c_master_bus_handle_t bus_handle, uint16_t xmax, uint16_t ymax, uint16_t rotation)
{
    esp_lcd_panel_io_handle_t touch_io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t touch_io_config = {};
    touch_io_config.dev_addr = ESP_LCD_TOUCH_IO_I2C_FT6336_ADDRESS,
    touch_io_config.control_phase_bytes = 1;
    touch_io_config.dc_bit_offset = 0;
    touch_io_config.lcd_cmd_bits = 8;
    touch_io_config.flags.disable_control_phase = 1;
    touch_io_config.scl_speed_hz = 400 * 1000;

    /* Touch IO handle */
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(bus_handle, &touch_io_config, &touch_io_handle));
    esp_lcd_touch_config_t tp_cfg = {};
    tp_cfg.x_max = xmax < ymax ? xmax : ymax;
    tp_cfg.y_max = xmax < ymax ? ymax : xmax;
    ;
    tp_cfg.rst_gpio_num = EXAMPLE_PIN_TP_RST;
    tp_cfg.int_gpio_num = EXAMPLE_PIN_TP_INT;
    tp_cfg.flags = {
        .swap_xy = 0,
        .mirror_x = 0,
        .mirror_y = 0,
    };

    if (90 == rotation)
    {
        tp_cfg.flags.swap_xy = 1;
        tp_cfg.flags.mirror_x = 0;
        tp_cfg.flags.mirror_y = 1;
    }
    else if (180 == rotation)
    {
        tp_cfg.flags.swap_xy = 0;
        tp_cfg.flags.mirror_x = 1;
        tp_cfg.flags.mirror_y = 1;
    }
    else if (270 == rotation)
    {
        tp_cfg.flags.swap_xy = 1;
        tp_cfg.flags.mirror_x = 1;
        tp_cfg.flags.mirror_y = 0;
    }

    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_ft6336(touch_io_handle, &tp_cfg, touch_handle));
}

void esp_3inch5_brightness_port_init(void)
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {};
    ledc_timer.speed_mode = LCD_BL_LEDC_MODE,
    ledc_timer.timer_num = LCD_BL_LEDC_TIMER;
    ledc_timer.duty_resolution = LCD_BL_LEDC_DUTY_RES;
    ledc_timer.freq_hz = LCD_BL_LEDC_FREQUENCY; // Set output frequency at 5 kHz
    ledc_timer.clk_cfg = LEDC_AUTO_CLK;
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {};
    ledc_channel.speed_mode = LCD_BL_LEDC_MODE;
    ledc_channel.channel = LCD_BL_LEDC_CHANNEL;
    ledc_channel.timer_sel = LCD_BL_LEDC_TIMER;
    ledc_channel.intr_type = LEDC_INTR_DISABLE;
    ledc_channel.gpio_num = EXAMPLE_PIN_LCD_BL;
    ledc_channel.duty = 0; // Set duty to 0%
    ledc_channel.hpoint = 0;
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

void esp_3inch5_brightness_port_set(uint8_t brightness)
{
    if (brightness > 100)
    {
        brightness = 100;
        ESP_LOGE(TAG, "Brightness value out of range");
    }
    uint32_t duty = (brightness * (LCD_BL_LEDC_DUTY - 1)) / 100;

    ESP_ERROR_CHECK(ledc_set_duty(LCD_BL_LEDC_MODE, LCD_BL_LEDC_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LCD_BL_LEDC_MODE, LCD_BL_LEDC_CHANNEL));

    ESP_LOGI(TAG, "LCD brightness set to %d%%", brightness);
}
