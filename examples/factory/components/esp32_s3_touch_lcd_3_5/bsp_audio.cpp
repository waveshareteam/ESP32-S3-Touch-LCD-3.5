#include "bsp/esp-bsp.h"

#include <climits>
#include <atomic>
#include "sdkconfig.h"
#include "driver/i2s_std.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#if CONFIG_CODEC_I2C_BACKWARD_COMPATIBLE
#error "This BSP uses i2c_master: disable CONFIG_CODEC_I2C_BACKWARD_COMPATIBLE."
#endif

static const char *TAG = "bsp_audio";
static i2s_chan_handle_t tx, rx;
static const audio_codec_data_if_t *data_if;
static const audio_codec_ctrl_if_t *ctrl_if;
static const audio_codec_gpio_if_t *gpio_if;
static const audio_codec_if_t *codec_if;
static esp_codec_dev_handle_t codec;
static SemaphoreHandle_t session_mutex, control_mutex;
static bool opened;
static bool voice_capture;
static std::atomic<bsp_audio_owner_t> audio_owner{BSP_AUDIO_OWNER_NONE};
static uint8_t opened_channels;
static constexpr unsigned DMA_DESCRIPTORS = 6, DMA_FRAMES = 240;
static int volume = 70;

static esp_err_t codec_result(int ret) { return ret == ESP_CODEC_DEV_OK ? ESP_OK : ESP_FAIL; }

static void cleanup(void)
{
    if (codec) { esp_codec_dev_delete(codec); codec = nullptr; }
    if (codec_if) { audio_codec_delete_codec_if(codec_if); codec_if = nullptr; }
    if (ctrl_if) { audio_codec_delete_ctrl_if(ctrl_if); ctrl_if = nullptr; }
    if (gpio_if) { audio_codec_delete_gpio_if(gpio_if); gpio_if = nullptr; }
    if (data_if) { audio_codec_delete_data_if(data_if); data_if = nullptr; }
    if (tx) { i2s_channel_disable(tx); i2s_del_channel(tx); tx = nullptr; }
    if (rx) { i2s_channel_disable(rx); i2s_del_channel(rx); rx = nullptr; }
    opened = false;
}

