#include "bsp/esp-bsp.h"
#include "bsp/touch.h"
#include "bsp_board_internal.h"

#include "esp_check.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "esp_codec_dev_defaults.h"
#include "esp_lcd_st7796.h"
#include "esp_lcd_touch_ft6336.h"
#include "driver/ledc.h"
#include "driver/sdmmc_host.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "bsp_3_5";
static i2c_master_bus_handle_t i2c_bus;
static esp_lcd_panel_io_handle_t lcd_io, touch_io;
static esp_lcd_panel_handle_t panel;
static esp_lcd_touch_handle_t touch;
static lv_display_t *display;
static lv_indev_t *input;
static bool backlight_ready, display_started, port_initialized;
static int brightness;
static SemaphoreHandle_t sd_mutex;
static sdmmc_card_t *sdcard;
static unsigned sd_leases;

/**************************************************************************************************
 * I2C interface
 **************************************************************************************************/
esp_err_t bsp_i2c_init(void)
{
    if (i2c_bus) return ESP_OK;
    ESP_RETURN_ON_ERROR(bsp_board_init_locks(), TAG, "Board locks");
    if (!sd_mutex) sd_mutex = xSemaphoreCreateMutex();
    if (!sd_mutex) return ESP_ERR_NO_MEM;
    i2c_master_bus_config_t cfg = {};
    cfg.i2c_port = BSP_I2C_NUM;
    cfg.sda_io_num = BSP_I2C_SDA;
    cfg.scl_io_num = BSP_I2C_SCL;
    cfg.clk_source = I2C_CLK_SRC_DEFAULT;
    cfg.glitch_ignore_cnt = 7;
    cfg.flags.enable_internal_pullup = true;
    return i2c_new_master_bus(&cfg, &i2c_bus);
}

esp_err_t bsp_i2c_deinit(void)
{
    if (!i2c_bus) return ESP_OK;
    // Child devices must be removed before deleting their common bus.
    if (touch || bsp_board_i2c_in_use()) return ESP_ERR_INVALID_STATE;
    esp_err_t ret = i2c_del_master_bus(i2c_bus);
    if (ret == ESP_OK) i2c_bus = nullptr;
    return ret;
}
i2c_master_bus_handle_t bsp_i2c_get_handle(void)
{
    return bsp_i2c_init() == ESP_OK ? i2c_bus : nullptr;
}

static esp_err_t sdcard_mount_locked(void)
{
    if (sdcard) return ESP_OK;
    esp_vfs_fat_sdmmc_mount_config_t cfg = {};
    cfg.format_if_mount_failed = false;
    cfg.max_files = 8;
    cfg.allocation_unit_size = 16 * 1024;
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
    slot.width = 1;
    slot.clk = BSP_SD_CLK;
    slot.cmd = BSP_SD_CMD;
    slot.d0 = BSP_SD_D0;
    slot.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
    esp_err_t ret = esp_vfs_fat_sdmmc_mount(BSP_SD_MOUNT_POINT, &host, &slot, &cfg, &sdcard);
    if (ret != ESP_OK) sdcard = nullptr;
    bsp_board_status.sdcard = ret;
    return ret;
}

esp_err_t bsp_sdcard_mount(void)
{
    if (!sd_mutex) return ESP_ERR_INVALID_STATE;
    xSemaphoreTake(sd_mutex, portMAX_DELAY);
    esp_err_t ret = sdcard_mount_locked();
    xSemaphoreGive(sd_mutex);
    return ret;
}

esp_err_t bsp_sdcard_acquire(void)
{
    if (!sd_mutex) return ESP_ERR_INVALID_STATE;
    xSemaphoreTake(sd_mutex, portMAX_DELAY);
    esp_err_t ret = sdcard_mount_locked();
    if (ret == ESP_OK) ++sd_leases;
    xSemaphoreGive(sd_mutex);
    return ret;
}

void bsp_sdcard_release(void)
{
    if (!sd_mutex) return;
    xSemaphoreTake(sd_mutex, portMAX_DELAY);
    if (sd_leases) --sd_leases;
    xSemaphoreGive(sd_mutex);
}

