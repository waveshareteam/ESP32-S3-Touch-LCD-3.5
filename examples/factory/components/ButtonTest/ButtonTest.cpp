/* SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0 */
#include "ButtonTest.hpp"
#include "esp_log.h"
#include "esp_timer.h"
#include <cstdio>
#include <new>

LV_IMAGE_DECLARE(img_app_button_test);

namespace {
constexpr uint32_t GREEN = 0x188563, ERROR = 0xc64f55;
constexpr unsigned SAMPLE_MS = 20, STABLE_SAMPLES = 3, MAX_COUNT = 9999;

// Same release -> press -> release test as the 2.16 Button Test.
// Three stable samples reject contact bounce. Holding a key on entry cannot pass.
struct Sequence {
    enum Phase { InitialRelease, Press, FinalRelease } phase = InitialRelease;
    int candidate = -1, state = -1;
    unsigned samples = 0;
    bool update(int raw) {
        if (raw < 0) { *this = {}; return false; }
        if (raw != candidate) { candidate = raw; samples = 1; }
        else if (samples < STABLE_SAMPLES) ++samples;
        if (samples < STABLE_SAMPLES) return false;
        state = raw;
        if (phase == InitialRelease && !raw) phase = Press;
        else if (phase == Press && raw) phase = FinalRelease;
        else if (phase == FinalRelease && !raw) { phase = Press; return true; }
        return false;
    }
};
} // namespace

