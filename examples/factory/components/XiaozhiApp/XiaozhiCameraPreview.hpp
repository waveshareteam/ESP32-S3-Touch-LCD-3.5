/* SPDX-License-Identifier: Apache-2.0 */
#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include "esp_camera.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

namespace esp_brookesia::apps {
// One owned mailbox between the MCP worker and LVGL. No camera framebuffer or
// LVGL pointer crosses tasks; only a reduced copy of the current frame does.
class XiaozhiCameraPreview {
public:
    static constexpr unsigned WIDTH = 128, HEIGHT = 192;
    static constexpr size_t BYTES = WIDTH * HEIGHT * sizeof(uint16_t);
    enum class Phase { Hidden, Preview, Encoding, Analyzing, Complete, Error };
    struct Snapshot {
        Phase phase = Phase::Hidden;
        bool has_frame = false;
        bool rotate_pending = false, rotation_notice = false, rotation_failed = false;
    };

    XiaozhiCameraPreview();
    ~XiaozhiCameraPreview();
    XiaozhiCameraPreview(const XiaozhiCameraPreview &) = delete;
    XiaozhiCameraPreview &operator=(const XiaozhiCameraPreview &) = delete;
    void setEnabled(bool enabled, uint32_t button_presses = 0);
    bool observeButton(uint32_t session, uint32_t button_presses);
    bool rotationRequest(uint32_t session, uint32_t &request);
    void finishRotation(uint32_t session, uint32_t request, bool success);
    uint32_t session() const { return _session.load(); }
    bool begin(uint32_t session);
    bool publish(uint32_t session, const camera_fb_t &frame, bool final_frame);
    void setPhase(uint32_t session, Phase phase);
    // Dismiss invalidates in-flight frames/results without disabling the next
    // photo or discarding BOOT orientation requests.
    void dismiss();
    // Observe TTS arrival, including sentence starts, before queuing control
    // events. Only speech following a finished analysis can dismiss its photo.
    void noteSpeech(bool stopped);
    bool finishPlayback();
    // Nonblocking UI read. A null destination queries state without consuming
    // an image update, so LVGL can allocate its own buffer only when needed.
    bool read(uint32_t &revision, uint16_t *pixels, Snapshot &snapshot);
private:
    bool active(uint32_t session) const;
    void dismissLocked();
    StaticSemaphore_t _mutex_storage{};
    SemaphoreHandle_t _mutex = nullptr;
    std::atomic<uint32_t> _session{0};
    uint32_t _revision = 0;
    uint16_t *_pixels = nullptr;
    Snapshot _snapshot{};
    bool _frozen = false;
    bool _reply_started = false, _reply_stopped = false;
    uint32_t _button_presses = 0, _rotation_requested = 0, _rotation_applied = 0;
    uint32_t _rotation_owner = 0;
    bool _rotation_notice = false, _rotation_failed = false;
};
XiaozhiCameraPreview &xiaozhiCameraPreview();
}