esp_err_t bsp_sdcard_unmount(void)
{
    if (!sd_mutex) return ESP_ERR_INVALID_STATE;
    xSemaphoreTake(sd_mutex, portMAX_DELAY);
    esp_err_t ret = sd_leases ? ESP_ERR_INVALID_STATE : ESP_OK;
    if (ret == ESP_OK && sdcard) {
        ret = esp_vfs_fat_sdcard_unmount(BSP_SD_MOUNT_POINT, sdcard);
        if (ret == ESP_OK) { sdcard = nullptr; bsp_board_status.sdcard = ESP_ERR_INVALID_STATE; }
    }
    xSemaphoreGive(sd_mutex);
    return ret;
}

esp_err_t bsp_sdcard_get_info(bsp_sdcard_info_t *info)
{
    if (!info) return ESP_ERR_INVALID_ARG;
    *info = {};
    if (!sd_mutex) return ESP_ERR_INVALID_STATE;
    xSemaphoreTake(sd_mutex, portMAX_DELAY);
    esp_err_t ret = ESP_ERR_INVALID_STATE;
    if (sdcard) {
        info->mounted = true;
        info->capacity_bytes = (uint64_t)sdcard->csd.capacity * sdcard->csd.sector_size;
        ret = esp_vfs_fat_info(BSP_SD_MOUNT_POINT, &info->total_bytes, &info->free_bytes);
    }
    xSemaphoreGive(sd_mutex);
    return ret;
}


/**************************************************************************************************
 * I2S audio interface: one shared ES8311 for playback and microphone capture.
 **************************************************************************************************/
static i2s_chan_handle_t tx, rx;
static const audio_codec_data_if_t *data_if;
static const audio_codec_ctrl_if_t *ctrl_if;
static const audio_codec_gpio_if_t *gpio_if;
static const audio_codec_if_t *codec_if;
static esp_codec_dev_handle_t codec;

static void bsp_audio_cleanup(void)
{
    if (codec) { esp_codec_dev_delete(codec); codec = nullptr; }
    if (codec_if) { audio_codec_delete_codec_if(codec_if); codec_if = nullptr; }
    if (ctrl_if) { audio_codec_delete_ctrl_if(ctrl_if); ctrl_if = nullptr; }
    if (gpio_if) { audio_codec_delete_gpio_if(gpio_if); gpio_if = nullptr; }
    if (data_if) { audio_codec_delete_data_if(data_if); data_if = nullptr; }
    if (tx) { i2s_channel_disable(tx); i2s_del_channel(tx); tx = nullptr; }
    if (rx) { i2s_channel_disable(rx); i2s_del_channel(rx); rx = nullptr; }
}

esp_err_t bsp_audio_deinit(void)
{
    // All APP workers must be stopped before releasing these BSP-owned handles.
    bsp_audio_cleanup();
    return ESP_OK;
}

static esp_err_t bsp_audio_init_channels(const i2s_std_config_t *i2s_config)
{
    i2s_chan_config_t channel = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    channel.dma_desc_num = BSP_AUDIO_DMA_DESCRIPTORS;
    channel.dma_frame_num = BSP_AUDIO_DMA_FRAMES;
    channel.auto_clear_after_cb = true;
    ESP_RETURN_ON_ERROR(i2s_new_channel(&channel, &tx, &rx), TAG, "I2S channels");
    i2s_std_config_t cfg = {};
    if (i2s_config) cfg = *i2s_config;
    else {
        cfg.clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000);
        cfg.slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
        cfg.gpio_cfg.mclk = BSP_I2S_MCLK;
        cfg.gpio_cfg.bclk = BSP_I2S_BCLK;
        cfg.gpio_cfg.ws = BSP_I2S_LRCLK;
        cfg.gpio_cfg.dout = BSP_I2S_DOUT;
        cfg.gpio_cfg.din = BSP_I2S_DIN;
    }
    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(tx, &cfg), TAG, "I2S TX");
    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(rx, &cfg), TAG, "I2S RX");
    ESP_RETURN_ON_ERROR(i2s_channel_enable(tx), TAG, "I2S TX enable");
    ESP_RETURN_ON_ERROR(i2s_channel_enable(rx), TAG, "I2S RX enable");
    audio_codec_i2s_cfg_t data = {};
    data.port = I2S_NUM_0;
    data.tx_handle = tx;
    data.rx_handle = rx;
    data_if = audio_codec_new_i2s_data(&data);
    return data_if ? ESP_OK : ESP_ERR_NO_MEM;
}

esp_err_t bsp_audio_init(const i2s_std_config_t *i2s_config)
{
    if (data_if) return ESP_OK;
    ESP_RETURN_ON_ERROR(bsp_i2c_init(), TAG, "I2C bus");
    esp_err_t ret = bsp_audio_init_channels(i2s_config);
    if (ret != ESP_OK) bsp_audio_cleanup();
    return ret;
}

esp_err_t bsp_audio_init_voice_24k(void)
{
    i2s_std_config_t cfg = {};
    cfg.clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(24000);
    cfg.slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
    cfg.gpio_cfg.mclk = BSP_I2S_MCLK;
    cfg.gpio_cfg.bclk = BSP_I2S_BCLK;
    cfg.gpio_cfg.ws = BSP_I2S_LRCLK;
    cfg.gpio_cfg.dout = BSP_I2S_DOUT;
    cfg.gpio_cfg.din = BSP_I2S_DIN;
    return bsp_audio_init(&cfg);
}

esp_codec_dev_handle_t bsp_audio_codec_speaker_init(void)
{
    if (codec) return codec;
    if (bsp_audio_init(nullptr) != ESP_OK) return nullptr;
    // Codec control uses an 8-bit address; the bus probe takes 7 bits.
    if (i2c_master_probe(bsp_i2c_get_handle(), ES8311_CODEC_DEFAULT_ADDR >> 1, 100) != ESP_OK) return nullptr;
    audio_codec_i2c_cfg_t ctrl = {};
    ctrl.port = BSP_I2C_NUM;
    ctrl.addr = ES8311_CODEC_DEFAULT_ADDR;
    ctrl.bus_handle = bsp_i2c_get_handle();
    ctrl_if = audio_codec_new_i2c_ctrl(&ctrl);
    gpio_if = audio_codec_new_gpio();
    if (!ctrl_if || !gpio_if) goto failed;
    {
        es8311_codec_cfg_t chip = {};
        chip.codec_mode = ESP_CODEC_DEV_WORK_MODE_BOTH;
        chip.ctrl_if = ctrl_if;
        chip.gpio_if = gpio_if;
        chip.pa_pin = BSP_AUDIO_PA;
        chip.use_mclk = true;
        chip.hw_gain.pa_voltage = 5.0f;
        chip.hw_gain.codec_dac_voltage = 3.3f;
        codec_if = es8311_codec_new(&chip);
        if (!codec_if) goto failed;
        esp_codec_dev_cfg_t dev = {};
        dev.dev_type = ESP_CODEC_DEV_TYPE_IN_OUT;
        dev.codec_if = codec_if;
        dev.data_if = data_if;
        codec = esp_codec_dev_new(&dev);
        if (!codec) goto failed;
    }
    return codec;
failed:
    bsp_audio_cleanup();
    return nullptr;
}

esp_codec_dev_handle_t bsp_audio_codec_microphone_init(void)
{
    // Both directions belong to the same physical ES8311. Never delete twice.
    return bsp_audio_codec_speaker_init();
}

// Board extensions retain bounded raw I2S transfers for the voice workers.
esp_err_t bsp_audio_i2s_read(void *data, size_t size, size_t *read, uint32_t timeout_ms)
{
    return rx ? i2s_channel_read(rx, data, size, read, timeout_ms) : ESP_ERR_INVALID_STATE;
}
esp_err_t bsp_audio_i2s_write(void *data, size_t size, size_t *written, uint32_t timeout_ms)
{
    return tx ? i2s_channel_write(tx, data, size, written, timeout_ms) : ESP_ERR_INVALID_STATE;
}

esp_err_t bsp_display_brightness_init(void)
{
    if (backlight_ready) return ESP_OK;
    ledc_timer_config_t timer = {};
    timer.speed_mode = LEDC_LOW_SPEED_MODE;
    timer.duty_resolution = LEDC_TIMER_10_BIT;
    timer.timer_num = LEDC_TIMER_1;
    timer.freq_hz = 5000;
    timer.clk_cfg = LEDC_AUTO_CLK;
    ESP_RETURN_ON_ERROR(ledc_timer_config(&timer), TAG, "Backlight timer");
    ledc_channel_config_t channel = {};
    channel.gpio_num = BSP_LCD_BACKLIGHT;
    channel.speed_mode = LEDC_LOW_SPEED_MODE;
    channel.channel = LEDC_CHANNEL_1; // Camera XCLK owns channel 0, timer 0.
    channel.timer_sel = LEDC_TIMER_1;
    ESP_RETURN_ON_ERROR(ledc_channel_config(&channel), TAG, "Backlight PWM");
    backlight_ready = true;
    return ESP_OK;
}

