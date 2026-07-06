#include <stdio.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#include "esp_dsp.h"
#include "bsp_board_extra.h"

#define TAG "audio_fft"

#define N_SAMPLES 1024
#define SAMPLE_RATE 16000
#define CHANNELS 2
#define DISPLAY_REFRESH_MS 200
#define STRIPE_COUNT 64

#define CANVAS_WIDTH 300
#define CANVAS_HEIGHT 150
#define RECORDING_PATH "/spiffs/recording.wav"
#define WAV_HEADER_SIZE 44
#define WAV_PCM_FORMAT 1
#define BITS_PER_SAMPLE 16
#define STATUS_MESSAGE_LEN 32
#define STARTUP_ERROR_MESSAGE_LEN 64
#define DISPLAY_AUTO_OFF_DELAY_MS 10000
#define CAPTURE_CHUNK_BYTES (N_SAMPLES * CHANNELS * sizeof(int16_t))
#define RECORDING_STORAGE_RESERVE_BYTES (64 * 1024)
#define RECORDING_MIN_START_FREE_BYTES (RECORDING_STORAGE_RESERVE_BYTES + CAPTURE_CHUNK_BYTES)

typedef enum {
    APP_UI_STATE_READY,
    APP_UI_STATE_RECORDING,
    APP_UI_STATE_SAVED,
    APP_UI_STATE_PLAYING,
    APP_UI_STATE_ERROR,
} app_ui_state_t;

typedef struct {
    FILE *fp;
    uint32_t data_bytes;
} wav_writer_t;

__attribute__((aligned(16))) int16_t raw_data[N_SAMPLES * CHANNELS];
__attribute__((aligned(16))) float audio_buffer[N_SAMPLES];
__attribute__((aligned(16))) float wind[N_SAMPLES];
__attribute__((aligned(16))) float fft_buffer[N_SAMPLES * 2];
__attribute__((aligned(16))) float spectrum[N_SAMPLES / 2];

float display_spectrum[STRIPE_COUNT];
float peak[STRIPE_COUNT];

static lv_obj_t *record_btn;
static lv_obj_t *stop_btn;
static lv_obj_t *play_btn;
static lv_obj_t *status_label;
static lv_obj_t *wake_layer;
static SemaphoreHandle_t app_state_mutex;
static app_ui_state_t app_ui_state = APP_UI_STATE_READY;
static bool recording_available = false;
static TickType_t recording_started_tick;
static uint32_t saved_recording_seconds;
static bool pending_status_available = false;
static char pending_status_message[STATUS_MESSAGE_LEN];
static char startup_error_message[STARTUP_ERROR_MESSAGE_LEN] = "Storage/audio startup failed";
static bool storage_full_status_active = false;
static const TickType_t display_auto_off_delay_ticks = pdMS_TO_TICKS(DISPLAY_AUTO_OFF_DELAY_MS);
static bool display_auto_off_enabled = true;
static bool display_auto_off_off = false;
static TickType_t last_touch_or_ui_activity_tick;
static wav_writer_t wav_writer = {0};

static void refresh_recording_controls(void);
static void set_status_text(const char *text);

static void put_le16(uint8_t *dest, uint16_t value)
{
    dest[0] = (uint8_t)(value & 0xff);
    dest[1] = (uint8_t)((value >> 8) & 0xff);
}

static void put_le32(uint8_t *dest, uint32_t value)
{
    dest[0] = (uint8_t)(value & 0xff);
    dest[1] = (uint8_t)((value >> 8) & 0xff);
    dest[2] = (uint8_t)((value >> 16) & 0xff);
    dest[3] = (uint8_t)((value >> 24) & 0xff);
}

static void build_wav_header(uint8_t header[WAV_HEADER_SIZE], uint32_t data_bytes)
{
    const uint32_t byte_rate = SAMPLE_RATE * CHANNELS * BITS_PER_SAMPLE / 8;
    const uint16_t block_align = CHANNELS * BITS_PER_SAMPLE / 8;

    memcpy(&header[0], "RIFF", 4);
    put_le32(&header[4], 36 + data_bytes);
    memcpy(&header[8], "WAVE", 4);
    memcpy(&header[12], "fmt ", 4);
    put_le32(&header[16], 16);
    put_le16(&header[20], WAV_PCM_FORMAT);
    put_le16(&header[22], CHANNELS);
    put_le32(&header[24], SAMPLE_RATE);
    put_le32(&header[28], byte_rate);
    put_le16(&header[32], block_align);
    put_le16(&header[34], BITS_PER_SAMPLE);
    memcpy(&header[36], "data", 4);
    put_le32(&header[40], data_bytes);
}

static esp_err_t wav_writer_write_header(FILE *fp, uint32_t data_bytes)
{
    uint8_t header[WAV_HEADER_SIZE] = {0};
    build_wav_header(header, data_bytes);

    if (fwrite(header, 1, sizeof(header), fp) != sizeof(header))
    {
        return ESP_FAIL;
    }

    return ESP_OK;
}

