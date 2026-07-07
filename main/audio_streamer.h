#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

esp_err_t audio_streamer_start(void);
void audio_streamer_stop(void);
bool audio_streamer_is_active(void);

/*
 * Copies interleaved PCM16 samples into an owned queue item before returning.
 * sample_count is the total sample count, not per-channel frame count.
 */
esp_err_t audio_streamer_submit_pcm(const int16_t *samples,
                                    size_t sample_count,
                                    uint32_t sample_rate,
                                    uint8_t channels);
