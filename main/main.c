#include <stdio.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_spiffs.h"
#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#include "bsp_board_extra.h"
#include "audio_segment_recorder.h"
#include "runtime_config.h"
#include "wifi_manager.h"

#define TAG "audio_recorder"

#define N_SAMPLES 1024
#define SAMPLE_RATE 16000
#define CHANNELS 2
#define UI_STATUS_REFRESH_MS 200
#define UI_WIDTH 300
#define STATUS_MESSAGE_LEN 32
#define STARTUP_ERROR_MESSAGE_LEN 64
#define DISPLAY_AUTO_OFF_DELAY_MS 10000
#define RECORDING_WAKE_DOUBLE_TAP_WINDOW_MS 700
#define RECORDING_WAKE_MAX_TAP_MS 300
#define APP_COMMAND_QUEUE_LEN 4
#define APP_DEFERRED_NETWORK_START_DELAY_MS 750
#define APP_DEFERRED_NETWORK_START_STACK_SIZE 4096
#define APP_DEFERRED_NETWORK_START_PRIORITY 3
#define AUDIO_CAPTURE_TASK_PRIORITY 7
#define APP_HEAP_DIAGNOSTICS_ENABLED 1

typedef enum {
    APP_UI_STATE_STARTING,
    APP_UI_STATE_STARTUP_ERROR,
    APP_UI_STATE_READY,
    APP_UI_STATE_RECORD_STARTING,
    APP_UI_STATE_RECORDING,
    APP_UI_STATE_ERROR,
} app_ui_state_t;

typedef enum {
    APP_CMD_START_RECORDING,
} app_command_type_t;

typedef struct {
    app_command_type_t type;
} app_command_t;

__attribute__((aligned(16))) int16_t raw_data[N_SAMPLES * CHANNELS];

static lv_obj_t *record_btn;
static lv_obj_t *stop_btn;
static lv_obj_t *status_label;
static lv_obj_t *wake_layer;
static lv_obj_t *startup_error_label;
static lv_timer_t *startup_error_timer;
static SemaphoreHandle_t app_state_mutex;
static QueueHandle_t app_command_queue;
static TaskHandle_t deferred_network_start_task_handle;
static app_ui_state_t app_ui_state = APP_UI_STATE_STARTING;
static TickType_t recording_started_tick;
static bool recording_cloud_active = false;
static bool recording_local_only = false;
static bool pending_status_available = false;
static char pending_status_message[STATUS_MESSAGE_LEN];
static char startup_error_message[STARTUP_ERROR_MESSAGE_LEN] = "Storage/audio startup failed";
static bool startup_error_screen_event_registered = false;
static const TickType_t display_auto_off_delay_ticks = pdMS_TO_TICKS(DISPLAY_AUTO_OFF_DELAY_MS);
static const TickType_t recording_wake_double_tap_window_ticks = pdMS_TO_TICKS(RECORDING_WAKE_DOUBLE_TAP_WINDOW_MS);
static const TickType_t recording_wake_max_tap_ticks = pdMS_TO_TICKS(RECORDING_WAKE_MAX_TAP_MS);
static bool display_auto_off_enabled = true;
static bool display_auto_off_off = false;
static TickType_t last_touch_or_ui_activity_tick;
static TickType_t recording_wake_first_tap_tick;
static TickType_t recording_wake_press_start_tick;

static void refresh_recording_controls(void);
static void set_status_text(const char *text);

static void app_heap_diag_log(const char *milestone)
{
#if APP_HEAP_DIAGNOSTICS_ENABLED
    const uint32_t internal_dma_caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA;
    size_t internal_dma_free = heap_caps_get_free_size(internal_dma_caps);
    size_t internal_dma_largest = heap_caps_get_largest_free_block(internal_dma_caps);
    size_t internal_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    size_t internal_largest = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
    size_t default_free = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    size_t default_largest = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);
    size_t psram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    size_t psram_largest = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);

    ESP_LOGI(TAG,
             "heap[%s]: internal_dma_free=%zu internal_dma_largest=%zu internal_free=%zu internal_largest=%zu default_free=%zu default_largest=%zu psram_free=%zu psram_largest=%zu",
             milestone,
             internal_dma_free,
             internal_dma_largest,
             internal_free,
             internal_largest,
             default_free,
             default_largest,
             psram_free,
             psram_largest);
