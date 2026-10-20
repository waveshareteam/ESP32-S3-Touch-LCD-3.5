#include "desktop.hpp"
#include "bsp/esp-bsp.h"
#include "wifi_manager.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

using esp_brookesia::systems::phone::StatusBar;

namespace {
struct Snapshot {
    bsp_battery_info_t battery;
    bool battery_valid;
    int wifi_level;
};
Snapshot latest = {};
portMUX_TYPE snapshot_lock = portMUX_INITIALIZER_UNLOCKED;

// Keep S3 hardware reads off the LVGL task. No C5 pins or drivers are used here.
void telemetry(void *)
{
    for (;;) {
        Snapshot value = {};
        value.battery_valid = bsp_battery_get_info(&value.battery) == ESP_OK;
        wifi_ap_record_t ap = {};
        if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK) {
            value.wifi_level = ap.rssi >= -60 ? 3 : ap.rssi >= -75 ? 2 : 1;
        }
        portENTER_CRITICAL(&snapshot_lock);
        latest = value;
        portEXIT_CRITICAL(&snapshot_lock);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void refresh(lv_timer_t *timer)
{
    auto *bar = static_cast<StatusBar *>(lv_timer_get_user_data(timer));
    Snapshot value;
    portENTER_CRITICAL(&snapshot_lock);
    value = latest;
    portEXIT_CRITICAL(&snapshot_lock);
    struct tm local;
    if (wifi_manager_get_local_time(&local)) bar->setClock(local.tm_hour, local.tm_min);
    else bar->clearClock();
    bar->setWifiIconState(value.wifi_level);
    if (value.battery_valid && value.battery.connected &&
        value.battery.percentage >= 0 && value.battery.percentage <= 100) {
        bar->setBatteryPercent(value.battery.charging, value.battery.percentage);
    } else {
        // The stock C5 bar has only a battery icon. Do not imply a battery exists
        // or is charging when the S3 board is powered solely through USB.
        bar->hideBatteryIcon();
    }
}
}

bool desktop_status_start(StatusBar *bar)
{
    if (!bar) return false;
    bar->setClockFormat(StatusBar::ClockFormat::FORMAT_24H);
    bar->clearClock();
    bar->hideBatteryIcon();
    bar->setWifiIconState(0);
    auto *timer = lv_timer_create(refresh, 1000, bar);
    if (!timer) return false;
    if (xTaskCreate(telemetry, "desktop_status", 4096, nullptr, 1, nullptr) != pdPASS) {
        lv_timer_delete(timer);
        return false;
    }
    return true;
}
