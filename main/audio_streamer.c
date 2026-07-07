#include "audio_streamer.h"

#include <inttypes.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "cloud_ws.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "offline_buffer.h"
#include "protocol.h"
#include "runtime_config.h"

#define AUDIO_STREAMER_QUEUE_LEN 4
#define AUDIO_STREAMER_MAX_PCM_BYTES 4096
#define AUDIO_STREAMER_TASK_STACK_SIZE 4096
#define AUDIO_STREAMER_TASK_PRIORITY 4
#define AUDIO_STREAMER_DRAIN_INTERVAL_MS 100
#define AUDIO_STREAMER_BUFFERED_RETRY_MS 1000

typedef struct {
    int16_t *samples;
    size_t sample_count;
    uint32_t sample_rate;
    uint8_t channels;
} audio_streamer_frame_t;

typedef struct {
    uint8_t *data;
    size_t len;
    uint32_t seq;
} audio_streamer_protocol_frame_t;

static const char *TAG = "audio_streamer";

static QueueHandle_t audio_streamer_queue;
static TaskHandle_t audio_streamer_task_handle;
static bool audio_streamer_active;
static uint32_t audio_streamer_next_seq;

static void write_u16_le(uint8_t *dst, uint16_t value)
{
    dst[0] = (uint8_t)(value & 0xff);
    dst[1] = (uint8_t)((value >> 8) & 0xff);
}

static void write_u32_le(uint8_t *dst, uint32_t value)
{
    dst[0] = (uint8_t)(value & 0xff);
    dst[1] = (uint8_t)((value >> 8) & 0xff);
    dst[2] = (uint8_t)((value >> 16) & 0xff);
    dst[3] = (uint8_t)((value >> 24) & 0xff);
}

static void write_u64_le(uint8_t *dst, uint64_t value)
{
    for (size_t i = 0; i < sizeof(value); ++i)
    {
        dst[i] = (uint8_t)((value >> (8 * i)) & 0xff);
    }
}

static uint16_t audio_streamer_duration_ms(size_t sample_count, uint32_t sample_rate, uint8_t channels)
{
    uint64_t frames_per_channel = sample_count / channels;
    uint64_t duration_ms = (frames_per_channel * 1000U + (sample_rate / 2U)) / sample_rate;
    return duration_ms > UINT16_MAX ? UINT16_MAX : (uint16_t)duration_ms;
}

static bool audio_streamer_seq_acked(uint32_t seq, uint32_t ack_seq)
{
    return seq <= ack_seq;
}

static void audio_streamer_free_frame(audio_streamer_frame_t *frame)
{
    if (frame == NULL)
    {
        return;
    }

    free(frame->samples);
    frame->samples = NULL;
    frame->sample_count = 0;
}

static void audio_streamer_free_protocol_frame(audio_streamer_protocol_frame_t *frame)
{
    if (frame == NULL)
    {
        return;
    }

    free(frame->data);
    frame->data = NULL;
    frame->len = 0;
}

static esp_err_t audio_streamer_build_protocol_frame(const audio_streamer_frame_t *pcm_frame,
                                                     audio_streamer_protocol_frame_t *protocol_frame)
{
    if (pcm_frame == NULL || protocol_frame == NULL || pcm_frame->samples == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    size_t payload_bytes = pcm_frame->sample_count * sizeof(int16_t);
    if (payload_bytes > AUDIO_STREAMER_MAX_PCM_BYTES ||
        payload_bytes > UINT32_MAX ||
        payload_bytes > SIZE_MAX - AUDIO_STREAM_PROTOCOL_HEADER_LEN)
    {
        return ESP_ERR_INVALID_SIZE;
    }

    size_t frame_len = AUDIO_STREAM_PROTOCOL_HEADER_LEN + payload_bytes;
    uint8_t *frame_data = malloc(frame_len);
    if (frame_data == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    uint32_t seq = audio_streamer_next_seq++;
    uint64_t timestamp_ms = (uint64_t)(esp_timer_get_time() / 1000LL);
    uint16_t duration_ms = audio_streamer_duration_ms(pcm_frame->sample_count,
                                                      pcm_frame->sample_rate,
                                                      pcm_frame->channels);

    memcpy(frame_data, AUDIO_STREAM_PROTOCOL_MAGIC, AUDIO_STREAM_PROTOCOL_MAGIC_LEN);
    write_u16_le(&frame_data[4], AUDIO_STREAM_PROTOCOL_HEADER_LEN);
    write_u32_le(&frame_data[6], seq);
    write_u64_le(&frame_data[10], timestamp_ms);
    write_u32_le(&frame_data[18], pcm_frame->sample_rate);
    frame_data[22] = pcm_frame->channels;
    frame_data[23] = AUDIO_STREAM_PROTOCOL_BITS_PER_SAMPLE;
    write_u16_le(&frame_data[24], duration_ms);
    write_u32_le(&frame_data[26], (uint32_t)payload_bytes);
    memcpy(&frame_data[AUDIO_STREAM_PROTOCOL_HEADER_LEN], pcm_frame->samples, payload_bytes);

    protocol_frame->data = frame_data;
    protocol_frame->len = frame_len;
    protocol_frame->seq = seq;
    return ESP_OK;
}

static esp_err_t audio_streamer_buffer_protocol_frame(const audio_streamer_protocol_frame_t *frame)
{
    esp_err_t ret = offline_buffer_push(frame->data, frame->len, frame->seq);
    if (ret != ESP_OK)
    {
        ESP_LOGW(TAG, "Failed to buffer audio frame %" PRIu32 ": %s", frame->seq, esp_err_to_name(ret));
    }
    return ret;
}

static void audio_streamer_drain_offline_buffer(bool *buffered_frame_inflight,
                                                uint32_t *buffered_inflight_seq,
                                                TickType_t *buffered_last_send_tick)
{
    uint32_t ack_seq = 0;
    if (cloud_ws_get_highest_audio_ack(&ack_seq))
    {
        offline_buffer_pop_acked(ack_seq);
        if (*buffered_frame_inflight && audio_streamer_seq_acked(*buffered_inflight_seq, ack_seq))
        {
            *buffered_frame_inflight = false;
        }
    }

    uint32_t oldest_seq = 0;
    esp_err_t oldest_ret = offline_buffer_peek_next_seq(&oldest_seq);
    if (oldest_ret == ESP_ERR_NOT_FOUND)
    {
        *buffered_frame_inflight = false;
    }
    else if (oldest_ret == ESP_OK && *buffered_frame_inflight && oldest_seq != *buffered_inflight_seq)
    {
        *buffered_frame_inflight = false;
    }

    if (!cloud_ws_is_connected())
    {
        *buffered_frame_inflight = false;
        return;
    }
    if (offline_buffer_bytes_used() == 0)
    {
        return;
    }
    if (*buffered_frame_inflight)
    {
        TickType_t now = xTaskGetTickCount();
        if ((now - *buffered_last_send_tick) < pdMS_TO_TICKS(AUDIO_STREAMER_BUFFERED_RETRY_MS))
        {
            return;
        }
        *buffered_frame_inflight = false;
    }

    uint8_t *buffered_frame = NULL;
    size_t buffered_len = 0;
    uint32_t buffered_seq = 0;
    esp_err_t ret = offline_buffer_peek_next(&buffered_frame, &buffered_len, &buffered_seq);
    if (ret == ESP_ERR_NOT_FOUND)
    {
        return;
    }
    if (ret != ESP_OK)
    {
        ESP_LOGW(TAG, "Failed to read buffered audio frame: %s", esp_err_to_name(ret));
        return;
    }

    ret = cloud_ws_send_binary(buffered_frame, buffered_len, 0);
    free(buffered_frame);
    if (ret == ESP_OK)
    {
        *buffered_frame_inflight = true;
        *buffered_inflight_seq = buffered_seq;
        *buffered_last_send_tick = xTaskGetTickCount();
        ESP_LOGD(TAG, "Queued buffered audio frame seq=%" PRIu32, buffered_seq);
    }
    else
    {
        ESP_LOGD(TAG, "Buffered audio frame %" PRIu32 " not sent: %s",
                 buffered_seq,
                 esp_err_to_name(ret));
    }
}

static void audio_streamer_flush_queue(void)
{
    if (audio_streamer_queue == NULL)
    {
        return;
    }

    audio_streamer_frame_t frame;
    while (xQueueReceive(audio_streamer_queue, &frame, 0) == pdPASS)
    {
        audio_streamer_free_frame(&frame);
    }
}

static void audio_streamer_task(void *pvParameters)
{
    (void)pvParameters;

    audio_streamer_frame_t frame;
    bool buffered_frame_inflight = false;
    uint32_t buffered_inflight_seq = 0;
    TickType_t buffered_last_send_tick = 0;
    while (1)
    {
        audio_streamer_drain_offline_buffer(&buffered_frame_inflight,
                                            &buffered_inflight_seq,
                                            &buffered_last_send_tick);

        if (xQueueReceive(audio_streamer_queue, &frame, pdMS_TO_TICKS(AUDIO_STREAMER_DRAIN_INTERVAL_MS)) != pdPASS)
        {
            continue;
        }

        if (audio_streamer_active)
        {
            audio_streamer_protocol_frame_t protocol_frame = {0};
            esp_err_t ret = audio_streamer_build_protocol_frame(&frame, &protocol_frame);
            if (ret != ESP_OK)
            {
                ESP_LOGD(TAG, "Audio frame build failed: %s", esp_err_to_name(ret));
            }
            else
            {
                audio_streamer_buffer_protocol_frame(&protocol_frame);
                audio_streamer_free_protocol_frame(&protocol_frame);
            }
        }

        audio_streamer_free_frame(&frame);
    }
}

esp_err_t audio_streamer_start(void)
{
    if (!runtime_config_audio_upload_enabled())
    {
        ESP_LOGW(TAG, "Audio streaming disabled by incomplete cloud upload config");
        return ESP_ERR_INVALID_STATE;
    }

    if (audio_streamer_queue == NULL)
    {
        audio_streamer_queue = xQueueCreate(AUDIO_STREAMER_QUEUE_LEN, sizeof(audio_streamer_frame_t));
        if (audio_streamer_queue == NULL)
        {
            return ESP_ERR_NO_MEM;
        }
    }

    esp_err_t ret = offline_buffer_init();
    if (ret != ESP_OK)
    {
        ESP_LOGW(TAG, "Offline audio buffer unavailable: %s", esp_err_to_name(ret));
        return ret;
    }

    if (audio_streamer_task_handle == NULL)
    {
        if (xTaskCreate(audio_streamer_task,
                        "audio_stream",
                        AUDIO_STREAMER_TASK_STACK_SIZE,
                        NULL,
                        AUDIO_STREAMER_TASK_PRIORITY,
                        &audio_streamer_task_handle) != pdPASS)
        {
            return ESP_ERR_NO_MEM;
        }
    }

    audio_streamer_flush_queue();
    audio_streamer_active = true;
    ESP_LOGI(TAG, "Audio streamer started");
    return ESP_OK;
}

void audio_streamer_stop(void)
{
    if (!audio_streamer_active)
    {
        return;
    }

    audio_streamer_active = false;
    audio_streamer_flush_queue();
    ESP_LOGI(TAG, "Audio streamer stopped");
}

bool audio_streamer_is_active(void)
{
    return audio_streamer_active;
}

esp_err_t audio_streamer_submit_pcm(const int16_t *samples,
                                    size_t sample_count,
                                    uint32_t sample_rate,
                                    uint8_t channels)
{
    if (!audio_streamer_active)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (samples == NULL || sample_count == 0 || sample_rate == 0 || channels == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if ((sample_count % channels) != 0)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (sample_count > (SIZE_MAX / sizeof(int16_t)))
    {
        return ESP_ERR_INVALID_SIZE;
    }

    size_t byte_len = sample_count * sizeof(int16_t);
    if (byte_len > AUDIO_STREAMER_MAX_PCM_BYTES || byte_len > INT_MAX)
    {
        return ESP_ERR_INVALID_SIZE;
    }

    int16_t *copy = malloc(byte_len);
    if (copy == NULL)
    {
        return ESP_ERR_NO_MEM;
    }
    memcpy(copy, samples, byte_len);

    audio_streamer_frame_t frame = {
        .samples = copy,
        .sample_count = sample_count,
        .sample_rate = sample_rate,
        .channels = channels,
    };

    if (xQueueSend(audio_streamer_queue, &frame, 0) != pdPASS)
    {
        free(copy);
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}
