#include "bsp/esp-bsp.h"

#include <atomic>
#include <cstring>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "driver/sdmmc_host.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "nvs_flash.h"
#include "esp_lcd_st7796.h"
#include "esp_lcd_touch_ft6336.h"
#include "img_converters.h"
#include "esp_io_expander_tca9554.h"
#include "esp_lv_adapter.h"
#include "iot_button.h"
#include "button_gpio.h"
#include "XPowersAXP2101.tpp"
#include "SensorPCF85063.hpp"
#include "SensorQMI8658.hpp"

static const char *TAG = "bsp_3_5";
static i2c_master_bus_handle_t i2c_bus;
static i2c_master_dev_handle_t pmu_dev, rtc_dev, imu_dev;
static esp_io_expander_handle_t expander;
static esp_lcd_panel_io_handle_t lcd_io, touch_io;
static esp_lcd_panel_handle_t panel;
static esp_lcd_touch_handle_t touch;
static lv_display_t *display;
static lv_indev_t *input;
static SemaphoreHandle_t sensor_mutex, sd_mutex, camera_mutex;
static StaticSemaphore_t camera_mutex_storage;
static XPowersAXP2101 power;
static SensorPCF85063 rtc;
static SensorQMI8658 imu;
static bool power_ready, rtc_ready, imu_ready, wifi_ready;
static std::atomic<bool> camera_ready{false};
static bool backlight_ready, display_started, expander_ready;
static int brightness;
static esp_timer_handle_t power_timer;
static button_handle_t boot_button;
static std::atomic<uint32_t> button_presses{0};
static sdmmc_card_t *sdcard;
static unsigned sd_leases;
static esp_err_t register_error;
static bsp_init_status_t status = {
    ESP_ERR_INVALID_STATE, ESP_ERR_INVALID_STATE, ESP_ERR_INVALID_STATE,
    ESP_ERR_INVALID_STATE, ESP_ERR_INVALID_STATE, ESP_ERR_INVALID_STATE,
    ESP_ERR_INVALID_STATE, ESP_ERR_INVALID_STATE, ESP_ERR_INVALID_STATE,
    ESP_ERR_INVALID_STATE, ESP_ERR_INVALID_STATE, ESP_ERR_INVALID_STATE
};

class SensorLock {
public:
    SensorLock() { xSemaphoreTakeRecursive(sensor_mutex, portMAX_DELAY); }
    ~SensorLock() { xSemaphoreGiveRecursive(sensor_mutex); }
};

esp_err_t bsp_i2c_init(void)
{
    if (i2c_bus) return ESP_OK;
    if (!sensor_mutex) sensor_mutex = xSemaphoreCreateRecursiveMutex();
    if (!sd_mutex) sd_mutex = xSemaphoreCreateMutex();
    // Create the camera gate before any APP starts, including when the initial
    // camera probe fails. Init retries and captures share this same gate.
    if (!camera_mutex) camera_mutex = xSemaphoreCreateMutexStatic(&camera_mutex_storage);
    if (!sensor_mutex || !sd_mutex || !camera_mutex) return ESP_ERR_NO_MEM;
    i2c_master_bus_config_t cfg = {};
    cfg.i2c_port = BSP_I2C_NUM;
    cfg.sda_io_num = BSP_I2C_SDA;
    cfg.scl_io_num = BSP_I2C_SCL;
    cfg.clk_source = I2C_CLK_SRC_DEFAULT;
    cfg.glitch_ignore_cnt = 7;
    cfg.flags.enable_internal_pullup = true;
    return i2c_new_master_bus(&cfg, &i2c_bus);
}

i2c_master_bus_handle_t bsp_i2c_get_handle(void) { return i2c_bus; }
const bsp_init_status_t *bsp_get_init_status(void) { return &status; }

static esp_err_t add_i2c_device(uint8_t addr, i2c_master_dev_handle_t *dev)
{
    ESP_RETURN_ON_ERROR(bsp_i2c_init(), TAG, "I2C bus");
    if (*dev) return ESP_OK;
    ESP_RETURN_ON_ERROR(i2c_master_probe(i2c_bus, addr, 100), TAG, "I2C device 0x%02x", addr);
    i2c_device_config_t cfg = {};
    cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    cfg.device_address = addr;
    cfg.scl_speed_hz = BSP_I2C_FREQ_HZ;
    return i2c_master_bus_add_device(i2c_bus, &cfg, dev);
}

/* XPowers/sensorlib callbacks use finite timeouts and retain the first error.
 * Public sensor calls hold sensor_mutex across multi-register operations. */
static i2c_master_dev_handle_t register_device(uint8_t addr)
{
    if (addr == AXP2101_SLAVE_ADDRESS) return pmu_dev;
    if (addr == PCF85063_SLAVE_ADDRESS) return rtc_dev;
    if (addr == QMI8658_L_SLAVE_ADDRESS) return imu_dev;
    return nullptr;
}

static int register_read(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t len)
{
    auto dev = register_device(addr);
    esp_err_t ret = dev ? i2c_master_transmit_receive(dev, &reg, 1, data, len, 100) : ESP_ERR_INVALID_STATE;
    if (ret != ESP_OK && register_error == ESP_OK) register_error = ret;
    return ret == ESP_OK ? 0 : -1;
}