static void wav_writer_close(void)
{
    if (wav_writer.fp)
    {
        fclose(wav_writer.fp);
        wav_writer.fp = NULL;
    }
    wav_writer.data_bytes = 0;
}

static esp_err_t wav_writer_start(void)
{
    wav_writer_close();

    wav_writer.fp = fopen(RECORDING_PATH, "wb");
    if (wav_writer.fp == NULL)
    {
        ESP_LOGE(TAG, "Failed to open recording file: %s", RECORDING_PATH);
        return ESP_FAIL;
    }

    wav_writer.data_bytes = 0;
    esp_err_t ret = wav_writer_write_header(wav_writer.fp, 0);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to write placeholder WAV header");
        wav_writer_close();
        return ret;
    }

    return ESP_OK;
}

static esp_err_t wav_writer_write_pcm(const void *data, size_t len)
{
    if (wav_writer.fp == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (len > UINT32_MAX - wav_writer.data_bytes)
    {
        ESP_LOGE(TAG, "Recording exceeds WAV size limit");
        return ESP_ERR_INVALID_SIZE;
    }

    if (fwrite(data, 1, len, wav_writer.fp) != len)
    {
        ESP_LOGE(TAG, "Failed to write PCM data");
        return ESP_FAIL;
    }

    wav_writer.data_bytes += (uint32_t)len;
    return ESP_OK;
}

static esp_err_t wav_writer_finalize(uint32_t *data_bytes)
{
    if (wav_writer.fp == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    uint32_t final_data_bytes = wav_writer.data_bytes;
    esp_err_t ret = ESP_OK;

    if (fseek(wav_writer.fp, 0, SEEK_SET) != 0)
    {
        ESP_LOGE(TAG, "Failed to seek to WAV header");
        ret = ESP_FAIL;
    }
    else
    {
        ret = wav_writer_write_header(wav_writer.fp, final_data_bytes);
    }

    if (ret == ESP_OK && fflush(wav_writer.fp) != 0)
    {
        ESP_LOGE(TAG, "Failed to flush WAV file");
        ret = ESP_FAIL;
    }

    if (fclose(wav_writer.fp) != 0)
    {
        ESP_LOGE(TAG, "Failed to close WAV file");
        ret = ESP_FAIL;
    }

    wav_writer.fp = NULL;
    wav_writer.data_bytes = 0;

    if (ret == ESP_OK && data_bytes)
    {
        *data_bytes = final_data_bytes;
    }

    return ret;
}

static void app_state_lock(void)
{
    if (app_state_mutex)
    {
        xSemaphoreTake(app_state_mutex, portMAX_DELAY);
    }
}

static void app_state_unlock(void)
{
    if (app_state_mutex)
    {
        xSemaphoreGive(app_state_mutex);
    }
}

static void app_status_post_locked(const char *status)
{
    if (status == NULL)
    {
        return;
    }

    snprintf(pending_status_message, sizeof(pending_status_message), "%s", status);
    pending_status_available = true;
}

static void set_startup_error_message(const char *step, esp_err_t ret)
{
    ESP_LOGE(TAG, "%s failed: %s", step, esp_err_to_name(ret));
    snprintf(startup_error_message, sizeof(startup_error_message),
             "%s failed\n%s", step, esp_err_to_name(ret));
}

static bool app_status_take(char *status, size_t status_len)
{
    bool has_status = false;

    app_state_lock();
    if (pending_status_available && status && status_len > 0)
    {
        snprintf(status, status_len, "%s", pending_status_message);
        pending_status_available = false;
        has_status = true;
    }
    app_state_unlock();

    return has_status;
}

static bool app_storage_full_status_is_active(void)
{
    app_state_lock();
    bool active = storage_full_status_active;
    app_state_unlock();

    return active;
}

static bool recording_file_is_valid(void)
{
    struct stat st;

    if (stat(RECORDING_PATH, &st) != 0)
    {
        return false;
    }

    return st.st_size >= WAV_HEADER_SIZE;
}

static esp_err_t spiffs_free_bytes(size_t *free_bytes)
{
    size_t total = 0;
    size_t used = 0;
    esp_err_t ret = esp_spiffs_info(CONFIG_BSP_SPIFFS_PARTITION_LABEL, &total, &used);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "SPIFFS info query failed while checking storage: %s", esp_err_to_name(ret));
        return ret;
    }

    if (free_bytes)
    {
        *free_bytes = total > used ? total - used : 0;
    }

    return ESP_OK;
}

static esp_err_t storage_can_accept_write(size_t next_write_len, bool *can_write, size_t *free_bytes)
{
    size_t free_now = 0;
    esp_err_t ret = spiffs_free_bytes(&free_now);
    if (ret != ESP_OK)
    {
        return ret;
    }

    bool has_room = free_now >= next_write_len &&
                    free_now - next_write_len >= RECORDING_STORAGE_RESERVE_BYTES;

    if (can_write)
    {
        *can_write = has_room;
    }
    if (free_bytes)
    {
        *free_bytes = free_now;
    }

    return ESP_OK;
}

static app_ui_state_t app_state_get(void)
{
    app_state_lock();
    app_ui_state_t state = app_ui_state;
    app_state_unlock();
    return state;
}

static void app_state_snapshot(app_ui_state_t *state, bool *has_recording, TickType_t *started_tick, uint32_t *saved_seconds)
{
    app_state_lock();
    if (state)
    {
        *state = app_ui_state;
    }
    if (has_recording)
    {
        *has_recording = recording_available;
    }
    if (started_tick)
    {
        *started_tick = recording_started_tick;
    }
    if (saved_seconds)
    {
        *saved_seconds = saved_recording_seconds;
    }
    app_state_unlock();
}

static void app_state_set_recording(void)
{
    app_state_lock();
    app_ui_state = APP_UI_STATE_RECORDING;
    recording_available = false;
    recording_started_tick = xTaskGetTickCount();
    saved_recording_seconds = 0;
    pending_status_available = false;
    storage_full_status_active = false;
    app_state_unlock();
}

static void app_state_set_error(void)
{
    app_state_lock();
    app_ui_state = APP_UI_STATE_ERROR;
    recording_available = false;
    saved_recording_seconds = 0;
    pending_status_available = false;
    storage_full_status_active = false;
    app_state_unlock();
}

static uint32_t ticks_to_seconds(TickType_t ticks)
{
    return (uint32_t)(pdTICKS_TO_MS(ticks) / 1000);
}

static esp_err_t finalize_recording_locked(uint32_t *saved_seconds, uint32_t *data_bytes)
{
    TickType_t elapsed_ticks = xTaskGetTickCount() - recording_started_tick;
    esp_err_t ret = wav_writer_finalize(data_bytes);
    if (ret == ESP_OK && recording_file_is_valid())
    {
        uint32_t elapsed_seconds = ticks_to_seconds(elapsed_ticks);
        app_ui_state = APP_UI_STATE_SAVED;
        recording_available = true;
        saved_recording_seconds = elapsed_seconds;
        pending_status_available = false;
        storage_full_status_active = false;
        if (saved_seconds)
        {
            *saved_seconds = elapsed_seconds;
        }
    }
    else
    {
        app_ui_state = APP_UI_STATE_ERROR;
        recording_available = false;
        saved_recording_seconds = 0;
        pending_status_available = false;
        storage_full_status_active = false;
        if (ret == ESP_OK)
        {
            ret = ESP_FAIL;
        }
    }

    return ret;
}

static void format_status_duration(char *buffer, size_t buffer_len, const char *prefix, uint32_t seconds)
{
    snprintf(buffer, buffer_len, "%s %02" PRIu32 ":%02" PRIu32, prefix, seconds / 60, seconds % 60);
}

static void display_auto_off_activity_mark(void)
{
    last_touch_or_ui_activity_tick = xTaskGetTickCount();
}

static void display_auto_off_overlay_update(void)
{
    if (wake_layer == NULL)
    {
        return;
    }

    if (display_auto_off_off)
    {
        lv_obj_clear_flag(wake_layer, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(wake_layer);
    }
    else
    {
        lv_obj_add_flag(wake_layer, LV_OBJ_FLAG_HIDDEN);
    }
}

static void display_auto_off_wake(void)
{
    bool was_off = display_auto_off_off;

    if (display_auto_off_off)
    {
        esp_err_t ret = bsp_display_backlight_on();
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to wake display: %s", esp_err_to_name(ret));
        }
        display_auto_off_off = false;
    }

    display_auto_off_activity_mark();
    display_auto_off_overlay_update();

    if (was_off)
    {
        refresh_recording_controls();
    }
}

static void display_auto_off_force_on(void)
{
    if (display_auto_off_off)
    {
        esp_err_t ret = bsp_display_backlight_on();
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to restore display: %s", esp_err_to_name(ret));
        }
    }

    display_auto_off_off = false;
    display_auto_off_activity_mark();
    display_auto_off_overlay_update();
}

static void display_auto_off_maybe_sleep(void)
{
    if (!display_auto_off_enabled || display_auto_off_off)
    {
        return;
    }

    TickType_t now = xTaskGetTickCount();
    if (last_touch_or_ui_activity_tick == 0)
    {
        last_touch_or_ui_activity_tick = now;
    }

    if ((now - last_touch_or_ui_activity_tick) >= display_auto_off_delay_ticks)
    {
        esp_err_t ret = bsp_display_backlight_off();
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to turn display off: %s", esp_err_to_name(ret));
            display_auto_off_activity_mark();
            return;
        }

        ESP_LOGI(TAG, "Display backlight off after inactivity");
        display_auto_off_off = true;
        display_auto_off_overlay_update();
    }
}

static const char *audio_player_event_name(audio_player_callback_event_t event)
{
    switch (event)
    {
    case AUDIO_PLAYER_CALLBACK_EVENT_IDLE:
        return "idle";
    case AUDIO_PLAYER_CALLBACK_EVENT_COMPLETED_PLAYING_NEXT:
        return "completed_playing_next";
    case AUDIO_PLAYER_CALLBACK_EVENT_PLAYING:
        return "playing";
    case AUDIO_PLAYER_CALLBACK_EVENT_PAUSE:
        return "pause";
    case AUDIO_PLAYER_CALLBACK_EVENT_SHUTDOWN:
        return "shutdown";
    case AUDIO_PLAYER_CALLBACK_EVENT_UNKNOWN_FILE_TYPE:
        return "unknown_file_type";
    case AUDIO_PLAYER_CALLBACK_EVENT_UNKNOWN:
    default:
        return "unknown";
    }
}

static void audio_player_event_cb(audio_player_cb_ctx_t *ctx)
{
    if (ctx == NULL)
    {
        ESP_LOGW(TAG, "Audio player callback received NULL context");
        return;
    }

    ESP_LOGI(TAG, "Audio player event: %s", audio_player_event_name(ctx->audio_event));

    app_state_lock();
    switch (ctx->audio_event)
    {
    case AUDIO_PLAYER_CALLBACK_EVENT_IDLE:
        if (app_ui_state == APP_UI_STATE_PLAYING)
        {
            app_ui_state = recording_available ? APP_UI_STATE_SAVED : APP_UI_STATE_READY;
        }
        break;
    case AUDIO_PLAYER_CALLBACK_EVENT_PLAYING:
    case AUDIO_PLAYER_CALLBACK_EVENT_COMPLETED_PLAYING_NEXT:
        if (app_ui_state == APP_UI_STATE_PLAYING)
        {
            app_status_post_locked("Playing");
        }
        break;
    case AUDIO_PLAYER_CALLBACK_EVENT_UNKNOWN_FILE_TYPE:
        if (recording_file_is_valid())
        {
            app_ui_state = APP_UI_STATE_ERROR;
            recording_available = false;
            saved_recording_seconds = 0;
            storage_full_status_active = false;
            app_status_post_locked("Error");
        }
        else
        {
            app_ui_state = APP_UI_STATE_READY;
            recording_available = false;
            saved_recording_seconds = 0;
            storage_full_status_active = false;
            app_status_post_locked("No recording");
        }
        break;
    case AUDIO_PLAYER_CALLBACK_EVENT_SHUTDOWN:
    case AUDIO_PLAYER_CALLBACK_EVENT_UNKNOWN:
        if (app_ui_state == APP_UI_STATE_PLAYING)
        {
            app_ui_state = APP_UI_STATE_ERROR;
            recording_available = false;
            saved_recording_seconds = 0;
            storage_full_status_active = false;
            app_status_post_locked("Error");
        }
        break;
    case AUDIO_PLAYER_CALLBACK_EVENT_PAUSE:
    default:
        break;
    }
    app_state_unlock();
}

static esp_err_t startup_storage_audio_init(void)
{
    ESP_LOGI(TAG, "Mounting SPIFFS");
    esp_vfs_spiffs_conf_t spiffs_conf = {
        .base_path = CONFIG_BSP_SPIFFS_MOUNT_POINT,
        .partition_label = CONFIG_BSP_SPIFFS_PARTITION_LABEL,
        .max_files = CONFIG_BSP_SPIFFS_MAX_FILES,
        .format_if_mount_failed = true,
    };

    esp_err_t ret = esp_vfs_spiffs_register(&spiffs_conf);
    if (ret == ESP_ERR_INVALID_STATE)
    {
        ESP_LOGW(TAG, "SPIFFS already mounted, continuing startup");
        ret = ESP_OK;
    }
    if (ret != ESP_OK)
    {
        set_startup_error_message("SPIFFS mount", ret);
        return ret;
    }

    size_t spiffs_total = 0;
    size_t spiffs_used = 0;
    ret = esp_spiffs_info(spiffs_conf.partition_label, &spiffs_total, &spiffs_used);
    if (ret != ESP_OK)
    {
        set_startup_error_message("SPIFFS info", ret);
        return ret;
    }
    ESP_LOGI(TAG, "SPIFFS mounted: total=%zu, used=%zu", spiffs_total, spiffs_used);

    ESP_LOGI(TAG, "Initializing audio codec");
    ret = bsp_extra_codec_init();
    if (ret != ESP_OK)
    {
        set_startup_error_message("Audio codec init", ret);
        return ret;
    }

    ESP_LOGI(TAG, "Initializing audio player");
    ret = bsp_extra_player_init();
    if (ret != ESP_OK)
    {
        set_startup_error_message("Audio player init", ret);
        return ret;
    }

    bsp_extra_player_register_callback(audio_player_event_cb, NULL);
    ESP_LOGI(TAG, "Audio player callback registered");

    return ESP_OK;
}

