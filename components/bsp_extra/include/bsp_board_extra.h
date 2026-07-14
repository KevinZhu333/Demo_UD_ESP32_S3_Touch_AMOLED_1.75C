/*
 * SPDX-FileCopyrightText: 2015-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CODEC_DEFAULT_SAMPLE_RATE (16000)
#define CODEC_DEFAULT_BIT_WIDTH (16)
#define CODEC_DEFAULT_ADC_VOLUME (24.0)
#define CODEC_DEFAULT_CHANNEL (2)

/** Initialize the board microphone codec for PCM capture. */
esp_err_t bsp_extra_codec_init(void);

/** Read PCM bytes from the board microphone codec. */
esp_err_t bsp_extra_i2s_read(
    void *audio_buffer,
    size_t len,
    size_t *bytes_read,
    uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif
