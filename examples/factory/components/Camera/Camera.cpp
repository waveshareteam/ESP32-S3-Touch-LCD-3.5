#include "Camera.hpp"
#include "img_converters.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "src/misc/cache/instance/lv_image_cache.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <new>
LV_IMAGE_DECLARE(img_app_camera);
namespace { constexpr unsigned W=224,H=336,BYTES=W*H*2; }
using namespace app_ui;
namespace esp_brookesia::apps {
Camera *Camera::instance_;
Camera *Camera::requestInstance() { if(!instance_) instance_=new(std::nothrow) Camera; return instance_; }
Camera::Camera():App("Camera",&img_app_camera,true,false,false) {}
Camera::~Camera() { close(); if(mutex_) vSemaphoreDelete(mutex_); instance_=nullptr; }
bool Camera::run() {
    if(!mutex_) mutex_=xSemaphoreCreateMutex();
    if(!mutex_) return false;
    pixels_=static_cast<uint16_t *>(heap_caps_calloc(1,BYTES,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
    pending_=static_cast<uint16_t *>(heap_caps_malloc(BYTES,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
    if(!pixels_ || !pending_) { close(); return false; }
    capture_=false; revision_=shown_=0; snprintf(message_,sizeof(message_),"Starting camera...");
    button_presses_=bsp_user_button_get_press_count();
    root_=screen("Camera",event,this);
    image_={}; image_.header.magic=LV_IMAGE_HEADER_MAGIC; image_.header.cf=LV_COLOR_FORMAT_RGB565;
    image_.header.w=W; image_.header.h=H; image_.header.stride=W*2; image_.data_size=BYTES; image_.data=reinterpret_cast<uint8_t *>(pixels_);
    image_obj_=lv_image_create(root_); lv_image_set_src(image_obj_,&image_); lv_obj_set_pos(image_obj_,48,72);
    status_=label(root_,"Starting camera...",12,414,296,&lv_font_montserrat_12,MUTED);
    lv_obj_set_style_text_align(status_,LV_TEXT_ALIGN_CENTER,0);
    button(root_,LV_SYMBOL_IMAGE "  Capture",64,440,192,32,1,event,this,true);
    timer_=lv_timer_create(poll,66,this);
    if(!timer_ || !start("camera_app",8192,2)) { close(); return false; }
    return true;
}
void Camera::message(const char *s) { xSemaphoreTake(mutex_,portMAX_DELAY); snprintf(message_,sizeof(message_),"%s",s); xSemaphoreGive(mutex_); }
void Camera::save(camera_fb_t *frame) {
    message("Saving photo...");
    auto ret=bsp_sdcard_acquire();
    if(ret!=ESP_OK) { message("No SD card. Preview remains available."); return; }
    uint8_t *jpg=nullptr; size_t size=0;
    FILE *file=nullptr; char path[128]={}; bool ok=false;
    do {
        if(mkdir("/sdcard/photo",0775)!=0 && errno!=EEXIST) break;
        if(!frame2jpg(frame,85,&jpg,&size) || !jpg || !size) break;
        bsp_sensor_datetime_t t={}; bool rtc=bsp_rtc_get_datetime(&t)==ESP_OK;
        for(unsigned i=0;i<1000;++i) {
            if(rtc) snprintf(path,sizeof(path),"/sdcard/photo/CAM_%04u%02u%02u_%02u%02u%02u_%03u.jpg",t.year,t.month,t.day,t.hour,t.minute,t.second,i);
            else snprintf(path,sizeof(path),"/sdcard/photo/CAM_%lld_%03u.jpg",esp_timer_get_time()/1000,i);
            int fd=open(path,O_CREAT|O_EXCL|O_WRONLY,0666);
            if(fd>=0) { file=fdopen(fd,"wb"); if(!file) { ::close(fd); unlink(path); } break; }
            if(errno!=EEXIST) break;
        }
        if(!file) break;
        bool written=fwrite(jpg,1,size,file)==size;
        int closed=fclose(file); file=nullptr;
        ok=written && closed==0;
        if(!ok) unlink(path); // Only the incomplete file created by this capture.
    } while(false);
    free(jpg); bsp_sdcard_release();
    if(ok) { char msg[96]; snprintf(msg,sizeof(msg),"Saved: %s",strrchr(path,'/')+1); message(msg); ESP_LOGI("camera","Saved %s",path); }
    else message("Photo save failed. Check SD card / free space.");
}
void Camera::work() {
    const esp_err_t initialized=bsp_camera_init();
    if(initialized!=ESP_OK) {
        char detail[96];
        snprintf(detail,sizeof(detail),"Camera init: %s. Reopen to retry.",esp_err_to_name(initialized));
        message(detail);
        ESP_LOGE("camera","BSP camera initialization failed: %s",esp_err_to_name(initialized));
        return;
    }
    if(!bsp_camera_acquire(1000)) { message("Camera busy. Reopen to retry."); return; }
    message("Live preview / BOOT: rotate 180 deg");
    while(!stopping()) {
        auto *frame=bsp_camera_get_frame();
        if(!frame) { message("Camera timeout. Reopen to retry."); break; }
        if(frame->format!=PIXFORMAT_RGB565 || !frame->width || !frame->height || frame->len<frame->width*frame->height*2) {
            bsp_camera_return_frame(frame); message("Unexpected camera frame format"); break;
        }
        const uint32_t presses=bsp_user_button_get_press_count();
        const uint32_t new_presses=presses-button_presses_;
        button_presses_=presses;
        // Two presses cancel out, including presses received while saving a photo.
        if((new_presses & 1U) && !stopping()) {
            const esp_err_t ret=bsp_camera_rotate_180();
            message(ret==ESP_OK ? "Live preview / BOOT: rotate 180 deg" : "Rotation failed. Press BOOT to retry.");
            // With one framebuffer, holding this completed frame prevents the
            // next capture while both sensor axes change. Discard the old view;
            // any pending photo request uses the next frame in the new direction.
            bsp_camera_return_frame(frame);
            continue;
        }
        xSemaphoreTake(mutex_,portMAX_DELAY);
        // Sensor RGB565 is MSB first; LVGL RGB565 on S3 is little-endian.
        for(unsigned y=0;y<H;++y) for(unsigned x=0;x<W;++x) {
            size_t i=((size_t)y*frame->height/H*frame->width+(size_t)x*frame->width/W)*2;
            pending_[y*W+x]=(uint16_t(frame->buf[i])<<8)|frame->buf[i+1];
        }
        ++revision_; xSemaphoreGive(mutex_);
        if(capture_.exchange(false) && !stopping()) save(frame);
        bsp_camera_return_frame(frame);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    bsp_camera_release();
}
void Camera::refresh() {
    bool update=false; char s[96];
    xSemaphoreTake(mutex_,portMAX_DELAY);
    if(shown_!=revision_) { memcpy(pixels_,pending_,BYTES); shown_=revision_; update=true; }
    snprintf(s,sizeof(s),"%s",message_); xSemaphoreGive(mutex_);
    if(update) { lv_image_cache_drop(&image_); lv_obj_invalidate(image_obj_); }
    text(status_,s);
}
void Camera::event(lv_event_t *e) { auto *a=static_cast<Camera *>(lv_event_get_user_data(e)); if(action(e)==0) a->back(); else a->capture_=true; }
void Camera::poll(lv_timer_t *t) { static_cast<Camera *>(lv_timer_get_user_data(t))->refresh(); }
bool Camera::pause() { capture_=false; if(timer_) lv_timer_pause(timer_); stop(); return true; }
bool Camera::resume() {
    // Ignore BOOT presses made on the desktop or in another APP while paused.
    button_presses_=bsp_user_button_get_press_count();
    if(timer_) lv_timer_resume(timer_);
    return start("camera_app",8192,2);
}
bool Camera::close() {
    capture_=false; stop(); if(timer_) { lv_timer_delete(timer_); timer_=nullptr; }
    if(root_) lv_obj_delete(root_);
    root_=image_obj_=status_=nullptr;
    lv_image_cache_drop(&image_); heap_caps_free(pixels_); heap_caps_free(pending_); pixels_=pending_=nullptr; return true;
}
}