#else
    (void)milestone;
#endif
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

static bool app_recording_cloud_active(void)
{
    app_state_lock();
    bool active = recording_cloud_active;
    app_state_unlock();

    return active;
}

static bool app_recording_local_only(void)
{
    app_state_lock();
    bool local_only = recording_local_only;
    app_state_unlock();

    return local_only;
}

static app_ui_state_t app_state_get(void)
{
    app_state_lock();
    app_ui_state_t state = app_ui_state;
    app_state_unlock();
    return state;
}

static void app_state_snapshot(app_ui_state_t *state, TickType_t *started_tick)
{
    app_state_lock();
    if (state)
    {
        *state = app_ui_state;
    }
    if (started_tick)
    {
        *started_tick = recording_started_tick;
    }
    app_state_unlock();
}

static void app_state_set_record_starting(void)
{
    app_state_lock();
    app_ui_state = APP_UI_STATE_RECORD_STARTING;
    recording_cloud_active = false;
    recording_local_only = false;
    pending_status_available = false;
    app_state_unlock();
}

static void app_state_set_error(void)
{
    app_state_lock();
    app_ui_state = APP_UI_STATE_ERROR;
    recording_cloud_active = false;
    recording_local_only = false;
    pending_status_available = false;
    app_state_unlock();
}

static void app_state_set_ready(void)
{
    app_state_lock();
    app_ui_state = APP_UI_STATE_READY;
    recording_cloud_active = false;
    recording_local_only = false;
    pending_status_available = false;
    app_state_unlock();
}

static void app_state_set_startup_error(void)
{
    app_state_lock();
    app_ui_state = APP_UI_STATE_STARTUP_ERROR;
    recording_cloud_active = false;
    recording_local_only = false;
    pending_status_available = false;
    app_state_unlock();
}

static bool app_command_send(app_command_type_t type)
{
    if (app_command_queue == NULL)
    {
        ESP_LOGE(TAG, "App command queue is not initialized");
        return false;
    }

    app_command_t command = {
        .type = type,
    };
    if (xQueueSend(app_command_queue, &command, 0) != pdPASS)
    {
        ESP_LOGE(TAG, "App command queue is full");
        return false;
    }

    return true;
}

static uint32_t ticks_to_seconds(TickType_t ticks)
{
    return (uint32_t)(pdTICKS_TO_MS(ticks) / 1000);
}

static esp_err_t finalize_recording_locked(uint32_t *saved_seconds)
{
    uint32_t elapsed_seconds = ticks_to_seconds(xTaskGetTickCount() - recording_started_tick);
    esp_err_t ret = audio_segment_recorder_stop();

    app_ui_state = ret == ESP_OK ? APP_UI_STATE_READY : APP_UI_STATE_ERROR;
    recording_cloud_active = false;
    recording_local_only = false;
    pending_status_available = false;

    if (ret == ESP_OK && saved_seconds)
    {
        *saved_seconds = elapsed_seconds;
    }
    return ret;
}

static void abort_recording_locked(const char *status)
{
    audio_segment_recorder_abort();
    app_ui_state = APP_UI_STATE_ERROR;
    recording_cloud_active = false;
    recording_local_only = false;
    app_status_post_locked(status);
}

