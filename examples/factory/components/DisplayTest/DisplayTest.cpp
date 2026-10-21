/* SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0 */
#include "DisplayTest.hpp"
#include "bsp/touch.h"
#include "esp_log.h"
#include <new>

LV_IMAGE_DECLARE(img_app_display_test);
namespace {
// Retain the old rgb_tile.cpp timer approach and its one-second interval.
constexpr uint32_t COLORS[] = {0xff0000, 0x00ff00, 0x0000ff, 0xffff00, 0xffffff};
constexpr unsigned COLOR_COUNT = sizeof(COLORS) / sizeof(COLORS[0]);
constexpr uint32_t COLOR_PERIOD_MS = 1000, GESTURE_POLL_MS = 10, UI_POLL_MS = 20;

esp_brookesia::systems::phone::App::Config fullscreen_config()
{
    auto config = esp_brookesia::systems::phone::App::Config::SIMPLE_CONSTRUCTOR(
        &img_app_display_test, false, false);
    // Keep the entire panel, including the edges, available for color inspection
    // and double-tap exit. Other APPs retain their normal navigation gestures.
    config.flags.enable_navigation_gesture = false;
    return config;
}
}

namespace esp_brookesia::apps {
DisplayTest *DisplayTest::instance_;
DisplayTest *DisplayTest::requestInstance()
{
    if (!instance_) instance_ = new (std::nothrow) DisplayTest;
    return instance_;
}
DisplayTest::DisplayTest()
    : App(systems::base::App::Config::SIMPLE_CONSTRUCTOR("Display Test", &img_app_display_test, true),
          fullscreen_config()) {}

bool DisplayTest::run()
{
    const auto area = getVisualArea();
    color_index_ = 0;
    root_ = lv_obj_create(lv_screen_active());
    if (!root_) { close(); return false; }
    lv_obj_remove_style_all(root_);
    lv_obj_set_pos(root_, area.x1, area.y1);
    lv_obj_set_size(root_, lv_area_get_width(&area), lv_area_get_height(&area));
    lv_obj_set_style_bg_color(root_, lv_color_hex(COLORS[0]), 0);
    lv_obj_set_style_bg_opa(root_, LV_OPA_COVER, 0);
    lv_obj_set_scrollable(root_, false);
    lv_obj_set_scroll_chain(root_, false);
    lv_obj_set_gesture_bubble(root_, false);
    lv_obj_set_clickable(root_, true);
    lv_obj_set_press_lock(root_, true);
    timer_ = lv_timer_create(cycle, COLOR_PERIOD_MS, this);
    if (!timer_) { close(); return false; }
    gesture_timer_ = lv_timer_create(pollGesture, UI_POLL_MS, this);
    if (!gesture_timer_) { close(); return false; }
    if (!startGestureReader()) { close(); return false; }
    ESP_LOGI("display_test", "Red -> green -> blue -> yellow -> white, 1 s each; FT6336 hardware double click to exit");
    return true;
}
void DisplayTest::cycle(lv_timer_t *timer)
{
    auto *app = static_cast<DisplayTest *>(lv_timer_get_user_data(timer));
    app->color_index_ = (app->color_index_ + 1) % COLOR_COUNT;
    lv_obj_set_style_bg_color(app->root_, lv_color_hex(COLORS[app->color_index_]), 0);
}
void DisplayTest::pollGesture(lv_timer_t *timer)
{
    auto *app = static_cast<DisplayTest *>(lv_timer_get_user_data(timer));
    // The worker retains the event even while a full-screen flush delays LVGL.
    if (!app->exit_requested_.exchange(false)) return;
    const esp_err_t ret = app->gesture_error_.load();
    lv_timer_pause(timer);
    if (ret == ESP_OK) ESP_LOGI("display_test", "Hardware double click: returning to desktop");
    else ESP_LOGW("display_test", "Touch gesture read failed: %s; leaving test", esp_err_to_name(ret));
    app->back();
}
bool DisplayTest::startGestureReader()
{
    exit_requested_ = false;
    gesture_error_ = ESP_OK;
    if (!setGestureEnabled(true)) return false;
    if (!start("display_gesture", 4096, 4)) {
        setGestureEnabled(false);
        return false;
    }
    return true;
}
void DisplayTest::work()
{
    unsigned errors = 0;
    // Hardware I2C polling is independent of LVGL rendering and color changes.
    // This task never touches LVGL objects or requests APP closure itself.
    while (!stopping()) {
        bool detected = false;
        const esp_err_t ret = bsp_touch_get_double_click(&detected);
        errors = ret == ESP_OK ? 0 : errors + 1;
        if (detected || errors >= 3) {
            gesture_error_ = ret;
            if (detected) ESP_LOGI("display_test", "FT6336 double click captured by 10 ms reader");
            exit_requested_ = true;
            return;
        }
        vTaskDelay(pdMS_TO_TICKS(GESTURE_POLL_MS));
    }
}
bool DisplayTest::setGestureEnabled(bool enabled)
{
    if (gesture_enabled_ == enabled) return true;
    const esp_err_t ret = bsp_touch_set_gesture_enabled(enabled);
    if (ret != ESP_OK) {
        ESP_LOGE("display_test", "Set touch gesture mode: %s", esp_err_to_name(ret));
        return false;
    }
    gesture_enabled_ = enabled;
    return true;
}
bool DisplayTest::pause()
{
    if (gesture_timer_) lv_timer_pause(gesture_timer_);
    if (timer_) lv_timer_pause(timer_);
    stop();
    return setGestureEnabled(false);
}
bool DisplayTest::resume()
{
    if (!startGestureReader()) return false;
    if (timer_) { lv_timer_reset(timer_); lv_timer_resume(timer_); }
    if (gesture_timer_) { lv_timer_reset(gesture_timer_); lv_timer_resume(gesture_timer_); }
    return true;
}
bool DisplayTest::close()
{
    stop();
    if (gesture_timer_) { lv_timer_delete(gesture_timer_); gesture_timer_ = nullptr; }
    if (timer_) { lv_timer_delete(timer_); timer_ = nullptr; }
    if (root_) { lv_obj_delete(root_); root_ = nullptr; }
    return setGestureEnabled(false);
}
} // namespace esp_brookesia::apps