static int register_write(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t len)
{
    uint8_t buffer[256];
    buffer[0] = reg;
    if (len) memcpy(buffer + 1, data, len);
    auto dev = register_device(addr);
    esp_err_t ret = dev ? i2c_master_transmit(dev, buffer, len + 1, 100) : ESP_ERR_INVALID_STATE;
    if (ret != ESP_OK && register_error == ESP_OK) register_error = ret;
    return ret == ESP_OK ? 0 : -1;
}

esp_err_t bsp_io_expander_init(void)
{
    if (expander_ready) return ESP_OK;
    ESP_RETURN_ON_ERROR(bsp_i2c_init(), TAG, "I2C bus");
    if (!expander) {
        ESP_RETURN_ON_ERROR(esp_io_expander_new_i2c_tca9554(i2c_bus,
                            ESP_IO_EXPANDER_I2C_TCA9554_ADDRESS_000, &expander), TAG, "TCA9554");
    }
    ESP_RETURN_ON_ERROR(esp_io_expander_set_dir(expander, IO_EXPANDER_PIN_NUM_1 | IO_EXPANDER_PIN_NUM_7,
                                              IO_EXPANDER_OUTPUT), TAG, "EXIO outputs");
    ESP_RETURN_ON_ERROR(esp_io_expander_set_dir(expander, IO_EXPANDER_PIN_NUM_6,
                                              IO_EXPANDER_INPUT), TAG, "EXIO input");
    ESP_RETURN_ON_ERROR(esp_io_expander_set_level(expander, IO_EXPANDER_PIN_NUM_1, 0), TAG, "EXIO low");
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_RETURN_ON_ERROR(esp_io_expander_set_level(expander, IO_EXPANDER_PIN_NUM_1 | IO_EXPANDER_PIN_NUM_7, 1),
                        TAG, "EXIO high");
    vTaskDelay(pdMS_TO_TICKS(100));
    expander_ready = true;
    return ESP_OK;
}

esp_err_t bsp_io_expander_read_input(bool *high)
{
    if (!high) return ESP_ERR_INVALID_ARG;
    if (!expander_ready) return ESP_ERR_INVALID_STATE;
    uint32_t level = 0;
    esp_err_t ret = esp_io_expander_get_level(expander, IO_EXPANDER_PIN_NUM_6, &level);
    if (ret == ESP_OK) *high = (level & IO_EXPANDER_PIN_NUM_6) != 0;
    return ret;
}

// Read the actual PMIC registers; do not report a requested mode as verified.
// Caller holds sensor_mutex, including across the PWM read/modify/write.
static esp_err_t power_log_dcdc(const char *stage, uint8_t *pwm_value = nullptr)
{
    const int outputs = power.readRegister(XPOWERS_AXP2101_DC_ONOFF_DVM_CTRL);
    const int pwm = power.readRegister(XPOWERS_AXP2101_DC_FORCE_PWM_CTRL);
    if (outputs < 0 || pwm < 0) {
        const esp_err_t error = register_error == ESP_OK ? ESP_FAIL : register_error;
        ESP_LOGE(TAG, "[%s] AXP2101 DCDC register read failed: %s", stage, esp_err_to_name(error));
        return error;
    }
    ESP_LOGI(TAG, "[%s] DCDC registers: 0x80=0x%02X, 0x81=0x%02X", stage, outputs, pwm);
    ESP_LOGI(TAG, "[%s] DCDC1 forced PWM: %s (0x81 bit2)", stage, (pwm & (1U << 2)) ? "ON" : "OFF");
    if (pwm_value) *pwm_value = static_cast<uint8_t>(pwm);
    return ESP_OK;
}

static void power_watchdog(void *)
{
    // Do not block the esp_timer task behind an APP's sensor read.
    if (xSemaphoreTakeRecursive(sensor_mutex, 0) == pdTRUE) {
        power.clrWatchdog();
        xSemaphoreGiveRecursive(sensor_mutex);
    }
}