static esp_err_t create_audio(void)
{
    i2s_chan_config_t channel = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    channel.dma_desc_num = DMA_DESCRIPTORS;
    channel.dma_frame_num = DMA_FRAMES;
    channel.auto_clear_after_cb = true;
    ESP_RETURN_ON_ERROR(i2s_new_channel(&channel, &tx, &rx), TAG, "I2S channels");
    i2s_std_config_t cfg = {};
    cfg.clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000);
    cfg.slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
    cfg.gpio_cfg.mclk = BSP_I2S_MCLK;
    cfg.gpio_cfg.bclk = BSP_I2S_BCLK;
    cfg.gpio_cfg.ws = BSP_I2S_LRCLK;
    cfg.gpio_cfg.dout = BSP_I2S_DOUT;
    cfg.gpio_cfg.din = BSP_I2S_DIN;
    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(tx, &cfg), TAG, "I2S TX");
    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(rx, &cfg), TAG, "I2S RX");
    ESP_RETURN_ON_ERROR(i2s_channel_enable(tx), TAG, "I2S TX enable");
    ESP_RETURN_ON_ERROR(i2s_channel_enable(rx), TAG, "I2S RX enable");
    audio_codec_i2s_cfg_t data = {};
    data.port = I2S_NUM_0;
    data.tx_handle = tx;
    data.rx_handle = rx;
    data_if = audio_codec_new_i2s_data(&data);
    if (!data_if) return ESP_ERR_NO_MEM;
    audio_codec_i2c_cfg_t ctrl = {};
    ctrl.port = BSP_I2C_NUM;
    ctrl.addr = ES8311_CODEC_DEFAULT_ADDR;
    ctrl.bus_handle = bsp_i2c_get_handle();
    ctrl_if = audio_codec_new_i2c_ctrl(&ctrl);
    gpio_if = audio_codec_new_gpio();
    if (!ctrl_if || !gpio_if) return ESP_ERR_NO_MEM;
    es8311_codec_cfg_t chip = {};
    chip.codec_mode = ESP_CODEC_DEV_WORK_MODE_BOTH;
    chip.ctrl_if = ctrl_if;
    chip.gpio_if = gpio_if;
    chip.pa_pin = BSP_AUDIO_PA;
    chip.use_mclk = true;
    chip.hw_gain.pa_voltage = 5.0f;
    chip.hw_gain.codec_dac_voltage = 3.3f;
    codec_if = es8311_codec_new(&chip);
    if (!codec_if) return ESP_FAIL;
    esp_codec_dev_cfg_t dev = {};
    dev.dev_type = ESP_CODEC_DEV_TYPE_IN_OUT;
    dev.codec_if = codec_if;
    dev.data_if = data_if;
    codec = esp_codec_dev_new(&dev);
    if (!codec) return ESP_ERR_NO_MEM;
    // Verify the original 48 kHz / mono format without starting a factory recording.
    esp_codec_dev_sample_info_t fs = {};
    fs.sample_rate = 48000;
    fs.channel = 1;
    fs.bits_per_sample = 16;
    ESP_RETURN_ON_ERROR(codec_result(esp_codec_dev_open(codec, &fs)), TAG, "Codec open");
    ESP_RETURN_ON_ERROR(codec_result(esp_codec_dev_set_out_mute(codec, true)), TAG, "Codec mute");
    ESP_RETURN_ON_ERROR(codec_result(esp_codec_dev_set_out_vol(codec, volume)), TAG, "Codec volume");
    ESP_RETURN_ON_ERROR(codec_result(esp_codec_dev_set_in_gain(codec, 40.0f)), TAG, "Mic gain");
    return codec_result(esp_codec_dev_close(codec));
}

esp_err_t bsp_audio_init(void)
{
    if (codec) return ESP_OK;
    ESP_RETURN_ON_ERROR(bsp_i2c_init(), TAG, "I2C bus");
    // esp_codec_dev uses the 8-bit address; i2c_master_probe takes 7 bits.
    ESP_RETURN_ON_ERROR(i2c_master_probe(bsp_i2c_get_handle(), ES8311_CODEC_DEFAULT_ADDR >> 1, 100), TAG, "ES8311 probe");
    if (!session_mutex) session_mutex = xSemaphoreCreateMutex();
    if (!control_mutex) control_mutex = xSemaphoreCreateMutex();
    if (!session_mutex || !control_mutex) return ESP_ERR_NO_MEM;
    esp_err_t ret = create_audio();
    if (ret != ESP_OK) cleanup();
    return ret;
}

bool bsp_audio_acquire(uint32_t timeout_ms)
{
    if (!codec || !session_mutex) return false;
    const TickType_t started = xTaskGetTickCount();
    do {
        bsp_audio_owner_t expected = BSP_AUDIO_OWNER_NONE;
        if (audio_owner.compare_exchange_strong(expected, BSP_AUDIO_OWNER_APP)) {
            if (xSemaphoreTake(session_mutex, 0) == pdTRUE) return true;
            audio_owner.store(BSP_AUDIO_OWNER_NONE);
        }
        if (timeout_ms && xTaskGetTickCount() - started >= pdMS_TO_TICKS(timeout_ms)) return false;
        vTaskDelay(pdMS_TO_TICKS(10));
    } while (true);
}

static bool owns_session(void)
{
    return session_mutex && xSemaphoreGetMutexHolder(session_mutex) == xTaskGetCurrentTaskHandle();
}

void bsp_audio_release(void)
{
    if (!owns_session()) return;
    bsp_audio_close();
    xSemaphoreGive(session_mutex);
    audio_owner.store(BSP_AUDIO_OWNER_NONE);
}

