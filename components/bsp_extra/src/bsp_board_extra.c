/*
 * SPDX-FileCopyrightText: 2015-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bsp/esp-bsp.h"
#include "bsp_board_extra.h"
#include "esp_codec_dev_defaults.h"

static esp_codec_dev_handle_t record_dev_handle;
static bool audio_initialized;

static esp_err_t codec_set_format(uint32_t sample_rate, uint32_t bits_per_sample, uint32_t channels)
{
    esp_codec_dev_sample_info_t format = {
        .sample_rate = sample_rate,
        .channel = channels,
        .bits_per_sample = bits_per_sample,
    };

    esp_err_t ret = esp_codec_dev_close(record_dev_handle);
    if (ret != ESP_OK)
    {
        return ret;
    }

    ret = esp_codec_dev_set_in_gain(record_dev_handle, CODEC_DEFAULT_ADC_VOLUME);
    if (ret != ESP_OK)
    {
        return ret;
    }

    return esp_codec_dev_open(record_dev_handle, &format);
}

esp_err_t bsp_extra_i2s_read(void *audio_buffer, size_t len, size_t *bytes_read, uint32_t timeout_ms)
{
    (void)timeout_ms;

    esp_err_t ret = esp_codec_dev_read(record_dev_handle, audio_buffer, len);
    if (bytes_read != NULL)
    {
        *bytes_read = ret == ESP_OK ? len : 0;
    }
    return ret;
}

esp_err_t bsp_extra_codec_init(void)
{
    if (audio_initialized)
    {
        return ESP_OK;
    }

    record_dev_handle = bsp_audio_codec_microphone_init();
    assert(record_dev_handle && "record_dev_handle not initialized");

    esp_err_t ret = codec_set_format(
        CODEC_DEFAULT_SAMPLE_RATE,
        CODEC_DEFAULT_BIT_WIDTH,
        CODEC_DEFAULT_CHANNEL);
    if (ret == ESP_OK)
    {
        audio_initialized = true;
    }
    return ret;
}
