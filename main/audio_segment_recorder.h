#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

typedef struct {
    uint32_t segments_started;
    uint32_t segments_finalized;
    uint32_t segments_enqueued;
    uint32_t segments_uploaded;
    uint32_t upload_retry_count;
    uint32_t upload_failure_count;
    uint32_t local_gap_count;
    uint32_t storage_overflow_count;
    uint32_t queue_overflow_count;
    uint32_t last_segment_index;
    char run_id[96];
} audio_segment_recorder_stats_t;

esp_err_t audio_segment_recorder_start(void);
esp_err_t audio_segment_recorder_start_pending_uploads(void);
esp_err_t audio_segment_recorder_write_pcm(const void *data, size_t len);
esp_err_t audio_segment_recorder_stop(void);
void audio_segment_recorder_abort(void);
bool audio_segment_recorder_is_active(void);
bool audio_segment_recorder_upload_enabled(void);
void audio_segment_recorder_get_stats(audio_segment_recorder_stats_t *stats);
