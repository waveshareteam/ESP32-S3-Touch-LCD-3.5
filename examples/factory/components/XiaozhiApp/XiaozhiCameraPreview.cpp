/* SPDX-License-Identifier: Apache-2.0 */
#include "XiaozhiCameraPreview.hpp"
#include "esp_heap_caps.h"
#include <cstring>

namespace esp_brookesia::apps {
XiaozhiCameraPreview::XiaozhiCameraPreview()
    : _mutex(xSemaphoreCreateMutexStatic(&_mutex_storage)) {}
XiaozhiCameraPreview::~XiaozhiCameraPreview()
{
    heap_caps_free(_pixels);
    if (_mutex) vSemaphoreDelete(_mutex);
}
XiaozhiCameraPreview &xiaozhiCameraPreview()
{
    static XiaozhiCameraPreview preview;
    return preview;
}
bool XiaozhiCameraPreview::active(uint32_t session) const
{
    return (session & 1U) && _session.load() == session;
}
void XiaozhiCameraPreview::setEnabled(bool enabled, uint32_t button_presses)
{
    if (!_mutex) return;
    xSemaphoreTake(_mutex, portMAX_DELAY);
    const uint32_t session = _session.load();
    if (static_cast<bool>(session & 1U) != enabled) {
        _session.store(session + 1);
        heap_caps_free(_pixels);
        _pixels = nullptr;
        _snapshot = {};
        _frozen = false;
        _reply_started = _reply_stopped = false;
        _button_presses = button_presses;
        _rotation_requested = _rotation_applied = 0;
        _rotation_owner = 0;
        _rotation_notice = _rotation_failed = false;
        ++_revision;
    }
    xSemaphoreGive(_mutex);
}
bool XiaozhiCameraPreview::observeButton(uint32_t session, uint32_t button_presses)
{
    if (!_mutex) return false;
    xSemaphoreTake(_mutex, portMAX_DELAY);
    const uint32_t presses = button_presses - _button_presses;
    // The UI and MCP worker can poll concurrently. Ignore a late, older sample;
    // unsigned subtraction also handles the BSP press counter wrapping.
    const bool changed = active(session) && presses && presses <= UINT32_MAX / 2;
    if (changed) {
        _button_presses = button_presses;
        _rotation_requested += presses;
        _rotation_notice = true;
        _rotation_failed = false;
        ++_revision;
    }
    xSemaphoreGive(_mutex);
    return changed;
}
bool XiaozhiCameraPreview::rotationRequest(uint32_t session, uint32_t &request)
{
    if (!_mutex) return false;
    xSemaphoreTake(_mutex, portMAX_DELAY);
    const bool pending = active(session) && ((_rotation_requested - _rotation_applied) & 1U);
    if (pending) {
        request = _rotation_requested;
        _rotation_owner = session;
    }
    xSemaphoreGive(_mutex);
    return pending;
}
void XiaozhiCameraPreview::finishRotation(uint32_t session, uint32_t request, bool success)
{
    if (!_mutex) return;
    xSemaphoreTake(_mutex, portMAX_DELAY);
    // Stop can dismiss a photo while the sensor I2C write is finishing. If
    // rotation succeeded, account for that physical change exactly once.
    // Closing the APP clears the owner so a new foreground session is isolated.
    if (active(session) || (success && (_session.load() & 1U) && _rotation_owner == session)) {
        if (success) _rotation_applied = request;
        _rotation_failed = !success;
        _rotation_notice = !success || ((_rotation_requested - _rotation_applied) & 1U);
        _rotation_owner = 0;
        ++_revision;
    }
    xSemaphoreGive(_mutex);
}
bool XiaozhiCameraPreview::begin(uint32_t session)
{
    if (!_mutex) return false;
    xSemaphoreTake(_mutex, portMAX_DELAY);
    bool ok = active(session);
    if (ok) {
        if (!_pixels) _pixels = static_cast<uint16_t *>(heap_caps_malloc(
            BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
        _snapshot = {Phase::Preview, false};
        _frozen = false;
        _reply_started = _reply_stopped = false;
        _rotation_notice = _rotation_failed = false;
        ++_revision;
        ok = _pixels != nullptr;
    }
    xSemaphoreGive(_mutex);
    return ok;
}
bool XiaozhiCameraPreview::publish(uint32_t session, const camera_fb_t &frame, bool final_frame)
{
    if (!_mutex || frame.format != PIXFORMAT_RGB565 || !frame.buf ||
        !frame.width || !frame.height || frame.width > SIZE_MAX / frame.height / 2 ||
        frame.len < frame.width * frame.height * 2) return false;
    xSemaphoreTake(_mutex, portMAX_DELAY);
    const bool ok = active(session) && _pixels && !_frozen;
    if (ok) {
        // Same byte order as the Camera APP: sensor bytes are big-endian,
        // while the LVGL RGB565 descriptor points at native little-endian words.
        for (unsigned y = 0; y < HEIGHT; ++y) {
            const size_t row = static_cast<size_t>(y) * frame.height / HEIGHT * frame.width;
            for (unsigned x = 0; x < WIDTH; ++x) {
                const size_t i = (row + static_cast<size_t>(x) * frame.width / WIDTH) * 2;
                _pixels[y * WIDTH + x] = (uint16_t(frame.buf[i]) << 8) | frame.buf[i + 1];
            }
        }
        _snapshot = {final_frame ? Phase::Encoding : Phase::Preview, true};
        _frozen = final_frame;
        ++_revision;
    }
    xSemaphoreGive(_mutex);
    return ok;
}
void XiaozhiCameraPreview::setPhase(uint32_t session, Phase phase)
{
    if (!_mutex) return;
    xSemaphoreTake(_mutex, portMAX_DELAY);
    if (active(session) && _snapshot.phase != phase) {
        _snapshot.phase = phase;
        _reply_started = _reply_stopped = false;
        ++_revision;
    }
    xSemaphoreGive(_mutex);
}
void XiaozhiCameraPreview::dismissLocked()
{
    // Keep the enabled bit, but reject callbacks holding the old token.
    if (_session.load() & 1U) _session.fetch_add(2);
    heap_caps_free(_pixels);
    _pixels = nullptr;
    _snapshot = {};
    _frozen = false;
    _reply_started = _reply_stopped = false;
    ++_revision;
}
void XiaozhiCameraPreview::dismiss()
{
    if (!_mutex) return;
    xSemaphoreTake(_mutex, portMAX_DELAY);
    dismissLocked();
    xSemaphoreGive(_mutex);
}
void XiaozhiCameraPreview::noteSpeech(bool stopped)
{
    if (!_mutex) return;
    xSemaphoreTake(_mutex, portMAX_DELAY);
    if (_snapshot.phase == Phase::Complete || _snapshot.phase == Phase::Error) {
        if (stopped) _reply_stopped = _reply_started;
        else {
            _reply_started = true;
            _reply_stopped = false;
        }
    }
    xSemaphoreGive(_mutex);
}
bool XiaozhiCameraPreview::finishPlayback()
{
    if (!_mutex) return false;
    xSemaphoreTake(_mutex, portMAX_DELAY);
    const bool restore = _reply_stopped &&
        (_snapshot.phase == Phase::Complete || _snapshot.phase == Phase::Error);
    if (restore) dismissLocked();
    xSemaphoreGive(_mutex);
    return restore;
}
bool XiaozhiCameraPreview::read(uint32_t &revision, uint16_t *pixels, Snapshot &snapshot)
{
    if (!_mutex || xSemaphoreTake(_mutex, 0) != pdTRUE) return false;
    const bool changed = revision != _revision;
    if (changed) {
        snapshot = _snapshot;
        snapshot.rotate_pending = (_rotation_requested - _rotation_applied) & 1U;
        snapshot.rotation_notice = _rotation_notice;
        snapshot.rotation_failed = _rotation_failed;
        if (snapshot.has_frame && pixels) memcpy(pixels, _pixels, BYTES);
        if (!snapshot.has_frame || pixels) revision = _revision;
    }
    xSemaphoreGive(_mutex);
    return changed;
}
}
