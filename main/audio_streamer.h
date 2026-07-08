#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

typedef struct {
    uint32_t submitted_count;
    uint32_t queued_count;
    uint32_t built_count;
    uint32_t buffered_count;
    uint32_t audio_loss_count;
    uint32_t offline_overflow_count;
    uint32_t capture_gap_count;
    uint32_t last_seq;
} audio_streamer_stats_t;

esp_err_t audio_streamer_start(void);
void audio_streamer_stop(void);
bool audio_streamer_is_active(void);
void audio_streamer_get_stats(audio_streamer_stats_t *stats);

/*
 * Copies interleaved PCM16 samples into an owned queue item before returning.
 * sample_count is the total sample count, not per-channel frame count.
 * Returns quickly from the capture path; any error is proof-failing loss
 * accounting rather than permission to block I2S capture.
 */
esp_err_t audio_streamer_submit_pcm(const int16_t *samples,
                                    size_t sample_count,
                                    uint32_t sample_rate,
                                    uint8_t channels);
