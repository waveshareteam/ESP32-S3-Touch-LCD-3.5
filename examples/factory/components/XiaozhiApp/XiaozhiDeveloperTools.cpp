/* SPDX-License-Identifier: Apache-2.0 */
#include "XiaozhiDeveloperTools.hpp"
#include "XiaozhiCameraPreview.hpp"
#include "bsp/esp-bsp.h"
#include "esp_mcp_engine.h"
#include "esp_xiaozhi_camera.h"
#include "esp_xiaozhi_mcp_observer.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "cJSON.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace esp_brookesia::apps {
namespace {
// Accessed only on the transport's dedicated MCP worker.
std::string vision_url, vision_token;
constexpr char TAG[]="xiaozhi_camera";

void observeMcp(const char *payload, size_t length) {
    cJSON *root=cJSON_ParseWithLength(payload,length);
    if(!root) return;
    auto *method=cJSON_GetObjectItemCaseSensitive(root,"method");
    if(cJSON_IsString(method) && !strcmp(method->valuestring,"initialize")) {
        vision_url.clear(); vision_token.clear();
        auto *params=cJSON_GetObjectItemCaseSensitive(root,"params");
        auto *caps=cJSON_GetObjectItemCaseSensitive(params,"capabilities");
        auto *vision=cJSON_GetObjectItemCaseSensitive(caps,"vision");
        auto *url=cJSON_GetObjectItemCaseSensitive(vision,"url");
        auto *token=cJSON_GetObjectItemCaseSensitive(vision,"token");
        if(cJSON_IsString(url) && strlen(url->valuestring)<512 &&
           (!strncmp(url->valuestring,"https://",8) || !strncmp(url->valuestring,"http://",7)) &&
           !strpbrk(url->valuestring,"\r\n") &&
           (!cJSON_IsString(token) || (strlen(token->valuestring)<1024 && !strpbrk(token->valuestring,"\r\n")))) {
            vision_url=url->valuestring;
            if(cJSON_IsString(token)) vision_token=token->valuestring;
        }
        if(vision_url.empty()) ESP_LOGW(TAG,"Vision service not supplied or invalid");
        else ESP_LOGI(TAG,"Vision service ready (%s)",vision_url.compare(0,8,"https://")==0?"HTTPS":"HTTP");
    }
    cJSON_Delete(root);
}
esp_err_t failure(esp_mcp_tool_result_t *result,const char *text) {
    ESP_LOGW(TAG,"%s",text);
    esp_mcp_tool_result_set_is_error(result,true);
    return esp_mcp_tool_result_add_text(result,text);
}
bool requestCancelled(void *context) {
    const auto session = *static_cast<const uint32_t *>(context);
    return !(session & 1) || xiaozhiCameraPreview().session() != session;
}
bool showCameraFrame(const camera_fb_t *frame, bool final_frame, void *context) {
    if(requestCancelled(context)) return false;
    if(frame && xiaozhiCameraPreview().publish(*static_cast<const uint32_t *>(context), *frame, final_frame) && final_frame)
        ESP_LOGI(TAG,"Preview frozen: 128x192, same source frame as uploaded JPEG");
    return !requestCancelled(context);
}
esp_err_t applyCameraRotation(uint32_t session, uint32_t request) {
    if(requestCancelled(&session)) return ESP_ERR_INVALID_STATE;
    esp_err_t ret=bsp_camera_init();
    if(ret==ESP_OK) {
        if(!bsp_camera_acquire(1000)) ret=ESP_ERR_NOT_FINISHED;
        else {
            if(requestCancelled(&session)) ret=ESP_ERR_INVALID_STATE;
            else {
                // As in Camera::work(), hold the completed framebuffer while
                // changing both axes, preventing a half-rotated DMA frame.
                auto *frame=bsp_camera_get_frame();
                if(!frame) ret=ESP_ERR_TIMEOUT;
                else {
                    ret=requestCancelled(&session)?ESP_ERR_INVALID_STATE:bsp_camera_rotate_180();
                    bsp_camera_return_frame(frame);
                }
            }
            bsp_camera_release();
        }
    }
    // Acknowledge the sampled request only. A BOOT press during this operation
    // remains queued for the following photo, even if the current upload fails.
    xiaozhiCameraPreview().finishRotation(session,request,ret==ESP_OK);
    if(ret==ESP_OK) ESP_LOGI(TAG,"BOOT rotation applied before capture: 180 degrees");
    else ESP_LOGW(TAG,"BOOT rotation failed: %s",esp_err_to_name(ret));
    return ret;
}
esp_err_t takePhoto(const esp_mcp_property_list_t *props,esp_mcp_tool_result_t *result) {
    auto &preview = xiaozhiCameraPreview();
    uint32_t session = preview.session();
    ESP_LOGI(TAG,"MCP self.camera.take_photo requested");
    if(requestCancelled(&session)) return failure(result,"Open AIChats before using the camera.");
    if(vision_url.empty()) return failure(result,"The server did not supply a valid HTTP or HTTPS vision service. Check the agent's vision configuration.");
    const char *question=esp_mcp_property_list_get_property_string(props,"question");
    if(!question || !question[0] || strlen(question)>1024) return failure(result,"A question of 1 to 1024 bytes is required.");
    preview.observeButton(session,bsp_user_button_get_press_count());
    uint32_t rotation_request=0;
    const bool rotate=preview.rotationRequest(session,rotation_request);
    if(!preview.begin(session)) ESP_LOGW(TAG,"Preview buffer unavailable; photo recognition remains available");
    else ESP_LOGI(TAG,"Local camera preview started: 128x192, at most 5 fps");
    uint8_t *jpeg=nullptr; size_t size=0;
    esp_err_t ret=rotate?applyCameraRotation(session,rotation_request):ESP_OK;
    if(ret!=ESP_OK) {
        preview.setPhase(session,XiaozhiCameraPreview::Phase::Error);
        if(requestCancelled(&session)) return failure(result,"Camera request cancelled because the conversation was stopped or AIChats was closed.");
        return failure(result,"The requested BOOT camera rotation could not be applied. Retry taking the photo; the rotation request is still pending.");
    }
    ret=bsp_camera_capture_jpeg_with_preview(&jpeg,&size,showCameraFrame,&session);
    if(ret!=ESP_OK) {
        preview.setPhase(session, XiaozhiCameraPreview::Phase::Error);
        free(jpeg); ESP_LOGW(TAG,"Capture failed: %s",esp_err_to_name(ret));
        if(requestCancelled(&session)) return failure(result,"Camera request cancelled because the conversation was stopped or AIChats was closed.");
        if(ret==ESP_ERR_NOT_FOUND) return failure(result,"BSP retried initialization but could not detect the camera sensor. Check the camera probe and power logs.");
        if(ret==ESP_ERR_NOT_FINISHED) return failure(result,"Another camera operation is in progress. Close the Camera APP and retry in AIChats.");
        if(ret==ESP_ERR_INVALID_STATE) return failure(result,"Camera initialization is not ready. Check the BSP power, expander and camera initialization logs.");
        if(ret==ESP_ERR_TIMEOUT) return failure(result,"The camera did not deliver a fresh frame. Capture timed out.");
        if(ret==ESP_ERR_NO_MEM) return failure(result,"Not enough memory to prepare the camera image.");
        return failure(result,"The camera frame could not be converted to a complete JPEG. Check the BSP AI capture log.");
    }
    if(requestCancelled(&session)) { free(jpeg); return failure(result,"Camera request cancelled because the conversation was stopped or AIChats was closed."); }
    esp_xiaozhi_camera_handle_t *client=nullptr;
    esp_xiaozhi_camera_config_t config={vision_url.c_str(),vision_token.c_str()};
    ret=esp_xiaozhi_camera_create(&config,&client);
    // Keep the response below 8 KiB so worst-case JSON escaping still fits
    // the MCP transport's uint16_t message length.
    constexpr size_t response_capacity=8192;
    char *response=static_cast<char *>(heap_caps_calloc(1,response_capacity,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
    size_t response_len=0;
    const char *phase="prepare"; int http_status=0;
    if(ret==ESP_OK && !response) ret=ESP_ERR_NO_MEM;
    if(ret==ESP_OK) {
        esp_xiaozhi_camera_set_cancel_callback(client,requestCancelled,&session);
        esp_xiaozhi_camera_frame_t frame={jpeg,size};
        preview.setPhase(session, XiaozhiCameraPreview::Phase::Analyzing);
        ESP_LOGI(TAG,"Explaining current camera frame: %u JPEG bytes",static_cast<unsigned>(size));
        ret=esp_xiaozhi_camera_explain(client,&frame,question,response,response_capacity,&response_len);
        esp_xiaozhi_camera_get_last_status(client,&phase,&http_status);
    }
    free(jpeg);
    if(client) esp_xiaozhi_camera_destroy(client);
    esp_err_t added;
    if(ret==ESP_OK && response_len && !requestCancelled(&session)) {
        preview.setPhase(session, XiaozhiCameraPreview::Phase::Complete);
        ESP_LOGI(TAG,"Vision response received: %u bytes",static_cast<unsigned>(response_len));
        added=esp_mcp_tool_result_add_text(result,response);
    }
    else {
        preview.setPhase(session, XiaozhiCameraPreview::Phase::Error);
        const char *reason="The vision service request failed. No image analysis result is available.";
        if(requestCancelled(&session)) reason="Camera request cancelled because the conversation was stopped or AIChats was closed or reopened.";
        else if(ret==ESP_ERR_NO_MEM) reason="Not enough memory for the vision request.";
        else if(ret==ESP_ERR_TIMEOUT) reason="The photo was captured, but the vision request exceeded its time limit.";
        else if(http_status==401 || http_status==403) reason="The photo was captured, but the vision service rejected authorization. Check the agent's vision service credentials.";
        else if(http_status==404) reason="The photo was captured, but the server's vision endpoint was not found.";
        else if(http_status>=300 && http_status<400) reason="The vision endpoint redirected the request. Configure its final URL on the server.";
        else if(http_status==429) reason="The vision service is rate limited. Retry later.";
        else if(http_status>=500) reason="The photo was captured, but the vision server returned an internal error.";
        else if(!strcmp(phase,"connect")) reason="The photo was captured, but connecting to the vision service failed. Check network/DNS/TLS.";
        else if(!strcmp(phase,"upload")) reason="The photo was captured, but uploading the JPEG failed.";
        else if(ret==ESP_ERR_INVALID_SIZE) reason="The vision response is too large for the MCP result buffer.";
        char message[384];
        snprintf(message,sizeof(message),"%s (phase=%s, HTTP=%d, error=%s)",reason,phase,http_status,esp_err_to_name(ret));
        added=failure(result,message);
    }
    free(response);
    return added;
}
}
void setXiaozhiCameraEnabled(bool enabled) {
    xiaozhiCameraPreview().setEnabled(enabled,bsp_user_button_get_press_count());
}
void pollXiaozhiCameraButton() {
    auto &preview=xiaozhiCameraPreview();
    const uint32_t session=preview.session();
    if(preview.observeButton(session,bsp_user_button_get_press_count()))
        ESP_LOGI(TAG,"BOOT pressed: next-photo orientation updated");
}
esp_err_t registerXiaozhiDeveloperTools(esp_mcp_t *mcp) {
    if(!mcp) return ESP_ERR_INVALID_ARG;
    esp_xiaozhi_mcp_set_observer(observeMcp);
    auto *tool=esp_mcp_tool_create_ex("self.camera.take_photo","Camera",
        "Take a fresh photo and explain it. Use when the user asks what the camera sees, to identify an object, or to read visible text. Requires a question.",takePhoto);
    if(!tool) return ESP_ERR_NO_MEM;
    auto *question=esp_mcp_property_create("question",ESP_MCP_PROPERTY_TYPE_STRING);
    esp_err_t ret=question?esp_mcp_tool_add_property(tool,question):ESP_ERR_NO_MEM;
    if(ret!=ESP_OK && question) esp_mcp_property_destroy(question);
    if(ret==ESP_OK) ret=esp_mcp_add_tool(mcp,tool);
    if(ret!=ESP_OK) esp_mcp_tool_destroy(tool);
    else ESP_LOGI(TAG,"Registered MCP tool: self.camera.take_photo");
    return ret;
}
}