esp_err_t bsp_display_brightness_set(int percent)
{
    if (percent < 0 || percent > 100) return ESP_ERR_INVALID_ARG;
    if (!backlight_ready) return ESP_ERR_INVALID_STATE;
    ESP_RETURN_ON_ERROR(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, percent * 1023 / 100), TAG, "Backlight duty");
    ESP_RETURN_ON_ERROR(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1), TAG, "Backlight update");
    brightness = percent;
    return ESP_OK;
}
int bsp_display_brightness_get(void) { return brightness; }


esp_err_t bsp_display_backlight_off(void) { return bsp_display_brightness_set(0); }
esp_err_t bsp_display_backlight_on(void) { return bsp_display_brightness_set(CONFIG_BSP_LCD_BRIGHTNESS); }

esp_err_t bsp_display_new(const bsp_display_config_t *config, esp_lcd_panel_handle_t *ret_panel, esp_lcd_panel_io_handle_t *ret_io)
{
    if (!ret_panel || !ret_io) return ESP_ERR_INVALID_ARG;
    if (panel || lcd_io) return ESP_ERR_INVALID_STATE;
    *ret_panel = nullptr;
    *ret_io = nullptr;
    esp_lcd_panel_handle_t new_panel = nullptr;
    esp_lcd_panel_io_handle_t new_io = nullptr;
    bool bus_created = false;
    auto create = [&]() -> esp_err_t {
        ESP_RETURN_ON_ERROR(bsp_i2c_init(), TAG, "I2C");
        ESP_RETURN_ON_ERROR(bsp_io_expander_init(), TAG, "Expander");
        ESP_RETURN_ON_ERROR(bsp_power_init(), TAG, "Power");
        ESP_RETURN_ON_ERROR(bsp_display_brightness_init(), TAG, "Backlight");
        spi_bus_config_t bus = {};
        bus.sclk_io_num = BSP_LCD_SCLK;
        bus.mosi_io_num = BSP_LCD_MOSI;
        bus.miso_io_num = -1;
        bus.quadwp_io_num = -1;
        bus.quadhd_io_num = -1;
        bus.max_transfer_sz = config && config->max_transfer_sz > 0 ? config->max_transfer_sz : BSP_LCD_H_RES * CONFIG_BSP_LCD_BUFFER_HEIGHT * 2;
        ESP_RETURN_ON_ERROR(spi_bus_initialize(BSP_LCD_SPI_HOST, &bus, SPI_DMA_CH_AUTO), TAG, "LCD SPI");
        bus_created = true;
        esp_lcd_panel_io_spi_config_t io = {};
        io.cs_gpio_num = BSP_LCD_CS;
        io.dc_gpio_num = BSP_LCD_DC;
        io.spi_mode = 0;
        io.pclk_hz = CONFIG_BSP_LCD_SPI_CLOCK_HZ;
        io.trans_queue_depth = 10;
        io.lcd_cmd_bits = 8;
        io.lcd_param_bits = 8;
        ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)BSP_LCD_SPI_HOST, &io, &new_io), TAG, "LCD IO");
    #if CONFIG_BSP_LCD_POWERON_RESTART
        if (esp_reset_reason() == ESP_RST_POWERON) {
            ESP_LOGI(TAG, "Original LCD cold-start workaround: restarting once");
            esp_restart();
        }
    #endif
        esp_lcd_panel_dev_config_t cfg = {};
        cfg.reset_gpio_num = BSP_LCD_RST;
        cfg.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR;
        cfg.bits_per_pixel = 16;
        ESP_RETURN_ON_ERROR(esp_lcd_new_panel_st7796(new_io, &cfg, &new_panel), TAG, "ST7796 create");
        ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(new_panel), TAG, "LCD reset");
        ESP_RETURN_ON_ERROR(esp_lcd_panel_init(new_panel), TAG, "LCD init");
        ESP_RETURN_ON_ERROR(esp_lcd_panel_invert_color(new_panel, true), TAG, "LCD inversion");
        // Native portrait orientation from the original rotation=0 configuration.
        ESP_RETURN_ON_ERROR(esp_lcd_panel_swap_xy(new_panel, false), TAG, "LCD axes");
        ESP_RETURN_ON_ERROR(esp_lcd_panel_mirror(new_panel, true, false), TAG, "LCD mirror");

        return ESP_OK;
    };
    esp_err_t ret = create();
    if (ret != ESP_OK) {
        if (new_panel) esp_lcd_panel_del(new_panel);
        if (new_io) esp_lcd_panel_io_del(new_io);
        if (bus_created) spi_bus_free(BSP_LCD_SPI_HOST);
        return ret;
    }
    *ret_panel = new_panel;
    *ret_io = new_io;
    return ESP_OK;
}
esp_err_t bsp_touch_new(const bsp_touch_config_t *config, esp_lcd_touch_handle_t *ret_touch)
{
    (void)config;
    if (!ret_touch) return ESP_ERR_INVALID_ARG;
    if (touch || touch_io) return ESP_ERR_INVALID_STATE;
    *ret_touch = nullptr;
    ESP_RETURN_ON_ERROR(bsp_i2c_init(), TAG, "I2C");
    ESP_RETURN_ON_ERROR(bsp_io_expander_init(), TAG, "Expander");
    esp_lcd_panel_io_i2c_config_t io = {};
    io.dev_addr = ESP_LCD_TOUCH_IO_I2C_FT6336_ADDRESS;
    io.control_phase_bytes = 1;
    io.lcd_cmd_bits = 8;
    io.flags.disable_control_phase = true;
    io.scl_speed_hz = BSP_I2C_FREQ_HZ;
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i2c(i2c_bus, &io, &touch_io), TAG, "Touch IO");
    esp_lcd_touch_config_t cfg = {};
    cfg.x_max = BSP_LCD_H_RES;
    cfg.y_max = BSP_LCD_V_RES;
    cfg.rst_gpio_num = BSP_TOUCH_RST;
    cfg.int_gpio_num = BSP_TOUCH_INT;
    esp_err_t ret = esp_lcd_touch_new_i2c_ft6336(touch_io, &cfg, &touch);
    if (ret != ESP_OK) {
        touch = nullptr;
        esp_lcd_panel_io_del(touch_io);
        touch_io = nullptr;
        return ret;
    }
    // Factory-only double-tap mode ends before normal UI in the old project.
    // LVGL/Brookesia swipe gestures still work with normal coordinate reporting.
    ret = esp_lcd_touch_ft6336_set_gesture_enabled(touch, false);
    if (ret != ESP_OK) {
        esp_lcd_touch_del(touch); touch = nullptr;
        esp_lcd_panel_io_del(touch_io); touch_io = nullptr;
        return ret;
    }
    *ret_touch = touch;
    return ESP_OK;
}