static esp_err_t open_codec(uint32_t sample_rate, uint8_t channels)
{
    if (!codec) return ESP_ERR_INVALID_STATE;
    if (sample_rate < 8000 || sample_rate > 96000 || (channels != 1 && channels != 2)) return ESP_ERR_INVALID_ARG;
    xSemaphoreTake(control_mutex, portMAX_DELAY);
    if (opened) {
        esp_err_t ret = codec_result(esp_codec_dev_close(codec));
        if (ret != ESP_OK) { xSemaphoreGive(control_mutex); return ret; }
        opened = false;
    }
    esp_codec_dev_sample_info_t fs = {};
    fs.sample_rate = sample_rate;
    fs.channel = channels;
    fs.bits_per_sample = 16;
    esp_err_t ret = codec_result(esp_codec_dev_open(codec, &fs));
    if (ret == ESP_OK) ret = codec_result(esp_codec_dev_set_out_vol(codec, volume));
    if (ret == ESP_OK) ret = codec_result(esp_codec_dev_set_in_gain(codec, 40.0f));
    if (ret == ESP_OK) ret = codec_result(esp_codec_dev_set_out_mute(codec, false));
    if (ret != ESP_OK) esp_codec_dev_close(codec);
    opened = ret == ESP_OK;
    opened_channels = opened ? channels : 0;
    xSemaphoreGive(control_mutex);
    return ret;
}

esp_err_t bsp_audio_open(uint32_t sample_rate, uint8_t channels)
{
    return owns_session() ? open_codec(sample_rate, channels) : ESP_ERR_INVALID_STATE;
}

static esp_err_t close_codec(void)
{
    if (!codec) return ESP_ERR_INVALID_STATE;
    xSemaphoreTake(control_mutex, portMAX_DELAY);
    esp_err_t ret = ESP_OK;
    if (opened) {
        esp_codec_dev_set_out_mute(codec, true);
        ret = codec_result(esp_codec_dev_close(codec));
        if (ret == ESP_OK) opened = false;
    }
    xSemaphoreGive(control_mutex);
    return ret;
}

esp_err_t bsp_audio_close(void)
{
    return owns_session() ? close_codec() : ESP_ERR_INVALID_STATE;
}

esp_err_t bsp_audio_write(const void *data, size_t size)
{
    if (!owns_session() || !opened) return ESP_ERR_INVALID_STATE;
    if (!data || !size || size > INT_MAX) return ESP_ERR_INVALID_ARG;
    return codec_result(esp_codec_dev_write(codec, const_cast<void *>(data), (int)size));
}

esp_err_t bsp_audio_read(void *data, size_t size)
{
    if (!owns_session() || !opened) return ESP_ERR_INVALID_STATE;
    if (!data || !size || size > INT_MAX) return ESP_ERR_INVALID_ARG;
    return codec_result(esp_codec_dev_read(codec, data, (int)size));
}

esp_err_t bsp_audio_drain(void)
{
    if (!owns_session() || !opened) return ESP_ERR_INVALID_STATE;
    const int16_t silence[256] = {};
    // One complete ring plus one write ensures previously queued PCM has left TX.
    size_t remaining = DMA_DESCRIPTORS * DMA_FRAMES * opened_channels * sizeof(int16_t) + sizeof(silence);
    while (remaining) {
        size_t count = remaining < sizeof(silence) ? remaining : sizeof(silence);
        ESP_RETURN_ON_ERROR(bsp_audio_write(silence, count), TAG, "Drain audio");
        remaining -= count;
    }
    return ESP_OK;
}

esp_err_t bsp_audio_volume_set(int percent)
{
    if (percent < 0 || percent > 100) return ESP_ERR_INVALID_ARG;
    if (!control_mutex || !codec) return ESP_ERR_INVALID_STATE;
    xSemaphoreTake(control_mutex, portMAX_DELAY);
    esp_err_t ret = opened ? codec_result(esp_codec_dev_set_out_vol(codec, percent)) : ESP_OK;
    if (ret == ESP_OK) volume = percent;
    xSemaphoreGive(control_mutex);
    return ret;
}