esp_err_t bsp_power_init(void)
{
    if (power_ready) return ESP_OK;
    ESP_RETURN_ON_ERROR(add_i2c_device(AXP2101_SLAVE_ADDRESS, &pmu_dev), TAG, "PMU");
    SensorLock lock;
    register_error = ESP_OK;
    if (!power.begin(AXP2101_SLAVE_ADDRESS, register_read, register_write)) {
        return register_error == ESP_OK ? ESP_ERR_NOT_FOUND : register_error;
    }
    ESP_RETURN_ON_ERROR(power_log_dcdc("before config"), TAG, "PMU initial mode read");
    // Preserve the active initializer in the 260911 source, not its unused init1().
    power.setVbusVoltageLimit(XPOWERS_AXP2101_VBUS_VOL_LIM_4V36);
    power.setVbusCurrentLimit(XPOWERS_AXP2101_VBUS_CUR_LIM_1500MA);
    power.setSysPowerDownVoltage(2600);
    power.setDC1Voltage(3300);
    power.setDC2Voltage(1000);
    power.setDC3Voltage(3300);
    power.setDC4Voltage(1000);
    power.setDC5Voltage(3300);
    power.setALDO1Voltage(3300);
    power.setALDO2Voltage(3300);
    power.setALDO3Voltage(3300);
    power.setALDO4Voltage(3300);
    power.setBLDO1Voltage(1500);
    power.setBLDO2Voltage(2800);
    power.setCPUSLDOVoltage(1000);
    power.setDLDO1Voltage(3300);
    // Old setDLDO2Voltage(3300) was rejected by XPowers (max 1400 mV).
    // Preserve the existing DLDO2 voltage instead of inventing a new setting.
    power.enableDC2(); power.enableDC3(); power.enableDC4(); power.enableDC5();
    power.enableALDO1(); power.enableALDO2(); power.enableALDO3(); power.enableALDO4();
    power.enableBLDO1(); power.enableBLDO2(); power.enableCPUSLDO();
    power.enableDLDO1(); power.enableDLDO2();
    power.setPowerKeyPressOffTime(XPOWERS_POWEROFF_4S);
    power.setPowerKeyPressOnTime(XPOWERS_POWERON_128MS);
    power.disableTSPinMeasure();
    power.enableBattDetection();
    power.enableVbusVoltageMeasure();
    power.enableBattVoltageMeasure();
    power.enableSystemVoltageMeasure();
    power.setChargingLedMode(XPOWERS_CHG_LED_CTRL_CHG);
    power.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
    power.clearIrqStatus();
    power.enableIRQ(XPOWERS_AXP2101_BAT_INSERT_IRQ | XPOWERS_AXP2101_BAT_REMOVE_IRQ |
                    XPOWERS_AXP2101_VBUS_INSERT_IRQ | XPOWERS_AXP2101_VBUS_REMOVE_IRQ |
                    XPOWERS_AXP2101_PKEY_SHORT_IRQ | XPOWERS_AXP2101_PKEY_LONG_IRQ |
                    XPOWERS_AXP2101_BAT_CHG_DONE_IRQ | XPOWERS_AXP2101_BAT_CHG_START_IRQ);
    power.setPrechargeCurr(XPOWERS_AXP2101_PRECHARGE_50MA);
    power.setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_200MA);
    power.setChargerTerminationCurr(XPOWERS_AXP2101_CHG_ITERM_25MA);
    power.setChargeTargetVoltage(XPOWERS_AXP2101_CHG_VOL_4V1);
    power.setWatchdogConfig(XPOWERS_AXP2101_WDT_IRQ_TO_PIN);
    power.setWatchdogTimeout(XPOWERS_AXP2101_WDT_TIMEOUT_4S);
    power.enableButtonBatteryCharge();
    power.setButtonBatteryChargeVoltage(3300);
    ESP_RETURN_ON_ERROR(register_error, TAG, "PMU configuration");

    // DCDC1 supplies 3.3 V: force PWM to address the validated low-brightness
    // flicker issue. XPowers changes only 0x81 bit2, preserving the other bits.
    power.settDC1WorkModeToPwm(true);
    ESP_RETURN_ON_ERROR(register_error, TAG, "DCDC1 forced PWM write");
    uint8_t pwm_after = 0;
    ESP_RETURN_ON_ERROR(power_log_dcdc("after config", &pwm_after), TAG, "PMU mode readback");
    if (!(pwm_after & (1U << 2))) {
        ESP_LOGE(TAG, "DCDC1 forced PWM verification failed: 0x81=0x%02X", pwm_after);
        return ESP_ERR_INVALID_RESPONSE;
    }
    ESP_LOGI(TAG, "DCDC1 forced PWM verified");

    if (!power_timer) {
        esp_timer_create_args_t timer = {};
        timer.callback = power_watchdog;
        timer.name = "pmu_wdt";
        ESP_RETURN_ON_ERROR(esp_timer_create(&timer, &power_timer), TAG, "PMU timer");
    }
    ESP_RETURN_ON_ERROR(esp_timer_start_periodic(power_timer, 2000000), TAG, "PMU timer start");
    power.enableWatchdog();
    if (register_error != ESP_OK) {
        esp_timer_stop(power_timer);
        return register_error;
    }
    power_ready = true;
    ESP_LOGI(TAG, "AXP2101 ready; DLDO2 voltage retained at %u mV", power.getDLDO2Voltage());
    return ESP_OK;
}

esp_err_t bsp_battery_get_info(bsp_battery_info_t *info)
{
    if (!info) return ESP_ERR_INVALID_ARG;
    if (!power_ready) return ESP_ERR_INVALID_STATE;
    SensorLock lock;
    register_error = ESP_OK;
    bsp_battery_info_t result = {};
    result.connected = power.isBatteryConnect();
    result.charging = power.isCharging();
    result.vbus_present = power.isVbusIn();
    result.voltage_mv = power.getBattVoltage();
    result.percentage = result.connected ? power.getBatteryPercent() : -1;
    if (register_error == ESP_OK) *info = result;
    return register_error;
}

static esp_err_t backlight_init(void)
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