void audio_fft_task(void *pvParameters)
{
    esp_err_t ret = dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "FFT init failed: %d", ret);
        vTaskDelete(NULL);
    }

    dsps_wind_hann_f32(wind, N_SAMPLES);
    ESP_LOGI(TAG, "FFT and window initialized");

    TickType_t last_wake_time = xTaskGetTickCount();
    size_t bytes_read;

    while (1)
    {
        app_ui_state_t state = app_state_get();
        if (state == APP_UI_STATE_PLAYING)
        {
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        ret = bsp_extra_i2s_read(raw_data, N_SAMPLES * CHANNELS * sizeof(int16_t), &bytes_read, portMAX_DELAY);
        if (ret != ESP_OK || bytes_read != N_SAMPLES * CHANNELS * sizeof(int16_t))
        {
            ESP_LOGW(TAG, "I2S read error: %d, bytes: %d", ret, bytes_read);
            continue;
        }

        state = app_state_get();
        if (state == APP_UI_STATE_RECORDING)
        {
            app_state_lock();
            if (app_ui_state == APP_UI_STATE_RECORDING)
            {
                bool can_write = false;
                size_t free_bytes = 0;
                ret = storage_can_accept_write(bytes_read, &can_write, &free_bytes);
                if (ret != ESP_OK)
                {
                    wav_writer_close();
                    app_ui_state = APP_UI_STATE_ERROR;
                    recording_available = false;
                    saved_recording_seconds = 0;
                    storage_full_status_active = false;
                    app_status_post_locked("Error");
                }
                else if (!can_write)
                {
                    uint32_t data_bytes = 0;
                    uint32_t saved_seconds = 0;
                    ESP_LOGW(TAG, "Stopping recording before storage exhaustion: free=%zu, next_write=%zu, reserve=%zu",
                             free_bytes, bytes_read, (size_t)RECORDING_STORAGE_RESERVE_BYTES);
                    ret = finalize_recording_locked(&saved_seconds, &data_bytes);
                    if (ret == ESP_OK)
                    {
                        ESP_LOGW(TAG, "Storage-limited recording saved: %s, seconds=%" PRIu32 ", data bytes=%" PRIu32,
                                 RECORDING_PATH, saved_seconds, data_bytes);
                        storage_full_status_active = true;
                        app_status_post_locked("Storage full");
                    }
                    else
                    {
                        ESP_LOGE(TAG, "Storage-limited recording finalize failed: %s", esp_err_to_name(ret));
                        wav_writer_close();
                        app_ui_state = APP_UI_STATE_ERROR;
                        recording_available = false;
                        saved_recording_seconds = 0;
                        storage_full_status_active = false;
                        app_status_post_locked("Error");
                    }
                }
                else
                {
                    ret = wav_writer_write_pcm(raw_data, bytes_read);
                    if (ret != ESP_OK)
                    {
                        ESP_LOGE(TAG, "Recording write failed: %s", esp_err_to_name(ret));
                        wav_writer_close();
                        app_ui_state = APP_UI_STATE_ERROR;
                        recording_available = false;
                        saved_recording_seconds = 0;
                        storage_full_status_active = false;
                        app_status_post_locked("Error");
                    }
                }
            }
            app_state_unlock();
        }

        for (int i = 0; i < N_SAMPLES; i++)
        {
            int16_t left = raw_data[i * CHANNELS];
            int16_t right = raw_data[i * CHANNELS + 1];
            audio_buffer[i] = (left + right) / (2.0f * 32768.0f);
        }

        dsps_mul_f32(audio_buffer, wind, audio_buffer, N_SAMPLES, 1, 1, 1);

        for (int i = 0; i < N_SAMPLES; i++)
        {
            fft_buffer[2 * i] = audio_buffer[i];
            fft_buffer[2 * i + 1] = 0;
        }

        dsps_fft2r_fc32(fft_buffer, N_SAMPLES);
        dsps_bit_rev_fc32(fft_buffer, N_SAMPLES);

        for (int i = 0; i < N_SAMPLES / 2; i++)
        {
            float real = fft_buffer[2 * i];
            float imag = fft_buffer[2 * i + 1];
            float magnitude = sqrtf(real * real + imag * imag);
            spectrum[i] = 20 * log10f(magnitude / (N_SAMPLES / 2) + 1e-9);
        }

        for (int i = 0; i < STRIPE_COUNT; i++)
        {
            int fft_idx = i * (N_SAMPLES / 2) / STRIPE_COUNT;
            display_spectrum[i] = fmaxf(-90.0f, fminf(0.0f, spectrum[fft_idx]));
        }

        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(1));
    }
}