using namespace app_ui;
namespace esp_brookesia::apps {
ButtonTest *ButtonTest::instance_;
ButtonTest *ButtonTest::requestInstance()
{
    if (!instance_) instance_ = new (std::nothrow) ButtonTest;
    return instance_;
}
ButtonTest::ButtonTest() : App("Button Test", &img_app_button_test, true, false, false) {}

void ButtonTest::publish(const View &view)
{
    portENTER_CRITICAL(&lock_);
    view_ = view;
    portEXIT_CRITICAL(&lock_);
}
ButtonTest::View ButtonTest::snapshot()
{
    portENTER_CRITICAL(&lock_);
    View view = view_;
    portEXIT_CRITICAL(&lock_);
    return view;
}
ButtonTest::Card ButtonTest::createCard(const char *name, const char *signal, int y)
{
    Card card;
    card.panel = box(root_, 16, y, 288, 110);
    lv_obj_set_style_border_width(card.panel, 1, 0);
    lv_obj_set_style_border_color(card.panel, lv_color_hex(0xe4eaf2), 0);
    label(card.panel, name, 16, 15, 134, &lv_font_montserrat_20);
    label(card.panel, signal, 16, 46, 258, &lv_font_montserrat_12, MUTED);
    card.badge = box(card.panel, 174, 14, 98, 28, 0xf0f3f8, 8);
    card.state = label(card.badge, "WAITING", 0, 0, 98, &lv_font_montserrat_12, MUTED);
    lv_obj_set_style_text_align(card.state, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(card.state);
    card.count = label(card.panel, "Presses: 0", 16, 80, 150, &lv_font_montserrat_14);
    card.result = label(card.panel, "NOT TESTED", 164, 81, 108, &lv_font_montserrat_12, MUTED);
    lv_obj_set_style_text_align(card.result, LV_TEXT_ALIGN_RIGHT, 0);
    return card;
}
bool ButtonTest::run()
{
    root_ = screen("Button Test", event, this);
    label(root_, "Press and release each physical button", 16, 72, 288,
          &lv_font_montserrat_12, MUTED);
    cards_[0] = createCard("BOOT", "GPIO0  /  Active low", 100);
    cards_[1] = createCard("PWR", "SYS_OUT  /  EXIO6  /  Active high", 222);
    summary_ = label(root_, "0 / 2 buttons passed", 16, 350, 288, &lv_font_montserrat_18);
    hint_ = label(root_, "Ready. Start with either button.", 16, 378, 288,
                  &lv_font_montserrat_12, MUTED);
    lv_obj_set_style_text_align(summary_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_align(hint_, LV_TEXT_ALIGN_CENTER, 0);
    progress_ = lv_bar_create(root_);
    lv_obj_set_pos(progress_, 36, 400);
    lv_obj_set_size(progress_, 248, 5);
    lv_bar_set_range(progress_, 0, 2);
    lv_obj_set_style_bg_color(progress_, lv_color_hex(0xe1e7f0), 0);
    lv_obj_set_style_bg_color(progress_, lv_color_hex(GREEN), LV_PART_INDICATOR);
    button(root_, LV_SYMBOL_REFRESH "  Test again", 16, 418, 288, 40, 1, event, this, true);
    auto *note = label(root_, "Short press PWR. Long hold powers off.", 16, 462, 288,
                      &lv_font_montserrat_12, MUTED);
    lv_obj_set_style_text_align(note, LV_TEXT_ALIGN_CENTER, 0);
    publish(View{});
    reset_ = false;
    shown_result_ = -1;
    refresh();
    timer_ = lv_timer_create(poll, 50, this);
    if (!timer_ || !start("button_test", 4096, 2)) { close(); return false; }
    return true;
}
void ButtonTest::work()
{
    View view = snapshot();
    Sequence sequence[2];
    // Reset phases on resume, keeping completed test counts.
    view.buttons[0].state = view.buttons[1].state = -1;
    constexpr bsp_button_id_t ids[] = {BSP_BUTTON_ID_BOOT, BSP_BUTTON_ID_PWR};
    esp_err_t last_error[] = {ESP_OK, ESP_OK};
    while (!stopping()) {
        const int64_t sample_start = esp_timer_get_time();
        if (reset_.exchange(false)) {
            view = View{};
            sequence[0] = sequence[1] = Sequence{};
        }
        for (int i = 0; i < 2; ++i) {
            bool pressed = false;
            const esp_err_t ret = bsp_button_get_pressed(ids[i], &pressed);
            if (ret != last_error[i]) {
                if (ret == ESP_OK) ESP_LOGI("button_test", "%s input ready", i ? "PWR EXIO6" : "BOOT GPIO0");
                else ESP_LOGW("button_test", "%s read failed: %s", i ? "PWR EXIO6" : "BOOT GPIO0", esp_err_to_name(ret));
                last_error[i] = ret;
            }
            if (sequence[i].update(ret == ESP_OK ? int(pressed) : -1)) {
                if (view.buttons[i].count < MAX_COUNT) ++view.buttons[i].count;
                ESP_LOGI("button_test", "%s passed, presses=%lu", i ? "PWR EXIO6" : "BOOT GPIO0",
                         static_cast<unsigned long>(view.buttons[i].count));
            }
            view.buttons[i].state = sequence[i].state;
        }
        publish(view);
        const int64_t elapsed_ms = (esp_timer_get_time() - sample_start) / 1000;
        // Always yield, including after an I2C error or timeout.
        vTaskDelay(pdMS_TO_TICKS(elapsed_ms < SAMPLE_MS ? SAMPLE_MS - elapsed_ms : SAMPLE_MS));
    }
}
void ButtonTest::refresh()
{
    const View view = snapshot();
    int passed = 0;
    bool unavailable = false;
    for (int i = 0; i < 2; ++i) {
        const Reading &reading = view.buttons[i];
        Card &card = cards_[i];
        passed += reading.count > 0;
        unavailable |= reading.state < 0;
        if (card.shown_state == reading.state && card.shown_count == reading.count) continue;
        card.shown_state = reading.state;
        card.shown_count = reading.count;
        const bool pressed = reading.state == 1;
        text(card.state, reading.state < 0 ? "NO INPUT" : pressed ? "PRESSED" : "RELEASED");
        lv_obj_set_style_bg_color(card.badge, lv_color_hex(pressed ? 0xe1f4ec : 0xf0f3f8), 0);
        lv_obj_set_style_text_color(card.state, lv_color_hex(reading.state < 0 ? ERROR : pressed ? GREEN : MUTED), 0);
        lv_obj_set_style_border_color(card.panel, lv_color_hex(pressed ? GREEN : 0xe4eaf2), 0);
        char count[32];
        std::snprintf(count, sizeof(count), "Presses: %lu", static_cast<unsigned long>(reading.count));
        text(card.count, count);
        text(card.result, reading.state < 0 ? "CHECK INPUT" : reading.count ? LV_SYMBOL_OK " PASS" : "NOT TESTED");
        lv_obj_set_style_text_color(card.result, lv_color_hex(reading.state < 0 ? ERROR : reading.count ? GREEN : MUTED), 0);
    }
    const int result = (view.buttons[0].count ? 1 : 0) | (view.buttons[1].count ? 2 : 0) |
                       (unavailable ? 4 : 0);
    if (shown_result_ == result) return;
    shown_result_ = result;
    char summary[32];
    std::snprintf(summary, sizeof(summary), "%d / 2 buttons passed", passed);
    text(summary_, summary);
    lv_obj_set_style_text_color(summary_, lv_color_hex(unavailable ? ERROR : passed == 2 ? GREEN : INK), 0);
    text(hint_, unavailable ? "Waiting for button inputs..." : passed == 2 ? "All done. Both buttons work correctly." :
         view.buttons[0].count ? "Next: press and release PWR." :
         view.buttons[1].count ? "Next: press and release BOOT." : "Ready. Start with either button.");
    lv_bar_set_value(progress_, passed, LV_ANIM_OFF);
}
void ButtonTest::event(lv_event_t *event)
{
    auto *app = static_cast<ButtonTest *>(lv_event_get_user_data(event));
    if (action(event) == 0) { app->back(); return; }
    // The worker alone owns the debounce state and counters.
    app->reset_ = true;
}
void ButtonTest::poll(lv_timer_t *timer)
{
    static_cast<ButtonTest *>(lv_timer_get_user_data(timer))->refresh();
}
bool ButtonTest::pause()
{
    if (timer_) lv_timer_pause(timer_);
    stop();
    return true;
}
bool ButtonTest::resume()
{
    if (!start("button_test", 4096, 2)) return false;
    if (timer_) lv_timer_resume(timer_);
    return true;
}
bool ButtonTest::close()
{
    stop();
    if (timer_) { lv_timer_delete(timer_); timer_ = nullptr; }
    if (root_) { lv_obj_delete(root_); root_ = nullptr; }
    cards_[0] = cards_[1] = Card{};
    summary_ = hint_ = progress_ = nullptr;
    return true;
}
} // namespace esp_brookesia::apps
