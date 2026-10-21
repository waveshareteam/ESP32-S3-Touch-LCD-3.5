#include <new>
#include <vector>
#include "bsp/esp-bsp.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_brookesia.hpp"
#include "Settings.hpp"
#include "DrawingBoard.hpp"
#include "Photos.hpp"
#include "Weather.hpp"
#include "Music.hpp"
#include "Sensor.hpp"
#include "Games.hpp"
#include "Camera.hpp"
#include "XiaozhiApp.hpp"
#include "MicTest.hpp"
#include "ButtonTest.hpp"
#include "DisplayTest.hpp"
#include "wifi_manager.h"
#include "settings_service.h"
#include "desktop.hpp"

using namespace esp_brookesia;
using namespace esp_brookesia::gui;
using namespace esp_brookesia::systems::phone;
using namespace esp_brookesia::apps;
static const char *TAG = "main";
static Phone *phone;
LV_IMAGE_DECLARE(img_desktop_wallpaper);

// Add future APP headers/dependencies, then install their instances here.
// Each APP owns its run/back/close/init/deinit/pause/resume implementation.
static bool app_start()
{
    LvLock::registerCallbacks([](int timeout_ms) {
        return bsp_display_lock(timeout_ms > 0 ? (uint32_t)timeout_ms : 0);
    }, []() { bsp_display_unlock(); return true; });
    LvLockGuard guard;
    phone = new (std::nothrow) Phone(bsp_display_get());
    if (!phone) return false;
    auto *touch = bsp_display_get_input_dev();
    if (touch && !phone->setTouchDevice(touch)) return false;
    // Use the C5 3.5 layout with a transparent status bar and no bottom indicator.
    static const Stylesheet stylesheet = [] {
        Stylesheet style = STYLESHEET_320_480_DARK;
        style.core.display.background.wallpaper_image_resource = StyleImage::IMAGE(&img_desktop_wallpaper);
        style.display.app_launcher.data.icon.label.text_color = StyleColor::COLOR(0x000000);
        style.display.flags.enable_status_bar = 1;
        auto &status = style.display.status_bar.data;
        status.main.background_color = StyleColor::COLOR_WITH_OPACITY(0x000000, LV_OPA_TRANSP);
        status.main.text_color = StyleColor::COLOR(0x000000);
        status.main.text_font = StyleFont::SIZE(28);
        // Keep the clock centered on the screen, independently of the right icons.
        status.area.data[0].layout_column_align = StatusBar::AreaAlign::CENTER;
        status.area.data[0].layout_column_start_offset = 0;
        status.area.data[1].size = StyleSize::RECT_PERCENT(25, 100);
        for (int i = 0; i < status.battery.icon_data.icon.image_num; ++i) {
            status.battery.icon_data.icon.images[i].recolor = StyleColor::COLOR(0x000000);
        }
        for (int i = 0; i < status.wifi.icon_data.icon.image_num; ++i) {
            status.wifi.icon_data.icon.images[i].recolor = StyleColor::COLOR(0x000000);
        }
        style.manager.gesture.flags.enable_indicator_bars[
            static_cast<int>(Gesture::IndicatorBarType::BOTTOM)] = 0;
        return style;
    }();
    if (!phone->addStylesheet(stylesheet) || !phone->activateStylesheet(stylesheet) || !phone->begin()) return false;

    unsigned installed = 0;
    // Desktop order: Settings, Xiaozhi, existing APPs, then the hardware tests.
    auto *app_settings = Settings::requestInstance();
    int settings_id = app_settings ? phone->installApp(app_settings) : -1;
    if (settings_id >= 0) ++installed;
    else ESP_LOGE(TAG, "Settings installation failed");

    auto *app_xiaozhi = XiaozhiApp::requestInstance();
    int xiaozhi_id = app_xiaozhi ? phone->installApp(app_xiaozhi) : -1;
    if (xiaozhi_id >= 0) ++installed;
    else ESP_LOGE(TAG, "Xiaozhi installation failed; keeping the other APPs available");

    auto *app_drawing = DrawingBoard::requestInstance();
    int drawing_id = app_drawing ? phone->installApp(app_drawing) : -1;
    if (drawing_id >= 0) ++installed;
    else ESP_LOGE(TAG, "Touch Test installation failed");

    struct Entry { const char *name; systems::phone::App *app; };
    const Entry apps[] = {
        {"Photos", Photos::requestInstance()}, {"Weather", Weather::requestInstance()},
        {"Music", Music::requestInstance()}, {"Sensor", Sensor::requestInstance()},
        {"2048", Games::requestInstance()}, {"Camera", Camera::requestInstance()},
        {"Mic Test", MicTest::requestInstance()},
        {"Button Test", ButtonTest::requestInstance()},
        {"Display Test", DisplayTest::requestInstance()},
    };
    for(const auto &entry:apps) {
        int id=entry.app?phone->installApp(entry.app):-1;
        if(id>=0) { ++installed; ESP_LOGI(TAG,"%s installed, ID=%d",entry.name,id); }
        else ESP_LOGE(TAG,"%s installation failed",entry.name);
    }

    // Keep the registry entry used by the reference project for future APP ports.
    std::vector<systems::base::Manager::RegistryAppInfo> registered;
    if (!phone->initAppFromRegistry(registered) || !phone->installAppFromRegistry(registered)) {
        ESP_LOGW(TAG, "Some registry APPs could not be installed");
    }
    if (!desktop_status_start(phone->getDisplay().getStatusBar())) {
        ESP_LOGW(TAG, "Desktop status updates could not start");
    }
    ESP_LOGI(TAG, "C5-style desktop ready: 320x480 portrait, 3 pages, %u/12 APPs, Settings ID=%d, Xiaozhi ID=%d, Touch Test ID=%d", installed, settings_id, xiaozhi_id, drawing_id);
    ESP_LOGI(TAG, "Free heap: internal=%u, PSRAM=%u", (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    return installed > 0;
}

extern "C" void app_main(void)
{
    esp_err_t ret = bsp_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BSP startup failed: %s", esp_err_to_name(ret));
        return;
    }
    // Settings restores saved preferences after hardware is ready, before UI start.
    if (!settings_service_start()) ESP_LOGE(TAG, "Settings worker could not start");
    if (!wifi_manager_init()) ESP_LOGE(TAG, "Network worker could not start");
    if (!app_start()) {
        ESP_LOGE(TAG, "Desktop startup failed; check the preceding errors");
        if (bsp_display_lock(0)) {
            auto *error = lv_label_create(lv_screen_active());
            lv_label_set_text(error, "Desktop startup failed\nSee serial log");
            lv_obj_center(error);
            bsp_display_unlock();
        }
    }
    // Present the completed desktop (or startup error) before lighting the LCD.
    ret = bsp_display_show();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Initial screen presentation failed: %s", esp_err_to_name(ret));
    }
}
