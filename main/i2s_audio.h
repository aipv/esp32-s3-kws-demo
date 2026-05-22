#ifndef I2S_AUDIO_H
#define I2S_AUDIO_H

#include <stdio.h>
#include "esp_log.h"
#include "driver/i2s_std.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define I2S_AUDIO_MIC_GPIO_WS       GPIO_NUM_4
#define I2S_AUDIO_MIC_GPIO_SCK      GPIO_NUM_5
#define I2S_AUDIO_MIC_GPIO_DIN      GPIO_NUM_6
#define I2S_AUDIO_SPK_GPIO_DOUT     GPIO_NUM_7
#define I2S_AUDIO_SPK_GPIO_BCLK     GPIO_NUM_15
#define I2S_AUDIO_SPK_GPIO_LRCK     GPIO_NUM_16

#define I2S_AUDIO_MIC_SAMPLE_RATE   16000
#define I2S_AUDIO_SPK_SAMPLE_RATE   16000

#define I2S_AUDIO_BUFFER_SAMPLES    1024
#define I2S_AUDIO_BUFFER_SIZE       4096
#define I2S_AUDIO_PCM16_SIZE    	2048

esp_err_t i2s_audio_mic_init(void);
esp_err_t i2s_audio_spk_init(void);
esp_err_t i2s_audio_convert_data(int32_t *input, int16_t *output, int samples);
esp_err_t i2s_audio_read_data(int32_t *buffer, int samples);
esp_err_t i2s_audio_play_data(int32_t *buffer, int samples);
esp_err_t i2s_audio_stream_data(int pcm16_flag);
esp_err_t i2s_audio_stop_stream();

#endif // I2S_AUDIO_H