static void app_command_start_recording(void)
{
    esp_err_t ret = audio_segment_recorder_start();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Segment recording start failed: %s", esp_err_to_name(ret));
        app_state_lock();
        if (app_ui_state == APP_UI_STATE_RECORD_STARTING)
        {
            app_ui_state = APP_UI_STATE_READY;
            recording_cloud_active = false;
            recording_local_only = false;
            pending_status_available = false;
            app_status_post_locked("Storage error");
        }
        app_state_unlock();
        return;
    }

    bool cloud_active = audio_segment_recorder_upload_enabled();
    ESP_LOGI(TAG, "Recording started%s",
             cloud_active ? " with segmented cloud upload" : " local-only segmented");

    bool stale_command = false;
    app_state_lock();
    if (app_ui_state == APP_UI_STATE_RECORD_STARTING)
    {
        app_ui_state = APP_UI_STATE_RECORDING;
        recording_started_tick = xTaskGetTickCount();
        recording_cloud_active = cloud_active;
        recording_local_only = !cloud_active;
        pending_status_available = false;
        app_status_post_locked(cloud_active ? "Recording" : "Local only");
    }
    else
    {
        stale_command = true;
    }
    app_state_unlock();

    if (stale_command)
    {
        audio_segment_recorder_abort();
    }
    else
    {
        app_heap_diag_log("after recording starts");
    }
}

static void app_command_task(void *pvParameters)
{
    (void)pvParameters;

    app_command_t command;
    while (1)
    {
        if (xQueueReceive(app_command_queue, &command, portMAX_DELAY) != pdPASS)
        {
            continue;
        }

        switch (command.type)
        {
        case APP_CMD_START_RECORDING:
            app_command_start_recording();
            break;
        default:
            ESP_LOGW(TAG, "Unknown app command: %d", command.type);
            break;
        }
    }
}

static void format_status_duration(char *buffer, size_t buffer_len, const char *prefix, uint32_t seconds)
{
    snprintf(buffer, buffer_len, "%s %02" PRIu32 ":%02" PRIu32, prefix, seconds / 60, seconds % 60);
}

static void display_auto_off_activity_mark(void)
{
    last_touch_or_ui_activity_tick = xTaskGetTickCount();
}

static void recording_wake_guard_reset(void)
{
    recording_wake_first_tap_tick = 0;
    recording_wake_press_start_tick = 0;
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

    recording_wake_guard_reset();
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

    recording_wake_guard_reset();
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
        recording_wake_guard_reset();
        display_auto_off_overlay_update();
    }
}

static esp_err_t startup_storage_audio_init(void)
{
    ESP_LOGI(TAG, "Mounting SPIFFS");
    esp_vfs_spiffs_conf_t spiffs_conf = {
        .base_path = CONFIG_BSP_SPIFFS_MOUNT_POINT,
        .partition_label = CONFIG_BSP_SPIFFS_PARTITION_LABEL,
        .max_files = CONFIG_BSP_SPIFFS_MAX_FILES,
        .format_if_mount_failed = false,
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

    return ESP_OK;
}

static void audio_capture_task(void *pvParameters)
{
    esp_err_t ret = ESP_OK;
    size_t bytes_read;

    while (1)
    {
        ret = bsp_extra_i2s_read(raw_data, N_SAMPLES * CHANNELS * sizeof(int16_t), &bytes_read, portMAX_DELAY);
        if (ret != ESP_OK || bytes_read != N_SAMPLES * CHANNELS * sizeof(int16_t))
        {
            ESP_LOGW(TAG, "I2S read error: %d, bytes: %d", ret, bytes_read);
            continue;
        }

        app_ui_state_t state = app_state_get();
        if (state != APP_UI_STATE_RECORDING)
        {
            continue;
        }

        if (audio_segment_recorder_is_active())
        {
            esp_err_t segment_ret = audio_segment_recorder_write_pcm(raw_data, bytes_read);
            if (segment_ret != ESP_OK)
            {
                audio_segment_recorder_stats_t stats;
                audio_segment_recorder_get_stats(&stats);
                ESP_LOGE(TAG,
                         "Segment recorder write failed: %s local_gap_count=%" PRIu32 " storage_overflow_count=%" PRIu32 " queue_overflow_count=%" PRIu32,
                         esp_err_to_name(segment_ret),
                         stats.local_gap_count,
                         stats.storage_overflow_count,
                         stats.queue_overflow_count);
                app_state_lock();
                if (app_ui_state == APP_UI_STATE_RECORDING)
                {
                    abort_recording_locked("Storage error");
                }
                app_state_unlock();
                continue;
            }
        }
    }
}

static void status_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    app_ui_state_t state = APP_UI_STATE_READY;
    TickType_t started_tick = 0;
    app_state_snapshot(&state, &started_tick);

    char pending_status[STATUS_MESSAGE_LEN];
    if (app_status_take(pending_status, sizeof(pending_status)))
    {
        display_auto_off_force_on();
        set_status_text(pending_status);
    }
    else if (state == APP_UI_STATE_STARTING)
    {
        set_status_text("Starting...");
    }
    else if (state == APP_UI_STATE_STARTUP_ERROR)
    {
        set_status_text("Error");
    }
    else if (state == APP_UI_STATE_RECORDING)
    {
        char status[24];
        const char *prefix = "Recording";
        if (app_recording_cloud_active())
        {
            prefix = "Recording";
        }
        else if (app_recording_local_only())
        {
            prefix = "Local only";
        }
        format_status_duration(status, sizeof(status), prefix, ticks_to_seconds(xTaskGetTickCount() - started_tick));
        set_status_text(status);
    }
    else if (state == APP_UI_STATE_RECORD_STARTING)
    {
        set_status_text("Starting...");
    }
    else if (state == APP_UI_STATE_ERROR)
    {
        set_status_text("Error");
    }

    refresh_recording_controls();
    display_auto_off_maybe_sleep();
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
    app_state_snapshot(&state, NULL);

    bool record_enabled = (state == APP_UI_STATE_READY) ||
                          (state == APP_UI_STATE_ERROR);
    bool stop_enabled = state == APP_UI_STATE_RECORDING;

    set_button_enabled(record_btn, record_enabled);
    set_button_enabled(stop_btn, stop_enabled);
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

    app_state_set_record_starting();
    display_auto_off_activity_mark();
    set_status_text("Starting...");
    refresh_recording_controls();

    if (!app_command_send(APP_CMD_START_RECORDING))
    {
        app_state_set_error();
        display_auto_off_force_on();
        set_status_text("Error");
        refresh_recording_controls();
    }
}