int bsp_audio_volume_get(void)
{
    if (!control_mutex) return volume;
    xSemaphoreTake(control_mutex, portMAX_DELAY);
    int result = volume;
    xSemaphoreGive(control_mutex);
    return result;
}

bsp_audio_owner_t bsp_audio_session_get_owner(void) { return audio_owner.load(); }
const char *bsp_audio_owner_name(bsp_audio_owner_t owner)
{
    return owner == BSP_AUDIO_OWNER_XIAOZHI ? "Xiaozhi" : owner == BSP_AUDIO_OWNER_APP ? "Music/Mic Test" : "None";
}
esp_err_t bsp_audio_session_acquire(bsp_audio_owner_t owner)
{
    if (!codec) return ESP_ERR_INVALID_STATE;
    if (owner != BSP_AUDIO_OWNER_XIAOZHI) return ESP_ERR_INVALID_ARG;
    bsp_audio_owner_t expected = BSP_AUDIO_OWNER_NONE;
    return audio_owner.compare_exchange_strong(expected, owner) ? ESP_OK : ESP_ERR_INVALID_STATE;
}
esp_err_t bsp_audio_session_release(bsp_audio_owner_t owner)
{
    if (owner != BSP_AUDIO_OWNER_XIAOZHI) return ESP_ERR_INVALID_ARG;
    if (opened) return ESP_ERR_INVALID_STATE;
    return audio_owner.compare_exchange_strong(owner, BSP_AUDIO_OWNER_NONE) ? ESP_OK : ESP_ERR_INVALID_STATE;
}
esp_err_t bsp_audio_voice_open(uint32_t sample_rate, bool capture)
{
    if (audio_owner.load() != BSP_AUDIO_OWNER_XIAOZHI || opened) return ESP_ERR_INVALID_STATE;
    esp_err_t ret = open_codec(sample_rate, 2);
    voice_capture = ret == ESP_OK && capture;
    if (ret == ESP_OK) ESP_LOGI(TAG, "Xiaozhi ES8311: %lu Hz, single microphone, no hardware echo reference", (unsigned long)sample_rate);
    return ret;
}
esp_err_t bsp_audio_voice_read(void *pcm, size_t bytes, size_t *read, uint32_t timeout_ms)
{
    if (read) *read = 0;
    if (!pcm || !read || !bytes || bytes % 4) return ESP_ERR_INVALID_ARG;
    if (audio_owner.load() != BSP_AUDIO_OWNER_XIAOZHI || !opened || !voice_capture) return ESP_ERR_INVALID_STATE;
    return i2s_channel_read(rx, pcm, bytes, read, timeout_ms);
}
esp_err_t bsp_audio_voice_write(void *pcm, size_t bytes, size_t *written, uint32_t timeout_ms)
{
    if (written) *written = 0;
    if (!pcm || !written || !bytes || bytes % 4) return ESP_ERR_INVALID_ARG;
    if (audio_owner.load() != BSP_AUDIO_OWNER_XIAOZHI || !opened) return ESP_ERR_INVALID_STATE;
    return i2s_channel_write(tx, pcm, bytes, written, timeout_ms);
}
esp_err_t bsp_audio_voice_mute(bool mute)
{
    if (audio_owner.load() != BSP_AUDIO_OWNER_XIAOZHI || !opened) return ESP_ERR_INVALID_STATE;
    xSemaphoreTake(control_mutex, portMAX_DELAY);
    esp_err_t ret = codec_result(esp_codec_dev_set_out_mute(codec, mute));
    xSemaphoreGive(control_mutex);
    return ret;
}
esp_err_t bsp_audio_voice_close(void)
{
    if (audio_owner.load() != BSP_AUDIO_OWNER_XIAOZHI) return ESP_ERR_INVALID_STATE;
    esp_err_t ret = close_codec();
    if (ret == ESP_OK) voice_capture = false;
    return ret;
}