static lv_display_t *bsp_display_lcd_init(const bsp_display_cfg_t *cfg)
{
    bsp_display_config_t panel_cfg = {};
    panel_cfg.max_transfer_sz = (cfg->trans_size ? cfg->trans_size : cfg->buffer_size) * sizeof(uint16_t);
    if (bsp_display_new(&panel_cfg, &panel, &lcd_io) != ESP_OK) return nullptr;
    lvgl_port_display_cfg_t disp_cfg = {};
    disp_cfg.io_handle = lcd_io;
    disp_cfg.panel_handle = panel;
    disp_cfg.buffer_size = cfg->buffer_size;
    disp_cfg.trans_size = cfg->trans_size;
    disp_cfg.double_buffer = cfg->double_buffer;
    disp_cfg.hres = BSP_LCD_H_RES;
    disp_cfg.vres = BSP_LCD_V_RES;
    disp_cfg.color_format = LV_COLOR_FORMAT_RGB565;
    disp_cfg.rotation.mirror_x = true;
    disp_cfg.flags.buff_dma = cfg->flags.buff_dma;
    disp_cfg.flags.buff_spiram = cfg->flags.buff_spiram;
    disp_cfg.flags.swap_bytes = true;
    return lvgl_port_add_disp(&disp_cfg);
}