static esp_err_t lcd_init(void)
{
    ESP_RETURN_ON_ERROR(backlight_init(), TAG, "Backlight");
    spi_bus_config_t bus = {};
    bus.sclk_io_num = BSP_LCD_SCLK;
    bus.mosi_io_num = BSP_LCD_MOSI;
    bus.miso_io_num = -1;
    bus.quadwp_io_num = -1;
    bus.quadhd_io_num = -1;
    bus.max_transfer_sz = BSP_LCD_H_RES * CONFIG_BSP_LCD_BUFFER_HEIGHT * 2;
    ESP_RETURN_ON_ERROR(spi_bus_initialize(BSP_LCD_SPI_HOST, &bus, SPI_DMA_CH_AUTO), TAG, "LCD SPI");
    esp_lcd_panel_io_spi_config_t io = {};
    io.cs_gpio_num = BSP_LCD_CS;
    io.dc_gpio_num = BSP_LCD_DC;
    io.spi_mode = 0;
    io.pclk_hz = CONFIG_BSP_LCD_SPI_CLOCK_HZ;
    io.trans_queue_depth = 10;
    io.lcd_cmd_bits = 8;
    io.lcd_param_bits = 8;
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)BSP_LCD_SPI_HOST, &io, &lcd_io), TAG, "LCD IO");
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
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_st7796(lcd_io, &cfg, &panel), TAG, "ST7796 create");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(panel), TAG, "LCD reset");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(panel), TAG, "LCD init");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_invert_color(panel, true), TAG, "LCD inversion");
    // Native portrait orientation from the original rotation=0 configuration.
    ESP_RETURN_ON_ERROR(esp_lcd_panel_swap_xy(panel, false), TAG, "LCD axes");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_mirror(panel, true, false), TAG, "LCD mirror");
    return esp_lcd_panel_disp_on_off(panel, true);
}

static esp_err_t touch_init(void)
{
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
    if (ret != ESP_OK) { touch = nullptr; return ret; }
    // Factory-only double-tap mode ends before normal UI in the old project.
    // LVGL/Brookesia swipe gestures still work with normal coordinate reporting.
    ESP_RETURN_ON_ERROR(esp_lcd_touch_ft6336_set_gesture_enabled(touch, false), TAG, "Touch coordinate mode");
    esp_lv_adapter_touch_config_t touch_cfg = ESP_LV_ADAPTER_TOUCH_DEFAULT_CONFIG(display, touch);
    input = esp_lv_adapter_register_touch(&touch_cfg);
    return input ? ESP_OK : ESP_ERR_NO_MEM;
}

