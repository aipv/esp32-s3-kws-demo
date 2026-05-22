#include <stdio.h>
#include "esp_log.h"
#include "driver/i2s_std.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2s_audio.h"

static const char *TAG = "I2S_AUDIO";

static i2s_chan_handle_t rx_handle = NULL;
static i2s_chan_handle_t tx_handle = NULL;

static int16_t  i2s_audio_pcm16_buffer[I2S_AUDIO_BUFFER_SAMPLES];
static int32_t  i2s_audio_raw_buffer[I2S_AUDIO_BUFFER_SAMPLES];
static int32_t  i2s_audio_data_stream_flag = false;
static int32_t  i2s_audio_data_convert_flag = false;
static TaskHandle_t i2s_audio_stream_task_handle = NULL;

// ================== 错误检查 ==================
static void check_esp_err(esp_err_t err, const char* msg)
{
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "%s failed: %s (0x%x)", msg, esp_err_to_name(err), err);
        abort();
    }
}

esp_err_t i2s_audio_mic_init()
{
    i2s_chan_config_t chan_cfg = {
        .id = (i2s_port_t)0,
        .role = I2S_ROLE_MASTER,
        .dma_desc_num = 8,
        .dma_frame_num = 256,
        .auto_clear_after_cb = true,
        .auto_clear_before_cb = false,
        .intr_priority = 0,
    };
    check_esp_err(i2s_new_channel(&chan_cfg, NULL, &rx_handle), "i2s_new_channel_rx");

    i2s_std_config_t std_cfg = {
        .clk_cfg = {
            .sample_rate_hz = (uint32_t)I2S_AUDIO_MIC_SAMPLE_RATE,
            .clk_src = I2S_CLK_SRC_DEFAULT,
            .mclk_multiple = I2S_MCLK_MULTIPLE_256,
            #ifdef   I2S_HW_VERSION_2
                .ext_clk_freq_hz = 0,
            #endif
        },
        .slot_cfg = {
            .data_bit_width = I2S_DATA_BIT_WIDTH_32BIT,
            .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO,
            .slot_mode = I2S_SLOT_MODE_MONO,
            .slot_mask = I2S_STD_SLOT_LEFT,
            .ws_width = I2S_DATA_BIT_WIDTH_32BIT,
            .ws_pol = false,
            .bit_shift = true,
            #ifdef   I2S_HW_VERSION_2
                .left_align = true,
                .big_endian = false,
                .bit_order_lsb = false
            #endif
        },
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_AUDIO_MIC_GPIO_SCK,
            .ws   = I2S_AUDIO_MIC_GPIO_WS,
            .dout = I2S_GPIO_UNUSED,
            .din  = I2S_AUDIO_MIC_GPIO_DIN,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false
            }
        },
    };
    check_esp_err(i2s_channel_init_std_mode(rx_handle, &std_cfg), "i2s_channel_init_std_mode_rx");
    check_esp_err(i2s_channel_enable(rx_handle), "i2s_channel_enable_rx");
    ESP_LOGI(TAG, "i2s_audio_mic_init() Success!");
    return ESP_OK;
}

esp_err_t i2s_audio_spk_init()
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_1, I2S_ROLE_MASTER);
    check_esp_err(i2s_new_channel(&chan_cfg, &tx_handle, NULL), "i2s_new_channel_tx");

    i2s_std_config_t std_cfg = {
        .clk_cfg = {
            .sample_rate_hz = (uint32_t)I2S_AUDIO_SPK_SAMPLE_RATE,
            .clk_src = I2S_CLK_SRC_DEFAULT,
            .mclk_multiple = I2S_MCLK_MULTIPLE_256,
            #ifdef   I2S_HW_VERSION_2
                .ext_clk_freq_hz = 0,
            #endif
        },
        .slot_cfg = {
            .data_bit_width = I2S_DATA_BIT_WIDTH_32BIT,
            .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO,
            .slot_mode = I2S_SLOT_MODE_MONO,
            .slot_mask = I2S_STD_SLOT_LEFT,
            .ws_width = I2S_DATA_BIT_WIDTH_32BIT,
            .ws_pol = false,
            .bit_shift = true,
            #ifdef   I2S_HW_VERSION_2
                .left_align = true,
                .big_endian = false,
                .bit_order_lsb = false
            #endif
        },
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_AUDIO_SPK_GPIO_BCLK,
            .ws = I2S_AUDIO_SPK_GPIO_LRCK,
            .dout = I2S_AUDIO_SPK_GPIO_DOUT,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false
            }
        },
    };

    check_esp_err(i2s_channel_init_std_mode(tx_handle, &std_cfg), "i2s_channel_init_std_mode_tx");
    ESP_LOGI(TAG, "i2s_audio_spk_init() Success!");
    return ESP_OK;
}

esp_err_t i2s_audio_convert_data(int32_t *input, int16_t *output, int samples)
{
    for (int i = 0; i < samples; i++)
    {
        int32_t value = input[i] >> 12;
        output[i] = (value > INT16_MAX) ? INT16_MAX : (value < -INT16_MAX) ? -INT16_MAX : (int16_t)value;
    }
    return ESP_OK;
}

esp_err_t i2s_audio_read_data(int32_t *buffer, int samples)
{
    size_t bytes_read = 0;
    size_t bytes_to_read = (size_t)samples * sizeof(int32_t);

    check_esp_err(i2s_channel_read(rx_handle, buffer, bytes_to_read, &bytes_read, pdMS_TO_TICKS(1000)), "i2s_channel_read");
    if (bytes_read != bytes_to_read)
    {
        ESP_LOGW(TAG, "Read data: expected %u bytes, got %u bytes", bytes_to_read, bytes_read);
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t i2s_audio_play_data(int32_t *buffer, int samples)
{
    size_t bytes_written = 0;
    size_t size_bytes = (size_t)samples * sizeof(int32_t);

    check_esp_err(i2s_channel_enable(tx_handle), "i2s_channel_enable_tx");
    check_esp_err(i2s_channel_write(tx_handle, (const void *)buffer, size_bytes, &bytes_written, pdMS_TO_TICKS(1000)), "i2s_channel_write");
    if (bytes_written != size_bytes)
    {
        ESP_LOGW(TAG, "Write data: Wrote %u bytes, expected %u bytes.", bytes_written, size_bytes);
        return ESP_FAIL;
    }
    check_esp_err(i2s_channel_disable(tx_handle), "i2s_channel_disable_tx");
    return ESP_OK;
}