static void timer_cb(lv_timer_t *timer)
{
    app_ui_state_t state = APP_UI_STATE_READY;
    TickType_t started_tick = 0;
    uint32_t saved_seconds = 0;
    app_state_snapshot(&state, NULL, &started_tick, &saved_seconds);

    char pending_status[STATUS_MESSAGE_LEN];
    if (app_status_take(pending_status, sizeof(pending_status)))
    {
        display_auto_off_force_on();
        set_status_text(pending_status);
    }
    else if (state == APP_UI_STATE_RECORDING)
    {
        char status[24];
        format_status_duration(status, sizeof(status), "Recording", ticks_to_seconds(xTaskGetTickCount() - started_tick));
        set_status_text(status);
    }
    else if (state == APP_UI_STATE_PLAYING)
    {
        set_status_text("Playing");
    }
    else if (state == APP_UI_STATE_ERROR)
    {
        set_status_text("Error");
    }
    else if (state == APP_UI_STATE_SAVED && app_storage_full_status_is_active())
    {
        set_status_text("Storage full");
    }
    else if (state == APP_UI_STATE_SAVED)
    {
        char status[20];
        format_status_duration(status, sizeof(status), "Saved", saved_seconds);
        set_status_text(status);
    }

    refresh_recording_controls();
    display_auto_off_maybe_sleep();

    if (state == APP_UI_STATE_PLAYING || display_auto_off_off)
    {
        return;
    }

    lv_obj_t *canvas = (lv_obj_t *)lv_timer_get_user_data(timer);
    lv_layer_t layer;
    lv_canvas_init_layer(canvas, &layer);
    lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_COVER);

    const int stripe_width = CANVAS_WIDTH / STRIPE_COUNT;
    const int center_y = CANVAS_HEIGHT / 2;

    const int bar_gap_px = 2;

    for (int i = 0; i < STRIPE_COUNT; i++) {
         float db = display_spectrum[i];
        float db_min = -90.0f, db_max = 0.0f;

        float norm = (db - db_min) / (db_max - db_min);
        norm = fmaxf(0.0f, fminf(1.0f, norm));
        norm = sqrtf(norm);

        int bar_height = (int)(norm * (CANVAS_HEIGHT / 2));

        if (peak[i] < bar_height) {
            peak[i] = bar_height;
        } else {
            peak[i] -= 2;
            if (peak[i] < 0) peak[i] = 0;
        }

        float hue_step = 270.0f / STRIPE_COUNT;
        uint16_t hue = (uint16_t)(i * hue_step);
        lv_color_t color = lv_color_hsv_to_rgb(hue, 100, 100);

        lv_draw_rect_dsc_t rect_dsc;
        lv_draw_rect_dsc_init(&rect_dsc);
        rect_dsc.bg_color = color;
        rect_dsc.bg_opa = LV_OPA_COVER;

        int x_start = i * stripe_width + bar_gap_px / 2;
        int x_end   = (i + 1) * stripe_width - bar_gap_px / 2 - 1;

        lv_area_t bar_area = {
            .x1 = x_start,
            .y1 = center_y - bar_height,
            .x2 = x_end,
            .y2 = center_y + bar_height
        };
        lv_draw_rect(&layer, &rect_dsc, &bar_area);

        int peak_y_top = center_y - (int)peak[i] - 2;
        int peak_y_bot = center_y + (int)peak[i];

        lv_area_t particle_area_top = {
            .x1 = x_start,
            .y1 = peak_y_top,
            .x2 = x_end,
            .y2 = peak_y_top + 2
        };
        lv_draw_rect(&layer, &rect_dsc, &particle_area_top);

        lv_area_t particle_area_bot = {
            .x1 = x_start,
            .y1 = peak_y_bot,
            .x2 = x_end,
            .y2 = peak_y_bot + 2
        };
        lv_draw_rect(&layer, &rect_dsc, &particle_area_bot);
    }

    lv_canvas_finish_layer(canvas, &layer);
}

static void set_button_enabled(lv_obj_t *button, bool enabled)
{
    if (button == NULL)
    {
        return;
    }

    if (enabled)
    {
        lv_obj_remove_state(button, LV_STATE_DISABLED);
    }
    else
    {
        lv_obj_add_state(button, LV_STATE_DISABLED);
    }
}

