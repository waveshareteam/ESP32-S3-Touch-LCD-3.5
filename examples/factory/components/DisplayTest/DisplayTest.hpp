/* SPDX-License-Identifier: Apache-2.0 */
#pragma once
#include "systems/phone/esp_brookesia_phone_app.hpp"
#include "app_support.hpp"

namespace esp_brookesia::apps {
class DisplayTest final : public systems::phone::App, private AppWorker {
public:
    static DisplayTest *requestInstance();
    ~DisplayTest() override { close(); instance_ = nullptr; }
protected:
    DisplayTest();
    bool init() override { return true; }
    bool run() override;
    bool back() override { return notifyCoreClosed(); }
    bool close() override;
    bool deinit() override { return close(); }
    bool pause() override;
    bool resume() override;
private:
    static DisplayTest *instance_;
    static void cycle(lv_timer_t *timer);
    static void pollGesture(lv_timer_t *timer);
    bool setGestureEnabled(bool enabled);
    bool startGestureReader();
    void work() override;
    lv_obj_t *root_ = nullptr;
    lv_timer_t *timer_ = nullptr;
    lv_timer_t *gesture_timer_ = nullptr;
    unsigned color_index_ = 0;
    bool gesture_enabled_ = false;
    std::atomic<bool> exit_requested_{false};
    std::atomic<esp_err_t> gesture_error_{ESP_OK};
};
} // namespace esp_brookesia::apps