static void stop_button_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED)
    {
        return;
    }

    display_auto_off_wake();

    uint32_t saved_seconds = 0;
    app_state_lock();
    if (app_ui_state != APP_UI_STATE_RECORDING)
    {
        app_state_unlock();
        refresh_recording_controls();
        return;
    }

    esp_err_t ret = finalize_recording_locked(&saved_seconds);
    app_state_unlock();

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Recording finalize failed: %s", esp_err_to_name(ret));
        display_auto_off_force_on();
        set_status_text("Error");
        refresh_recording_controls();
        return;
    }

    char status[STATUS_MESSAGE_LEN];
    format_status_duration(status, sizeof(status), "Stopped", saved_seconds);
    ESP_LOGI(TAG, "Recording stopped after %" PRIu32 " seconds", saved_seconds);
    display_auto_off_force_on();
    set_status_text(status);
    refresh_recording_controls();
}

static void wake_layer_event_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    if (code != LV_EVENT_PRESSED &&
        code != LV_EVENT_RELEASED &&
        code != LV_EVENT_PRESS_LOST &&
        code != LV_EVENT_SHORT_CLICKED &&
        code != LV_EVENT_DOUBLE_CLICKED)
    {
        return;
    }

    if (!display_auto_off_off)
    {
        if (code == LV_EVENT_PRESSED)
        {
            display_auto_off_activity_mark();
        }
        recording_wake_guard_reset();
        return;
    }

    app_ui_state_t state = app_state_get();
    if (state != APP_UI_STATE_RECORDING)
    {
        recording_wake_guard_reset();
        if (code == LV_EVENT_PRESSED)
        {
            display_auto_off_wake();
        }
        return;
    }

    TickType_t now = xTaskGetTickCount();

    if (code == LV_EVENT_PRESS_LOST)
    {
        recording_wake_guard_reset();
        return;
    }

    if (code == LV_EVENT_PRESSED)
    {
        recording_wake_press_start_tick = now;
        return;
    }

    if (code == LV_EVENT_DOUBLE_CLICKED)
    {
        display_auto_off_wake();
        return;
    }

    if (code == LV_EVENT_RELEASED)
    {
        return;
    }

    if (code != LV_EVENT_SHORT_CLICKED)
    {
        return;
    }

    if (recording_wake_press_start_tick == 0)
    {
        recording_wake_guard_reset();
        return;
    }

    if (recording_wake_press_start_tick != 0)
    {
        TickType_t press_duration = now - recording_wake_press_start_tick;
        recording_wake_press_start_tick = 0;

        if (press_duration > recording_wake_max_tap_ticks)
        {
            recording_wake_guard_reset();
            return;
        }
    }

    if (recording_wake_first_tap_tick != 0 &&
        (now - recording_wake_first_tap_tick) <= recording_wake_double_tap_window_ticks)
    {
        display_auto_off_wake();
        return;
    }

    recording_wake_first_tap_tick = now;
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
    lv_obj_add_event_cb(wake_layer, wake_layer_event_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(wake_layer, wake_layer_event_cb, LV_EVENT_PRESS_LOST, NULL);
    lv_obj_add_event_cb(wake_layer, wake_layer_event_cb, LV_EVENT_SHORT_CLICKED, NULL);
    lv_obj_add_event_cb(wake_layer, wake_layer_event_cb, LV_EVENT_DOUBLE_CLICKED, NULL);
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
    lv_obj_add_flag(lv_screen_active(), LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(lv_screen_active(), ui_activity_event_cb, LV_EVENT_PRESSED, NULL);

    lv_timer_create(status_timer_cb, UI_STATUS_REFRESH_MS, NULL);

    lv_obj_t *controls = lv_obj_create(lv_screen_active());
    lv_obj_set_size(controls, UI_WIDTH, 44);
    lv_obj_align(controls, LV_ALIGN_CENTER, 0, -12);
    lv_obj_set_flex_flow(controls, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(controls, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(controls, 0, 0);
    lv_obj_set_style_border_width(controls, 0, 0);
    lv_obj_set_style_bg_opa(controls, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(controls, LV_OBJ_FLAG_SCROLLABLE);

    record_btn = create_control_button(controls, "Start", record_button_event_cb);
    stop_btn = create_control_button(controls, "Stop", stop_button_event_cb);

    status_label = lv_label_create(lv_screen_active());
    lv_label_set_text(status_label, "Starting...");
    lv_obj_align(status_label, LV_ALIGN_CENTER, 0, 42);

    create_wake_layer();

    display_auto_off_activity_mark();
    display_auto_off_force_on();
    refresh_recording_controls();
}

void show_startup_error(void)
{
    lv_obj_add_flag(lv_screen_active(), LV_OBJ_FLAG_CLICKABLE);
    if (!startup_error_screen_event_registered)
    {
        lv_obj_add_event_cb(lv_screen_active(), ui_activity_event_cb, LV_EVENT_PRESSED, NULL);
        startup_error_screen_event_registered = true;
    }
    create_wake_layer();
    display_auto_off_activity_mark();
    display_auto_off_force_on();

    if (startup_error_label == NULL)
    {
        startup_error_label = lv_label_create(lv_screen_active());
        lv_obj_set_width(startup_error_label, 280);
        lv_label_set_long_mode(startup_error_label, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_align(startup_error_label, LV_TEXT_ALIGN_CENTER, 0);
    }
    lv_label_set_text(startup_error_label, startup_error_message);
    lv_obj_center(startup_error_label);
    lv_obj_move_foreground(startup_error_label);

    if (startup_error_timer == NULL)
    {
        startup_error_timer = lv_timer_create(startup_error_timer_cb, UI_STATUS_REFRESH_MS, NULL);
    }
}

static void deferred_network_start_task(void *pvParameters)
{
    (void)pvParameters;

    vTaskDelay(pdMS_TO_TICKS(APP_DEFERRED_NETWORK_START_DELAY_MS));
    ESP_LOGI(TAG, "Starting deferred Wi-Fi/upload services");

    esp_err_t wifi_ret = wifi_manager_start();
    app_heap_diag_log("after Wi-Fi start");
    if (wifi_ret != ESP_OK)
    {
        ESP_LOGW(TAG, "Wi-Fi startup failed, continuing offline: %s", esp_err_to_name(wifi_ret));
    }

    if (audio_segment_recorder_upload_enabled())
    {
        esp_err_t pending_upload_ret = audio_segment_recorder_start_pending_uploads();
        app_heap_diag_log("after pending segment upload start");
        if (pending_upload_ret != ESP_OK)
        {
            ESP_LOGW(TAG, "Pending segment upload startup failed: %s", esp_err_to_name(pending_upload_ret));
        }
    }

    deferred_network_start_task_handle = NULL;
    vTaskDelete(NULL);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting audio recorder");
    runtime_config_log_startup_warnings();

    esp_err_t ret = ESP_OK;
    app_state_mutex = xSemaphoreCreateMutex();
    if (app_state_mutex == NULL)
    {
        ESP_LOGE(TAG, "Failed to create app state mutex");
        snprintf(startup_error_message, sizeof(startup_error_message),
                 "App state mutex failed\n%s", esp_err_to_name(ESP_ERR_NO_MEM));
        ret = ESP_ERR_NO_MEM;
    }

    app_heap_diag_log("before display init");
    lv_display_t *disp = bsp_display_start();
    app_heap_diag_log("after display init");

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
        app_heap_diag_log("after UI creation");
    }

    if (disp)
    {
        bsp_display_backlight_on();
    }

    if (ret == ESP_OK)
    {
        ret = startup_storage_audio_init();
        if (ret == ESP_OK)
        {
            app_heap_diag_log("after SPIFFS/audio init");
        }
    }
    if (ret == ESP_OK)
    {
        app_command_queue = xQueueCreate(APP_COMMAND_QUEUE_LEN, sizeof(app_command_t));
        if (app_command_queue == NULL)
        {
            ESP_LOGE(TAG, "Failed to create app command queue");
            snprintf(startup_error_message, sizeof(startup_error_message),
                     "App command queue failed\n%s", esp_err_to_name(ESP_ERR_NO_MEM));
            ret = ESP_ERR_NO_MEM;
        }
    }
    if (ret == ESP_OK)
    {
        if (xTaskCreate(app_command_task, "app_cmd", 4 * 1024, NULL, 4, NULL) != pdPASS)
        {
            ESP_LOGE(TAG, "Failed to create app command task");
            snprintf(startup_error_message, sizeof(startup_error_message),
                     "App command task failed\n%s", esp_err_to_name(ESP_ERR_NO_MEM));
            ret = ESP_ERR_NO_MEM;
        }
    }

    if (ret == ESP_OK)
    {
        if (xTaskCreate(audio_capture_task,
                        "audio_capture",
                        6 * 1024,
                        NULL,
                        AUDIO_CAPTURE_TASK_PRIORITY,
                        NULL) != pdPASS)
        {
            ESP_LOGE(TAG, "Failed to create audio capture task");
            snprintf(startup_error_message, sizeof(startup_error_message),
                     "Audio task failed\n%s", esp_err_to_name(ESP_ERR_NO_MEM));
            ret = ESP_ERR_NO_MEM;
        }
    }

    if (ret == ESP_OK)
    {
        app_state_set_ready();
        bsp_display_lock(-1);
        set_status_text("Ready");
        refresh_recording_controls();
        bsp_display_unlock();
    }
    else
    {
        app_state_set_startup_error();

        bsp_display_lock(-1);
        set_status_text("Error");
        show_startup_error();
        refresh_recording_controls();
        bsp_display_unlock();
        return;
    }

    if (xTaskCreate(deferred_network_start_task,
                    "defer_net",
                    APP_DEFERRED_NETWORK_START_STACK_SIZE,
                    NULL,
                    APP_DEFERRED_NETWORK_START_PRIORITY,
                    &deferred_network_start_task_handle) != pdPASS)
    {
        deferred_network_start_task_handle = NULL;
        ESP_LOGW(TAG, "Deferred Wi-Fi/cloud task could not start; continuing local-only");
    }
}