static void refresh_recording_controls(void)
{
    app_ui_state_t state = APP_UI_STATE_READY;
    bool has_recording = false;
    app_state_snapshot(&state, &has_recording, NULL, NULL);

    bool record_enabled = (state == APP_UI_STATE_READY) ||
                          (state == APP_UI_STATE_SAVED) ||
                          (state == APP_UI_STATE_ERROR);
    bool stop_enabled = state == APP_UI_STATE_RECORDING;
    bool play_enabled = has_recording &&
                        state != APP_UI_STATE_RECORDING &&
                        state != APP_UI_STATE_PLAYING;

    set_button_enabled(record_btn, record_enabled);
    set_button_enabled(stop_btn, stop_enabled);
    set_button_enabled(play_btn, play_enabled);
}

static void set_status_text(const char *text)
{
    if (status_label)
    {
        lv_label_set_text(status_label, text);
    }
}

static void record_button_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED)
    {
        return;
    }

    display_auto_off_wake();

    size_t free_bytes = 0;
    esp_err_t ret = spiffs_free_bytes(&free_bytes);
    if (ret != ESP_OK)
    {
        app_state_set_error();
        display_auto_off_force_on();
        set_status_text("Error");
        refresh_recording_controls();
        return;
    }

    if (free_bytes < RECORDING_MIN_START_FREE_BYTES)
    {
        ESP_LOGW(TAG, "Not enough SPIFFS space to start recording: free=%zu, required=%zu",
                 free_bytes, (size_t)RECORDING_MIN_START_FREE_BYTES);
        app_state_lock();
        app_ui_state = APP_UI_STATE_READY;
        recording_available = false;
        saved_recording_seconds = 0;
        pending_status_available = false;
        storage_full_status_active = false;
        app_state_unlock();
        display_auto_off_force_on();
        set_status_text("Storage full");
        refresh_recording_controls();
        return;
    }

    ret = wav_writer_start();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Recording start failed: %s", esp_err_to_name(ret));
        app_state_set_error();
        display_auto_off_force_on();
        set_status_text("Error");
        refresh_recording_controls();
        return;
    }

    ESP_LOGI(TAG, "Recording started: %s", RECORDING_PATH);
    app_state_set_recording();
    display_auto_off_activity_mark();
    set_status_text("Recording 00:00");
    refresh_recording_controls();
}

static void stop_button_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED)
    {
        return;
    }

    display_auto_off_wake();

    uint32_t data_bytes = 0;
    uint32_t saved_seconds = 0;
    app_state_lock();
    if (app_ui_state != APP_UI_STATE_RECORDING)
    {
        app_state_unlock();
        refresh_recording_controls();
        return;
    }

    esp_err_t ret = finalize_recording_locked(&saved_seconds, &data_bytes);
    app_state_unlock();

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Recording finalize failed: %s", esp_err_to_name(ret));
        display_auto_off_force_on();
        set_status_text("Error");
        refresh_recording_controls();
        return;
    }

    char status[24];
    format_status_duration(status, sizeof(status), "Saved", saved_seconds);

    ESP_LOGI(TAG, "Recording saved: %s, data bytes: %" PRIu32, RECORDING_PATH, data_bytes);
    display_auto_off_force_on();
    set_status_text(status);
    refresh_recording_controls();
}

static void play_button_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED)
    {
        return;
    }

    display_auto_off_wake();

    bool has_recording = false;
    app_state_snapshot(NULL, &has_recording, NULL, NULL);

    if (!has_recording || !recording_file_is_valid())
    {
        ESP_LOGW(TAG, "Play requested with no finalized recording");
        app_state_lock();
        app_ui_state = APP_UI_STATE_READY;
        recording_available = false;
        saved_recording_seconds = 0;
        pending_status_available = false;
        storage_full_status_active = false;
        app_state_unlock();
        display_auto_off_force_on();
        set_status_text("No recording");
        refresh_recording_controls();
        return;
    }

    app_state_lock();
    app_ui_state = APP_UI_STATE_PLAYING;
    pending_status_available = false;
    storage_full_status_active = false;
    app_state_unlock();
    display_auto_off_force_on();
    set_status_text("Playing");
    refresh_recording_controls();

    ESP_LOGI(TAG, "Playing recording: %s", RECORDING_PATH);
    esp_err_t ret = bsp_extra_player_play_file(RECORDING_PATH);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Playback start failed: %s", esp_err_to_name(ret));

        bool file_still_valid = recording_file_is_valid();
        app_state_lock();
        if (file_still_valid)
        {
            app_ui_state = APP_UI_STATE_ERROR;
            recording_available = false;
            saved_recording_seconds = 0;
            pending_status_available = false;
            storage_full_status_active = false;
            app_state_unlock();
            display_auto_off_force_on();
            set_status_text("Error");
        }
        else
        {
            app_ui_state = APP_UI_STATE_READY;
            recording_available = false;
            saved_recording_seconds = 0;
            pending_status_available = false;
            storage_full_status_active = false;
            app_state_unlock();
            display_auto_off_force_on();
            set_status_text("No recording");
        }
        refresh_recording_controls();
    }
}

