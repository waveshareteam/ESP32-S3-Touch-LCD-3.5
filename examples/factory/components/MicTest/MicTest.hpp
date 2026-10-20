#pragma once
#include "systems/phone/esp_brookesia_phone_app.hpp"
#include "app_support.hpp"

namespace esp_brookesia::apps {
class MicTest : public systems::phone::App, private AppWorker {
public:
    static MicTest *requestInstance();
    ~MicTest() override { close(); instance_=nullptr; }
protected:
    bool init() override { return true; }
    bool run() override;
    bool back() override { return notifyCoreClosed(); }
    bool close() override;
    bool deinit() override { return close(); }
    bool pause() override;
    bool resume() override;
private:
    enum class Command { None, Record, Stop, Play };
    enum class Mode { Monitor, Record, Play };
    struct View {
        Mode mode=Mode::Monitor;
        bool ready=false, playable=false;
        unsigned level=0, elapsed_ms=0;
        char status[112]="Preparing microphone...";
    };
    MicTest();
    static MicTest *instance_;
    static void event(lv_event_t *);
    static void poll(lv_timer_t *);
    void work() override;
    void refresh();
    void publish(const View &view);
    View snapshot();
    portMUX_TYPE lock_=portMUX_INITIALIZER_UNLOCKED;
    View view_;
    std::atomic<Command> command_{Command::None};
    char last_file_[128]={};
    lv_obj_t *root_=nullptr,*meter_=nullptr,*level_=nullptr,*status_=nullptr;
    lv_obj_t *record_=nullptr,*play_=nullptr,*record_label_=nullptr,*play_label_=nullptr;
    lv_timer_t *timer_=nullptr;
};
}
