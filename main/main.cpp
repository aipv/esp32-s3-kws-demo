#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "i2s_audio.h"

#include "edge-impulse-sdk/classifier/ei_run_classifier.h"

#define EI_BUFFER_SIZE 16000
#define EI_THRESHOLD   0.85f

static const char *TAG = "KWS";

static int16_t inference_buffer[EI_BUFFER_SIZE];

static int get_signal_data(
    size_t offset,
    size_t length,
    float *out_ptr
)
{
    for (size_t i = 0; i < length; i++)
    {
        out_ptr[i] =
            (float)inference_buffer[offset + i];
    }

    return 0;
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Init I2S microphone");

    i2s_audio_mic_init();

    /*
     * 每次从 I2S 读取 512 samples
     */
    int samples_per_read = 512;

    int32_t *raw_buffer = (int32_t *)malloc(
        samples_per_read * sizeof(int32_t)
    );

    int16_t *pcm_buffer = (int16_t *)malloc(
        samples_per_read * sizeof(int16_t)
    );

    if (!raw_buffer || !pcm_buffer)
    {
        ESP_LOGE(TAG, "Buffer malloc failed");
        return;
    }

    ESP_LOGI(TAG, "Edge Impulse KWS started");

    /*
     * temporal smoothing
     */
    static int hit_count = 0;

    while (1)
    {
        /*
         * 采满 1 秒音频
         * 16000 samples
         */
        int index = 0;

        while (index < EI_BUFFER_SIZE)
        {
            if (i2s_audio_read_data(
                    raw_buffer,
                    samples_per_read
                ) != ESP_OK)
            {
                continue;
            }

            /*
             * int32 -> int16
             */
            i2s_audio_convert_data(
                raw_buffer,
                pcm_buffer,
                samples_per_read
            );

            int copy_samples = samples_per_read;

            if ((index + copy_samples) > EI_BUFFER_SIZE)
            {
                copy_samples =
                    EI_BUFFER_SIZE - index;
            }

            memcpy(
                &inference_buffer[index],
                pcm_buffer,
                copy_samples * sizeof(int16_t)
            );

            index += copy_samples;
        }

        /*
         * Edge Impulse signal
         */
        signal_t signal;

        signal.total_length = EI_BUFFER_SIZE;
        signal.get_data = &get_signal_data;

        /*
         * inference result
         */
        ei_impulse_result_t result = {0};

        EI_IMPULSE_ERROR err =
            run_classifier(
                &signal,
                &result,
                false
            );

        if (err != EI_IMPULSE_OK)
        {
            ESP_LOGE(
                TAG,
                "run_classifier failed (%d)",
                err
            );

            continue;
        }

        float wakeword_score = 0.0f;

        /*
         * 打印所有分类结果
         */
        for (size_t ix = 0;
             ix < EI_CLASSIFIER_LABEL_COUNT;
             ix++)
        {
            ESP_LOGI(
                TAG,
                "%s = %.3f",
                result.classification[ix].label,
                result.classification[ix].value
            );

            /*
             * 找到 wakeword label
             */
            if (strcmp(
                    result.classification[ix].label,
                    "wakeword"
                ) == 0)
            {
                wakeword_score =
                    result.classification[ix].value;
            }
        }

        /*
         * threshold
         */
        if (wakeword_score > EI_THRESHOLD)
        {
            hit_count++;

            ESP_LOGI(
                TAG,
                "hit_count=%d score=%.3f",
                hit_count,
                wakeword_score
            );
        }
        else
        {
            hit_count = 0;
        }

        /*
         * temporal smoothing
         * 连续 3 次命中才触发
         */
        if (hit_count >= 3)
        {
            ESP_LOGI(
                TAG,
                "Wakeword Detected!"
            );

            hit_count = 0;
        }
    }
}