lv_display_t *bsp_display_start(void)
{
    if (display_started) return display;
    // Board startup is single-threaded; APPs use display_lock after this returns.
    status.i2c = bsp_i2c_init();
    if (status.i2c != ESP_OK) return nullptr;
    status.expander = bsp_io_expander_init();
    if (status.expander != ESP_OK) return nullptr;
    status.power = bsp_power_init();
    if (status.power != ESP_OK) return nullptr;
    status.display = lcd_init();
    if (status.display != ESP_OK) goto failed;
    {
        esp_lv_adapter_config_t adapter = ESP_LV_ADAPTER_DEFAULT_CONFIG();
        // Xtensa GUI callbacks and APP/snapshot transitions need their own stack
        // budget; free PSRAM does not enlarge the adapter's default 8 KB stack.
        adapter.task_stack_size = 16 * 1024;
        status.display = esp_lv_adapter_init(&adapter);
        if (status.display != ESP_OK) goto failed;
        esp_lv_adapter_display_config_t cfg = {};
        cfg.panel = panel;
        cfg.panel_io = lcd_io;
        cfg.profile.interface = ESP_LV_ADAPTER_PANEL_IF_OTHER;
        cfg.profile.rotation = ESP_LV_ADAPTER_ROTATE_0;
        cfg.profile.hor_res = BSP_LCD_H_RES;
        cfg.profile.ver_res = BSP_LCD_V_RES;
        cfg.profile.buffer_height = CONFIG_BSP_LCD_BUFFER_HEIGHT;
        cfg.profile.use_psram = false;
        cfg.profile.require_double_buffer = true;
        cfg.tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_NONE;
        cfg.te_sync.gpio_num = -1;
        display = esp_lv_adapter_register_display(&cfg);
        if (!display) { status.display = ESP_ERR_NO_MEM; goto failed; }
        status.touch = touch_init();
        if (status.touch != ESP_OK) {
            ESP_LOGW(TAG, "Touch: %s", esp_err_to_name(status.touch));
            if (touch) { esp_lcd_touch_del(touch); touch = nullptr; }
            if (touch_io) { esp_lcd_panel_io_del(touch_io); touch_io = nullptr; }
        }
        status.display = esp_lv_adapter_start();
        if (status.display != ESP_OK) goto failed;
        status.display = bsp_display_brightness_set(CONFIG_BSP_LCD_BRIGHTNESS);
        if (status.display != ESP_OK) goto failed;
    }
    display_started = true;
    ESP_LOGI(TAG, "ST7796 %dx%d RGB565; LVGL %d.%d.%d, adapter porting, 2 DMA buffers",
             BSP_LCD_H_RES, BSP_LCD_V_RES, LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR, LVGL_VERSION_PATCH);
    return display;

failed:
    ESP_LOGE(TAG, "Display startup failed: %s", esp_err_to_name(status.display));
    if (esp_lv_adapter_is_initialized()) esp_lv_adapter_deinit();
    input = nullptr;
    display = nullptr;
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
bool bsp_display_lock(uint32_t timeout_ms)
{
    return esp_lv_adapter_lock(timeout_ms ? (int32_t)timeout_ms : -1) == ESP_OK;
}
void bsp_display_unlock(void) { esp_lv_adapter_unlock(); }

static void button_pressed(void *, void *) { button_presses.fetch_add(1); }
esp_err_t bsp_user_button_init(void)
{
    if (boot_button) return ESP_OK;
    button_config_t cfg = {};
    button_gpio_config_t gpio = {};
    gpio.gpio_num = BSP_BUTTON_BOOT;
    gpio.active_level = 0;
    ESP_RETURN_ON_ERROR(iot_button_new_gpio_device(&cfg, &gpio, &boot_button), TAG, "BOOT button");
    esp_err_t ret = iot_button_register_cb(boot_button, BUTTON_PRESS_DOWN, nullptr, button_pressed, nullptr);
    if (ret != ESP_OK) { iot_button_delete(boot_button); boot_button = nullptr; }
    return ret;
}
uint32_t bsp_user_button_get_press_count(void) { return button_presses.load(); }

static bool valid_datetime(const bsp_sensor_datetime_t &d)
{
    if (d.year < 2000 || d.year > 2099 || d.month < 1 || d.month > 12 ||
        d.day < 1 || d.hour > 23 || d.minute > 59 || d.second > 59) return false;
    const uint8_t days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    return d.day <= days[d.month - 1] + (d.month == 2 && d.year % 4 == 0);
}

esp_err_t bsp_rtc_get_datetime(bsp_sensor_datetime_t *datetime)
{
    if (!datetime) return ESP_ERR_INVALID_ARG;
    if (!rtc_ready) return ESP_ERR_INVALID_STATE;
    SensorLock lock;
    uint8_t raw[7] = {};
    const uint8_t reg = PCF85063_SEC_REG;
    ESP_RETURN_ON_ERROR(i2c_master_transmit_receive(rtc_dev, &reg, 1, raw, sizeof(raw), 100), TAG, "RTC read");
    if (raw[0] & 0x80) return ESP_ERR_INVALID_STATE; // Oscillator-stop flag.
    raw[0] &= 0x7f; raw[1] &= 0x7f; raw[2] &= 0x3f;
    raw[3] &= 0x3f; raw[4] &= 7; raw[5] &= 0x1f;
    for (uint8_t byte : raw) {
        if ((byte & 15) > 9 || (byte >> 4) > 9) return ESP_ERR_INVALID_STATE;
    }
    auto dec = [](uint8_t v) -> uint8_t { return (v >> 4) * 10 + (v & 15); };
    bsp_sensor_datetime_t result = { (uint16_t)(2000 + dec(raw[6])), dec(raw[5]), dec(raw[3]),
                                    dec(raw[2]), dec(raw[1]), dec(raw[0]), dec(raw[4]) };
    if (!valid_datetime(result) || result.weekday > 6) return ESP_ERR_INVALID_STATE;
    *datetime = result;
    return ESP_OK;
}

esp_err_t bsp_rtc_set_datetime(const bsp_sensor_datetime_t *datetime)
{
    if (!datetime || !valid_datetime(*datetime)) return ESP_ERR_INVALID_ARG;
    if (!rtc_ready) return ESP_ERR_INVALID_STATE;
    SensorLock lock;
    register_error = ESP_OK;
    rtc.setDateTime(datetime->year, datetime->month, datetime->day,
                    datetime->hour, datetime->minute, datetime->second);
    return register_error;
}

esp_err_t bsp_rtc_init(void)
{
    if (rtc_ready) return ESP_OK;
    ESP_RETURN_ON_ERROR(add_i2c_device(PCF85063_SLAVE_ADDRESS, &rtc_dev), TAG, "RTC");
    SensorLock lock;
    register_error = ESP_OK;
    if (!rtc.begin(PCF85063_SLAVE_ADDRESS, register_read, register_write)) {
        return register_error == ESP_OK ? ESP_ERR_INVALID_STATE : register_error;
    }
    ESP_RETURN_ON_ERROR(register_error, TAG, "RTC init");
    rtc_ready = true;
    bsp_sensor_datetime_t date = {};
    esp_err_t ret = bsp_rtc_get_datetime(&date);
    if (ret == ESP_ERR_INVALID_STATE || (ret == ESP_OK && date.year < 2025)) {
        const bsp_sensor_datetime_t fallback = {2025, 1, 1, 12, 0, 0, 3};
        ret = bsp_rtc_set_datetime(&fallback);
        ESP_LOGW(TAG, "RTC invalid/old calendar; using original fallback 2025-01-01 12:00:00");
    }
    if (ret != ESP_OK) rtc_ready = false;
    return ret;
}

esp_err_t bsp_imu_init(void)
{
    if (imu_ready) return ESP_OK;
    ESP_RETURN_ON_ERROR(add_i2c_device(QMI8658_L_SLAVE_ADDRESS, &imu_dev), TAG, "QMI8658 (0x6b)");
    SensorLock lock;
    register_error = ESP_OK;
    if (!imu.begin(QMI8658_L_SLAVE_ADDRESS, register_read, register_write)) {
        return register_error == ESP_OK ? ESP_ERR_NOT_FOUND : register_error;
    }
    imu.configAccelerometer(SensorQMI8658::ACC_RANGE_4G, SensorQMI8658::ACC_ODR_1000Hz, SensorQMI8658::LPF_MODE_0);
    imu.configGyroscope(SensorQMI8658::GYR_RANGE_64DPS, SensorQMI8658::GYR_ODR_896_8Hz, SensorQMI8658::LPF_MODE_3);
    bool gyro_ok = imu.enableGyroscope();
    bool accel_ok = imu.enableAccelerometer();
    ESP_RETURN_ON_ERROR(register_error, TAG, "QMI configuration");
    if (!gyro_ok || !accel_ok) return ESP_FAIL;
    imu_ready = true;
    return ESP_OK;
}

esp_err_t bsp_imu_get_data(bsp_imu_data_t *data)
{
    if (!data) return ESP_ERR_INVALID_ARG;
    if (!imu_ready) return ESP_ERR_INVALID_STATE;
    SensorLock lock;
    register_error = ESP_OK;
    if (!imu.getDataReady()) return register_error == ESP_OK ? ESP_ERR_NOT_FINISHED : register_error;
    bsp_imu_data_t result = {};
    bool accel_ok = imu.getAccelerometer(result.accel_x, result.accel_y, result.accel_z);
    bool gyro_ok = imu.getGyroscope(result.gyro_x, result.gyro_y, result.gyro_z);
    result.temperature_c = imu.getTemperature_C();
    if (register_error != ESP_OK) return register_error;
    if (!accel_ok || !gyro_ok) return ESP_FAIL;
    *data = result;
    return ESP_OK;
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
    status.sdcard = ret;
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
        if (ret == ESP_OK) { sdcard = nullptr; status.sdcard = ESP_ERR_INVALID_STATE; }
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

static esp_err_t camera_init_locked(void)
{
    camera_config_t cfg = {};
    cfg.ledc_channel = LEDC_CHANNEL_0;
    cfg.ledc_timer = LEDC_TIMER_0;
    cfg.pin_d0 = BSP_CAMERA_D0; cfg.pin_d1 = BSP_CAMERA_D1;
    cfg.pin_d2 = BSP_CAMERA_D2; cfg.pin_d3 = BSP_CAMERA_D3;
    cfg.pin_d4 = BSP_CAMERA_D4; cfg.pin_d5 = BSP_CAMERA_D5;
    cfg.pin_d6 = BSP_CAMERA_D6; cfg.pin_d7 = BSP_CAMERA_D7;
    cfg.pin_xclk = BSP_CAMERA_XCLK;
    cfg.pin_pclk = BSP_CAMERA_PCLK;
    cfg.pin_vsync = BSP_CAMERA_VSYNC;
    cfg.pin_href = BSP_CAMERA_HREF;
    cfg.pin_sccb_sda = -1;
    cfg.pin_sccb_scl = -1;
    cfg.sccb_i2c_port = BSP_I2C_NUM;
    cfg.pin_pwdn = -1;
    cfg.pin_reset = -1;
    cfg.xclk_freq_hz = 20000000;
    cfg.frame_size = FRAMESIZE_320X480;
    cfg.pixel_format = PIXFORMAT_RGB565;
    cfg.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    cfg.fb_location = CAMERA_FB_IN_PSRAM;
    cfg.jpeg_quality = 12;
    cfg.fb_count = 1;
    const esp_err_t ret = esp_camera_init(&cfg);
    if (ret != ESP_OK) {
        // esp32-camera cleans up its own partial initialization on failure.
        ESP_LOGE(TAG, "Camera init: driver failed: %s (0x%x), internal free=%u, largest DMA=%u",
                 esp_err_to_name(ret), static_cast<unsigned>(ret),
                 static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_INTERNAL)),
                 static_cast<unsigned>(heap_caps_get_largest_free_block(MALLOC_CAP_DMA)));
        return ret;
    }
    sensor_t *sensor = esp_camera_sensor_get();
    if (!sensor || !sensor->set_vflip) {
        ESP_LOGE(TAG, "Camera init: sensor or vertical-flip callback is missing");
        esp_camera_deinit();
        return ESP_ERR_NOT_SUPPORTED;
    }
    if (sensor->set_vflip(sensor, 1) != 0) {
        ESP_LOGE(TAG, "Camera init: setting vertical flip failed, PID=0x%04x", sensor->id.PID);
        esp_camera_deinit();
        return ESP_FAIL;
    }
    camera_ready.store(true);
    ESP_LOGI(TAG, "Camera init: ready, PID=0x%04x, RGB565 320x480, XCLK=20 MHz", sensor->id.PID);
    return ESP_OK;
}