static void wake_layer_event_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    if (code != LV_EVENT_PRESSED && code != LV_EVENT_CLICKED)
    {
        return;
    }

    if (display_auto_off_off)
    {
        display_auto_off_wake();
    }
    else
    {
        display_auto_off_activity_mark();
    }
}

static void ui_activity_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) == LV_EVENT_PRESSED)
    {
        display_auto_off_activity_mark();
    }
}

static void create_wake_layer(void)
{
    if (wake_layer != NULL)
    {
        return;
    }

    wake_layer = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(wake_layer);
    lv_obj_set_size(wake_layer, LV_PCT(100), LV_PCT(100));
    lv_obj_center(wake_layer);
    lv_obj_add_flag(wake_layer, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(wake_layer, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(wake_layer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(wake_layer, wake_layer_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(wake_layer, wake_layer_event_cb, LV_EVENT_CLICKED, NULL);
}

static void startup_error_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    display_auto_off_maybe_sleep();
}

static lv_obj_t *create_control_button(lv_obj_t *parent, const char *text, lv_event_cb_t event_cb)
{
    lv_obj_t *button = lv_button_create(parent);
    lv_obj_set_size(button, 82, 40);
    lv_obj_add_event_cb(button, event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(button, ui_activity_event_cb, LV_EVENT_PRESSED, NULL);

    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, text);
    lv_obj_center(label);

    return button;
}

void create_app_ui(void)
{
    LV_DRAW_BUF_DEFINE_STATIC(draw_buf, CANVAS_WIDTH, CANVAS_HEIGHT, LV_COLOR_FORMAT_RGB565);
    LV_DRAW_BUF_INIT_STATIC(draw_buf);

    lv_obj_add_flag(lv_screen_active(), LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(lv_screen_active(), ui_activity_event_cb, LV_EVENT_PRESSED, NULL);

    lv_obj_t *canvas = lv_canvas_create(lv_screen_active());
    lv_obj_set_size(canvas, CANVAS_WIDTH, CANVAS_HEIGHT);
    lv_obj_align(canvas, LV_ALIGN_CENTER, 0, -52);
    lv_canvas_set_draw_buf(canvas, &draw_buf);
    lv_obj_add_flag(canvas, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(canvas, ui_activity_event_cb, LV_EVENT_PRESSED, NULL);

    lv_timer_create(timer_cb, 33, canvas);

    lv_obj_t *controls = lv_obj_create(lv_screen_active());
    lv_obj_set_size(controls, CANVAS_WIDTH, 44);
    lv_obj_align(controls, LV_ALIGN_CENTER, 0, 70);
    lv_obj_set_flex_flow(controls, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(controls, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(controls, 0, 0);
    lv_obj_set_style_border_width(controls, 0, 0);
    lv_obj_set_style_bg_opa(controls, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(controls, LV_OBJ_FLAG_SCROLLABLE);

    record_btn = create_control_button(controls, "Record", record_button_event_cb);
    stop_btn = create_control_button(controls, "Stop", stop_button_event_cb);
    play_btn = create_control_button(controls, "Play", play_button_event_cb);

    status_label = lv_label_create(lv_screen_active());
    lv_label_set_text(status_label, "Ready");
    lv_obj_align(status_label, LV_ALIGN_CENTER, 0, 112);

    create_wake_layer();

    display_auto_off_force_on();
    refresh_recording_controls();
}

void show_startup_error(void)
{
    lv_obj_add_flag(lv_screen_active(), LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(lv_screen_active(), ui_activity_event_cb, LV_EVENT_PRESSED, NULL);
    create_wake_layer();
    display_auto_off_force_on();

    lv_obj_t *label = lv_label_create(lv_screen_active());
    lv_obj_set_width(label, 280);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_label_set_text(label, startup_error_message);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(label);

    lv_timer_create(startup_error_timer_cb, 33, NULL);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting Audio Spectrum Analyzer");

    lv_display_t *disp = bsp_display_start();
    if (disp)
    {
        bsp_display_backlight_on();
    }

    esp_err_t ret = startup_storage_audio_init();
    if (ret == ESP_OK)
    {
        app_state_mutex = xSemaphoreCreateMutex();
        if (app_state_mutex == NULL)
        {
            ESP_LOGE(TAG, "Failed to create app state mutex");
            snprintf(startup_error_message, sizeof(startup_error_message),
                     "App state mutex failed\n%s", esp_err_to_name(ESP_ERR_NO_MEM));
            ret = ESP_ERR_NO_MEM;
        }
    }

    bsp_display_lock(-1);
    if (ret == ESP_OK)
    {
        create_app_ui();
    }
    else
    {
        show_startup_error();
    }
    bsp_display_unlock();

    if (ret == ESP_OK)
    {
        xTaskCreate(audio_fft_task, "audio_fft", 6 * 1024, NULL, 5, NULL);
    }
}
