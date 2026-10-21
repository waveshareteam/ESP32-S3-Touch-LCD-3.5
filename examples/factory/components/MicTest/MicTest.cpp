#include "bsp_board_extra.h"
/* SPDX-License-Identifier: Apache-2.0
 * C5 3.5 record/replay workflow, using the S3 BSP's shared audio session. */
#include "MicTest.hpp"
#include "esp_timer.h"
#include "esp_log.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <new>

LV_IMAGE_DECLARE(img_app_specanalyzer);
namespace {
constexpr unsigned RATE=16000, MAX_BYTES=RATE*2*10;
struct __attribute__((packed)) WavHeader {
    char riff[4]; uint32_t file_size; char wave[4],fmt[4]; uint32_t fmt_size;
    uint16_t format,channels; uint32_t rate,byte_rate; uint16_t align,bits;
    char data[4]; uint32_t data_size;
};
static_assert(sizeof(WavHeader)==44);
WavHeader header(unsigned bytes) {
    return {{'R','I','F','F'},36+bytes,{'W','A','V','E'},{'f','m','t',' '},16,
            1,1,RATE,RATE*2,2,16,{'d','a','t','a'},bytes};
}
}
using namespace app_ui;
namespace esp_brookesia::apps {
MicTest *MicTest::instance_;
MicTest *MicTest::requestInstance() { if(!instance_) instance_=new(std::nothrow) MicTest; return instance_; }
MicTest::MicTest():App("Mic Test",&img_app_specanalyzer,true,false,false) {}
void MicTest::publish(const View &v) { portENTER_CRITICAL(&lock_); view_=v; portEXIT_CRITICAL(&lock_); }
MicTest::View MicTest::snapshot() { portENTER_CRITICAL(&lock_); View v=view_; portEXIT_CRITICAL(&lock_); return v; }
bool MicTest::run() {
    root_=screen("Mic Test",event,this);
    auto *card=box(root_,16,78,288,226);
    auto *title=label(card,"MICROPHONE",0,20,288,&lv_font_montserrat_12,MUTED);
    lv_obj_set_style_text_align(title,LV_TEXT_ALIGN_CENTER,0);
    level_=label(card,"0%",0,60,288,&lv_font_montserrat_32,BLUE);
    lv_obj_set_style_text_align(level_,LV_TEXT_ALIGN_CENTER,0);
    meter_=lv_bar_create(card); lv_obj_set_pos(meter_,24,126); lv_obj_set_size(meter_,240,16);
    lv_bar_set_range(meter_,0,100); lv_obj_set_style_bg_color(meter_,lv_color_hex(0xe5ecf8),0);
    lv_obj_set_style_bg_color(meter_,lv_color_hex(BLUE),LV_PART_INDICATOR);
    auto *hint=label(card,"Speak to check the microphone\n16 kHz / Mono / 16-bit",16,166,256,&lv_font_montserrat_12,MUTED);
    lv_obj_set_style_text_align(hint,LV_TEXT_ALIGN_CENTER,0);
    status_=label(root_,"Preparing microphone...",16,322,288,&lv_font_montserrat_14,INK);
    lv_obj_set_style_text_align(status_,LV_TEXT_ALIGN_CENTER,0); lv_obj_set_height(status_,68);
    record_=button(root_,"Record",16,410,136,48,1,event,this,true);
    play_=button(root_,"Play",168,410,136,48,2,event,this);
    record_label_=lv_obj_get_child(record_,0); play_label_=lv_obj_get_child(play_,0);
    command_=Command::None; publish(View{});
    timer_=lv_timer_create(poll,100,this);
    if(!timer_ || !start("mic_test",8192,3)) { close(); return false; }
    return true;
}
void MicTest::work() {
    View v; publish(v);
    if(!bsp_extra_audio_acquire(BSP_EXTRA_AUDIO_OWNER_RECORDER, 1000)) { snprintf(v.status,sizeof(v.status),"Audio unavailable or busy. Reopen to retry."); publish(v); return; }
    if(bsp_extra_codec_set_fs(RATE, 16, static_cast<i2s_slot_mode_t>(1))!=ESP_OK) {
        snprintf(v.status,sizeof(v.status),"Microphone initialization failed."); publish(v); bsp_extra_audio_release(); return;
    }
    FILE *file=nullptr; bool sd=false; unsigned bytes=0, playback_left=0;
    char active_path[128]={}; int16_t samples[512];
    v.ready=true; v.playable=last_file_[0]; snprintf(v.status,sizeof(v.status),"Ready. Record up to 10 seconds, then play it back.");
    auto finish=[&](bool success) {
        if(file) {
            if(v.mode==Mode::Record) {
                WavHeader h=header(bytes);
                success=(fseek(file,0,SEEK_SET)==0 && fwrite(&h,1,sizeof(h),file)==sizeof(h)) && success;
            }
            success=(fclose(file)==0) && success; file=nullptr;
            if(v.mode==Mode::Record) {
                if(success && bytes) {
                    snprintf(last_file_,sizeof(last_file_),"%s",active_path); v.playable=true;
                    snprintf(v.status,sizeof(v.status),"Saved %u.%u s. Tap Play to listen.",bytes/(RATE*2),(bytes*10/(RATE*2))%10);
                    ESP_LOGI("mic_test","Saved %s: %u bytes, 16000 Hz mono PCM16",last_file_,bytes);
                } else { unlink(active_path); snprintf(v.status,sizeof(v.status),"Recording failed. Check SD card / free space."); }
            } else if(!success) snprintf(v.status,sizeof(v.status),"Playback failed. Check audio / SD card.");
        }
        if(v.mode==Mode::Play) {
            bsp_extra_audio_drain();
            if(success) snprintf(v.status,sizeof(v.status),"Playback stopped. Ready to record again.");
        }
        if(sd) { bsp_sdcard_release(); sd=false; }
        v.mode=Mode::Monitor;
    };
    while(!stopping()) {
        const Command command=command_.exchange(Command::None);
        if(command==Command::Stop) { finish(true); v.elapsed_ms=0; }
        if(command==Command::Record || command==Command::Play) {
            finish(true); bytes=0; v.elapsed_ms=0;
            if(bsp_sdcard_acquire()!=ESP_OK) snprintf(v.status,sizeof(v.status),"Insert an SD card to record or replay.");
            else {
                sd=true;
                if(command==Command::Record) {
                    if(mkdir("/sdcard/recordings",0775)==0 || errno==EEXIST) {
                        for(unsigned i=0;i<100;++i) {
                            snprintf(active_path,sizeof(active_path),"/sdcard/recordings/MIC_%lld_%u.wav",esp_timer_get_time()/1000,i);
                            int fd=open(active_path,O_CREAT|O_EXCL|O_WRONLY,0666);
                            if(fd>=0) { file=fdopen(fd,"wb"); if(!file) { ::close(fd); unlink(active_path); } break; }
                            if(errno!=EEXIST) break;
                        }
                    }
                    if(file) {
                        v.mode=Mode::Record; WavHeader h=header(0);
                        if(fwrite(&h,1,sizeof(h),file)!=sizeof(h)) finish(false);
                    }
                } else if(last_file_[0]) {
                    file=fopen(last_file_,"rb");
                    if(file) {
                        v.mode=Mode::Play; WavHeader h={};
                        if(fread(&h,1,sizeof(h),file)!=sizeof(h) || memcmp(h.riff,"RIFF",4) || memcmp(h.wave,"WAVE",4) ||
                           memcmp(h.data,"data",4) || h.format!=1 || h.rate!=RATE || h.channels!=1 || h.bits!=16 || h.data_size>MAX_BYTES || h.data_size%2) finish(false);
                        else playback_left=h.data_size;
                    }
                }
                if(!file && sd) { bsp_sdcard_release(); sd=false; snprintf(v.status,sizeof(v.status),"Could not open recording on SD card."); }
            }
        }
        if(v.mode==Mode::Play) {
            size_t count=std::min<size_t>(sizeof(samples),playback_left);
            if(!count) { finish(true); snprintf(v.status,sizeof(v.status),"Playback finished. Ready to record again."); }
            else if(fread(samples,1,count,file)!=count || bsp_extra_i2s_write(samples, count, nullptr, 1000)!=ESP_OK) finish(false);
            else { playback_left-=count; bytes+=count; v.elapsed_ms=bytes*1000/(RATE*2); snprintf(v.status,sizeof(v.status),"Playing  %u.%u s",v.elapsed_ms/1000,(v.elapsed_ms/100)%10); }
        } else {
            if(bsp_extra_i2s_read(samples, sizeof(samples), nullptr, 1000)!=ESP_OK) { finish(false); v.ready=false; snprintf(v.status,sizeof(v.status),"Microphone read failed. Reopen to retry."); publish(v); break; }
            uint64_t energy=0;
            for(int16_t sample:samples) energy+=int64_t(sample)*sample;
            const double rms=std::sqrt(double(energy)/512.0);
            // Meter spans -60..0 dBFS, so normal speech remains visible.
            v.level=std::clamp<int>(int((20*std::log10(std::max(rms,1.0)/32768.0)+60)*100/60),0,100);
            if(v.mode==Mode::Record) {
                const size_t count=std::min<size_t>(sizeof(samples),MAX_BYTES-bytes);
                if(fwrite(samples,1,count,file)!=count) finish(false);
                else {
                    bytes+=count; v.elapsed_ms=bytes*1000/(RATE*2);
                    snprintf(v.status,sizeof(v.status),"Recording  %u.%u / 10.0 s",v.elapsed_ms/1000,(v.elapsed_ms/100)%10);
                    if(bytes==MAX_BYTES) finish(true);
                }
            }
        }
        publish(v);
    }
    finish(true); bsp_extra_audio_release(); v.ready=false; publish(v);
}
void MicTest::refresh() {
    View v=snapshot(); char value[20]; snprintf(value,sizeof(value),"%u%%",v.level);
    text(level_,value); lv_bar_set_value(meter_,v.level,LV_ANIM_OFF); text(status_,v.status);
    text(record_label_,v.mode==Mode::Record?"Stop":"Record");
    text(play_label_,v.mode==Mode::Play?"Stop":"Play");
    enabled(record_,v.ready && v.mode!=Mode::Play);
    enabled(play_,v.ready && v.playable && v.mode!=Mode::Record);
}
void MicTest::event(lv_event_t *e) {
    auto *a=static_cast<MicTest *>(lv_event_get_user_data(e)); int id=action(e);
    if(!id) { a->back(); return; }
    View v=a->snapshot();
    a->command_=id==1 ? (v.mode==Mode::Record?Command::Stop:Command::Record) : (v.mode==Mode::Play?Command::Stop:Command::Play);
}
void MicTest::poll(lv_timer_t *t) { static_cast<MicTest *>(lv_timer_get_user_data(t))->refresh(); }
bool MicTest::pause() { command_=Command::None; if(timer_) lv_timer_pause(timer_); stop(); return true; }
bool MicTest::resume() { command_=Command::None; if(timer_) lv_timer_resume(timer_); return start("mic_test",8192,3); }
bool MicTest::close() {
    stop(); if(timer_) { lv_timer_delete(timer_); timer_=nullptr; }
    if(root_) lv_obj_delete(root_);
    root_=nullptr;
    return true;
}
}