esp_err_t bsp_camera_init(void)
{
    if (camera_ready.load()) return ESP_OK;
    if (!power_ready || !expander_ready || !camera_mutex) {
        ESP_LOGE(TAG, "Camera init: prerequisites missing: power=%d, expander=%d, mutex=%d",
                 power_ready, expander_ready, camera_mutex != nullptr);
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(camera_mutex, pdMS_TO_TICKS(1500)) != pdTRUE) {
        ESP_LOGW(TAG, "Camera init: another camera operation is in progress");
        return ESP_ERR_NOT_FINISHED;
    }
    esp_err_t ret = ESP_OK;
    if (!camera_ready.load()) {
        for (unsigned attempt = 1; attempt <= 2; ++attempt) {
            ESP_LOGI(TAG, "Camera init: attempt %u/2, previous=%s", attempt, esp_err_to_name(status.camera));
            ret = camera_init_locked();
            status.camera = ret;
            if (ret == ESP_OK || attempt == 2 ||
                    (ret != ESP_ERR_NOT_FOUND && ret != ESP_ERR_TIMEOUT && ret != ESP_FAIL)) break;
            // Retry only transient probe/register failures after the driver's
            // cleanup. Do not reset shared PMIC/expander outputs or the I2C bus.
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
    xSemaphoreGive(camera_mutex);
    return ret;
}
camera_fb_t *bsp_camera_get_frame(void) { return camera_ready ? esp_camera_fb_get() : nullptr; }
void bsp_camera_return_frame(camera_fb_t *frame) { if (camera_ready && frame) esp_camera_fb_return(frame); }

bool bsp_camera_acquire(uint32_t timeout_ms)
{
    return camera_ready && camera_mutex && xSemaphoreTake(camera_mutex, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}
void bsp_camera_release(void)
{
    if (camera_mutex && xSemaphoreGetMutexHolder(camera_mutex) == xTaskGetCurrentTaskHandle()) xSemaphoreGive(camera_mutex);
}
namespace {
struct CameraJpegBuffer {
    uint8_t *data = nullptr;
    size_t size = 0;
    size_t capacity = 0;
    esp_err_t error = ESP_OK;
};

size_t camera_jpeg_write(void *arg, size_t index, const void *data, size_t len)
{
    auto &out = *static_cast<CameraJpegBuffer *>(arg);
    if (out.error != ESP_OK || !data || !len) return 0;
    constexpr size_t max_size = 512 * 1024;
    if (index != out.size || index > max_size || len > max_size - index) {
        out.error = ESP_ERR_INVALID_SIZE;
        return 0;
    }
    const size_t needed = index + len;
    if (needed > out.capacity) {
        size_t capacity = out.capacity ? out.capacity : 16384;
        while (capacity < needed) capacity *= 2;
        auto *buffer = static_cast<uint8_t *>(heap_caps_realloc(
            out.data, capacity, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
        if (!buffer) {
            out.error = ESP_ERR_NO_MEM;
            return 0;
        }
        out.data = buffer;
        out.capacity = capacity;
    }
    memcpy(out.data + index, data, len);
    out.size = needed;
    return len;
}
}

esp_err_t bsp_camera_capture_jpeg(uint8_t **jpeg, size_t *size)
{
    return bsp_camera_capture_jpeg_with_preview(jpeg, size, nullptr, nullptr);
}

esp_err_t bsp_camera_capture_jpeg_with_preview(uint8_t **jpeg, size_t *size,
                                             bsp_camera_preview_cb_t preview, void *context)
{
    if (!jpeg || !size) return ESP_ERR_INVALID_ARG;
    *jpeg = nullptr; *size = 0;
    const esp_err_t init_result = bsp_camera_init();
    if (init_result != ESP_OK) {
        ESP_LOGE(TAG, "AI capture: camera initialization failed: %s (0x%x)",
                 esp_err_to_name(init_result), static_cast<unsigned>(init_result));
        return init_result;
    }
    const int64_t started = esp_timer_get_time();
    if (!bsp_camera_acquire(1000)) {
        ESP_LOGW(TAG, "AI capture: camera is still owned by another APP");
        return ESP_ERR_NOT_FINISHED;
    }
    // Discard the queued frame so AI sees the current scene, not the last APP view.
    camera_fb_t *stale = bsp_camera_get_frame();
    if (stale) bsp_camera_return_frame(stale);
    esp_err_t ret = ESP_ERR_TIMEOUT;
    camera_fb_t snapshot = {};
    // A brief local preview on the MCP worker, not a second camera task.
    // The last frame is copied once and feeds both the frozen preview and JPEG.
    const unsigned frame_count = preview ? 4 : 1;
    for (unsigned i = 0; i < frame_count; ++i) {
        if (preview && !preview(nullptr, false, context)) { ret = ESP_ERR_INVALID_STATE; break; }
        camera_fb_t *frame = bsp_camera_get_frame();
        if (!frame) { ret = ESP_ERR_TIMEOUT; break; }
        const size_t required = static_cast<size_t>(frame->width) * frame->height * 2;
        if (frame->format != PIXFORMAT_RGB565) {
            ret = ESP_ERR_NOT_SUPPORTED;
        } else if (!frame->buf || !required || frame->len < required) {
            ret = ESP_ERR_INVALID_SIZE;
        } else if (i + 1 < frame_count) {
            ret = preview(frame, false, context) ? ESP_OK : ESP_ERR_INVALID_STATE;
        } else {
            snapshot = *frame;
            snapshot.len = required;
            snapshot.buf = static_cast<uint8_t *>(heap_caps_malloc(
                required, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
            ret = snapshot.buf ? ESP_OK : ESP_ERR_NO_MEM;
            if (snapshot.buf) memcpy(snapshot.buf, frame->buf, required);
        }
        bsp_camera_return_frame(frame);
        if (ret != ESP_OK) break;
        if (i + 1 < frame_count) vTaskDelay(pdMS_TO_TICKS(200));
    }
    // Encoding/network must not hold the shared camera or its single framebuffer.
    bsp_camera_release();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "AI capture: frame copy failed: %s", esp_err_to_name(ret));
        return ret;
    }

    if (preview && !preview(&snapshot, true, context)) {
        heap_caps_free(snapshot.buf);
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "AI capture: %ux%u RGB565, %u bytes copied to PSRAM",
             static_cast<unsigned>(snapshot.width), static_cast<unsigned>(snapshot.height),
             static_cast<unsigned>(snapshot.len));
    CameraJpegBuffer encoded;
    // The library's frame2jpg() uses a fixed 128 KiB buffer and can silently
    // truncate output. Accumulate callback output and check errors explicitly.
    const bool ok = frame2jpg_cb(&snapshot, 80, camera_jpeg_write, &encoded);
    heap_caps_free(snapshot.buf);
    ret = encoded.error;
    if (ret == ESP_OK && (!ok || encoded.size < 4 ||
            encoded.data[0] != 0xff || encoded.data[1] != 0xd8 ||
            encoded.data[encoded.size - 2] != 0xff || encoded.data[encoded.size - 1] != 0xd9)) {
        ret = ESP_FAIL;
    }
    if (ret != ESP_OK) {
        heap_caps_free(encoded.data);
        ESP_LOGE(TAG, "AI capture: JPEG encoding failed: %s", esp_err_to_name(ret));
        return ret;
    }
    *jpeg = encoded.data;
    *size = encoded.size;
    ESP_LOGI(TAG, "AI capture: JPEG ready, %u bytes, %lld ms",
             static_cast<unsigned>(*size), (esp_timer_get_time() - started) / 1000);
    return ESP_OK;
}

esp_err_t bsp_camera_rotate_180(void)
{
    if (!camera_ready || !sensor_mutex) return ESP_ERR_INVALID_STATE;
    SensorLock lock;
    sensor_t *sensor = esp_camera_sensor_get();
    if (!sensor || !sensor->set_hmirror || !sensor->set_vflip) return ESP_ERR_NOT_SUPPORTED;

    // Restore the original factory BOOT action: toggle both axes together.
    const int old_hmirror = sensor->status.hmirror;
    const int old_vflip = sensor->status.vflip;
    const int hmirror = !old_hmirror;
    const int vflip = !old_vflip;
    const int hresult = sensor->set_hmirror(sensor, hmirror);
    const int vresult = sensor->set_vflip(sensor, vflip);
    if (hresult != 0 || vresult != 0) {
        // Avoid leaving only one axis flipped if a register write failed.
        const int hrestore = sensor->set_hmirror(sensor, old_hmirror);
        const int vrestore = sensor->set_vflip(sensor, old_vflip);
        ESP_LOGE(TAG, "Camera rotation failed: hmirror=%d, vflip=%d; restore=%d/%d",
                 hresult, vresult, hrestore, vrestore);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Camera rotated 180 degrees: hmirror=%d, vflip=%d", hmirror, vflip);
    return ESP_OK;
}

esp_err_t bsp_wifi_init(void)
{
    if (wifi_ready) return ESP_OK;
    ESP_RETURN_ON_ERROR(esp_netif_init(), TAG, "Netif");
    esp_err_t ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) return ret;
    static esp_netif_t *sta, *ap;
    if (!sta) sta = esp_netif_create_default_wifi_sta();
    if (!ap) ap = esp_netif_create_default_wifi_ap();
    if (!sta || !ap) return ESP_ERR_NO_MEM;
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&cfg), TAG, "Wi-Fi driver");
    ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret != ESP_OK) { esp_wifi_deinit(); return ret; }
    // Do not connect to the old factory SSID. Provisioning is implemented by APPs later.
    wifi_ready = true;
    return ESP_OK;
}

static void report(const char *name, esp_err_t result)
{
    if (result == ESP_OK) ESP_LOGI(TAG, "%s ready", name);
    else ESP_LOGW(TAG, "%s unavailable: %s", name, esp_err_to_name(result));
}

esp_err_t bsp_wifi_start(void)
{
    if (!wifi_ready) return ESP_ERR_INVALID_STATE;
    // esp_wifi_start is idempotent when the initialized Wi-Fi driver is running.
    return esp_wifi_start();
}

esp_err_t bsp_init(void)
{
    esp_err_t ret = nvs_flash_init();
    // Preserve saved data on NVS incompatibility; report instead of silently erasing.
    ESP_RETURN_ON_ERROR(ret, TAG, "NVS initialization");
    if (!bsp_display_start()) {
        if (status.i2c != ESP_OK) return status.i2c;
        if (status.expander != ESP_OK) return status.expander;
        if (status.power != ESP_OK) return status.power;
        return status.display;
    }
    status.button = bsp_user_button_init(); report("BOOT button", status.button);
    status.audio = bsp_audio_init(); report("ES8311", status.audio);
    status.rtc = bsp_rtc_init(); report("PCF85063", status.rtc);
    status.imu = bsp_imu_init(); report("QMI8658", status.imu);
    status.sdcard = bsp_sdcard_mount(); report("SD card", status.sdcard);
    status.camera = bsp_camera_init(); report("Camera", status.camera);
    status.wifi = bsp_wifi_init(); report("Wi-Fi driver", status.wifi);
    return ESP_OK;
}