static lv_indev_t *bsp_display_indev_init(lv_display_t *disp)
{
    bsp_board_status.touch = bsp_touch_new(nullptr, &touch);
    if (bsp_board_status.touch != ESP_OK) return nullptr;
    lvgl_port_touch_cfg_t touch_cfg = {};
    touch_cfg.disp = disp;
    touch_cfg.handle = touch;
    return lvgl_port_add_touch(&touch_cfg);
}

lv_display_t *bsp_display_start(void)
{
    bsp_display_cfg_t cfg = {};
    cfg.lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    cfg.lvgl_port_cfg.task_stack = 16 * 1024;
    cfg.lvgl_port_cfg.task_max_sleep_ms = 20;
    cfg.buffer_size = BSP_LCD_H_RES * CONFIG_BSP_LCD_BUFFER_HEIGHT;
    cfg.double_buffer = true;
    cfg.flags.buff_dma = true;
    return bsp_display_start_with_config(&cfg);
}

lv_display_t *bsp_display_start_with_config(const bsp_display_cfg_t *cfg)
{
    if (display_started) return display;
    if (!cfg || !cfg->buffer_size) return nullptr;
    bsp_board_status.i2c = bsp_i2c_init();
    if (bsp_board_status.i2c != ESP_OK) return nullptr;
    bsp_board_status.expander = bsp_io_expander_init();
    if (bsp_board_status.expander != ESP_OK) return nullptr;
    bsp_board_status.power = bsp_power_init();
    if (bsp_board_status.power != ESP_OK) return nullptr;
    bsp_board_status.display = lvgl_port_init(&cfg->lvgl_port_cfg);
    if (bsp_board_status.display != ESP_OK) return nullptr;
    port_initialized = true;
    display = bsp_display_lcd_init(cfg);
    if (!display) { bsp_board_status.display = ESP_FAIL; goto failed; }
    {
        // Register display/touch while the port task is excluded.
        if (!lvgl_port_lock(0)) { bsp_board_status.display = ESP_FAIL; goto failed; }
        input = bsp_display_indev_init(display);
        if (!input) {
            if (bsp_board_status.touch == ESP_OK) bsp_board_status.touch = ESP_ERR_NO_MEM;
            ESP_LOGW(TAG, "Touch: %s", esp_err_to_name(bsp_board_status.touch));
            if (touch) { esp_lcd_touch_del(touch); touch = nullptr; }
            if (touch_io) { esp_lcd_panel_io_del(touch_io); touch_io = nullptr; }
        }
        lvgl_port_unlock();
    }
    bsp_board_status.display = esp_lcd_panel_disp_on_off(panel, true);
    if (bsp_board_status.display != ESP_OK) goto failed;
    bsp_board_status.display = bsp_display_backlight_on();
    if (bsp_board_status.display != ESP_OK) goto failed;
    display_started = true;
    ESP_LOGI(TAG, "ST7796 %dx%d RGB565; LVGL %d.%d.%d, esp_lvgl_port, 2 DMA buffers",
             BSP_LCD_H_RES, BSP_LCD_V_RES, LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR, LVGL_VERSION_PATCH);
    return display;
failed:
    ESP_LOGE(TAG, "Display startup failed: %s", esp_err_to_name(bsp_board_status.display));
    if (input) { lvgl_port_remove_touch(input); input = nullptr; }
    if (display) { lvgl_port_remove_disp(display); display = nullptr; }
    if (port_initialized) { lvgl_port_deinit(); port_initialized = false; }
    if (touch) { esp_lcd_touch_del(touch); touch = nullptr; }
    if (touch_io) { esp_lcd_panel_io_del(touch_io); touch_io = nullptr; }
    if (panel) { esp_lcd_panel_del(panel); panel = nullptr; }
    if (lcd_io) { esp_lcd_panel_io_del(lcd_io); lcd_io = nullptr; }
    spi_bus_free(BSP_LCD_SPI_HOST);
    return nullptr;
}

lv_display_t *bsp_display_get(void) { return display; }
lv_indev_t *bsp_display_get_input_dev(void) { return input; }
esp_lcd_touch_handle_t bsp_touch_get_handle(void) { return touch; }
void bsp_display_rotate(lv_display_t *disp, lv_disp_rotation_t rotation)
{
    lv_display_set_rotation(disp, rotation);
}
bool bsp_display_lock(uint32_t timeout_ms) { return lvgl_port_lock(timeout_ms); }
void bsp_display_unlock(void) { lvgl_port_unlock(); }
