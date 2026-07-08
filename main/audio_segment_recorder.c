#include "audio_segment_recorder.h"

#include <dirent.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "cJSON.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "psa/crypto.h"
#include "runtime_config.h"
#include "wifi_manager.h"

#define TAG "audio_segments"

#define SEGMENT_STORAGE_ROOT CONFIG_BSP_SPIFFS_MOUNT_POINT
#define SEGMENT_PATH_LEN 160
#define SEGMENT_NAME_PREFIX "segment_"
#define SEGMENT_INDEX_DIGITS 6
#define SEGMENT_WAV_SUFFIX ".wav"
#define SEGMENT_META_SUFFIX ".meta.json"
#define SEGMENT_TMP_SUFFIX ".tmp"
#define SEGMENT_LONGEST_SPIFFS_NAME_LEN \
    (1 + (sizeof(SEGMENT_NAME_PREFIX) - 1) + SEGMENT_INDEX_DIGITS + \
     (sizeof(SEGMENT_META_SUFFIX) - 1) + (sizeof(SEGMENT_TMP_SUFFIX) - 1))
_Static_assert(SEGMENT_LONGEST_SPIFFS_NAME_LEN <= CONFIG_SPIFFS_OBJ_NAME_LEN - 1,
               "Segment SPIFFS filenames exceed CONFIG_SPIFFS_OBJ_NAME_LEN");
#define SEGMENT_UPLOAD_QUEUE_LEN 8
#define SEGMENT_UPLOAD_TASK_STACK_SIZE 8192
#define SEGMENT_UPLOAD_TASK_PRIORITY 3
#define SEGMENT_UPLOAD_RETRY_DELAY_MS 5000
#define SEGMENT_UPLOAD_WIFI_WAIT_MS 15000
#define SEGMENT_UPLOAD_HTTP_TIMEOUT_MS 30000
#define SEGMENT_UPLOAD_CHUNK_BYTES 2048
#define SEGMENT_ACK_MAX_BYTES 2048
#define WAV_HEADER_SIZE 44
#define WAV_PCM_FORMAT 1
#define SAMPLE_RATE 16000
#define CHANNELS 2
#define BITS_PER_SAMPLE 16
#define BYTES_PER_SECOND (SAMPLE_RATE * CHANNELS * (BITS_PER_SAMPLE / 8))
#define SEGMENT_STORAGE_RESERVE_BYTES (64 * 1024)

typedef struct {
    FILE *fp;
    uint32_t data_bytes;
    uint32_t segment_index;
    uint64_t segment_start_ms;
    char path[SEGMENT_PATH_LEN];
} active_segment_t;

typedef struct {
    char path[SEGMENT_PATH_LEN];
    char run_id[96];
    uint32_t segment_index;
    uint64_t segment_start_ms;
    uint32_t data_bytes;
    uint32_t duration_ms;
    uint32_t local_gap_count;
    uint32_t storage_overflow_count;
    uint32_t upload_retry_count;
} upload_segment_t;

static SemaphoreHandle_t segment_mutex;
static QueueHandle_t upload_queue;
static TaskHandle_t upload_task_handle;
static active_segment_t active_segment;
static bool recorder_active;
static bool upload_task_started;
static audio_segment_recorder_stats_t recorder_stats;

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
    const uint32_t byte_rate = BYTES_PER_SECOND;
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

static esp_err_t write_wav_header(FILE *fp, uint32_t data_bytes)
{
    uint8_t header[WAV_HEADER_SIZE] = {0};
    build_wav_header(header, data_bytes);
    return fwrite(header, 1, sizeof(header), fp) == sizeof(header) ? ESP_OK : ESP_FAIL;
}

static void segment_lock(void)
{
    if (segment_mutex != NULL)
    {
        xSemaphoreTake(segment_mutex, portMAX_DELAY);
    }
}

static void segment_unlock(void)
{
    if (segment_mutex != NULL)
    {
        xSemaphoreGive(segment_mutex);
    }
}

static esp_err_t ensure_runtime_objects(void)
{
    if (segment_mutex == NULL)
    {
        segment_mutex = xSemaphoreCreateMutex();
        if (segment_mutex == NULL)
        {
            return ESP_ERR_NO_MEM;
        }
    }

    if (upload_queue == NULL)
    {
        upload_queue = xQueueCreate(SEGMENT_UPLOAD_QUEUE_LEN, sizeof(upload_segment_t));
        if (upload_queue == NULL)
        {
            return ESP_ERR_NO_MEM;
        }
    }

    return ESP_OK;
}

static esp_err_t ensure_segment_storage(void)
{
    DIR *dir = opendir(SEGMENT_STORAGE_ROOT);
    if (dir == NULL)
    {
        ESP_LOGE(TAG, "Failed to open segment storage root: %s", SEGMENT_STORAGE_ROOT);
        return ESP_FAIL;
    }
    closedir(dir);

    return ESP_OK;
}

static bool storage_can_accept(size_t next_write_len)
{
    size_t total = 0;
    size_t used = 0;
    esp_err_t ret = esp_spiffs_info(CONFIG_BSP_SPIFFS_PARTITION_LABEL, &total, &used);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "SPIFFS info query failed: %s", esp_err_to_name(ret));
        return false;
    }

    size_t free_now = total > used ? total - used : 0;
    return free_now >= next_write_len && free_now - next_write_len >= SEGMENT_STORAGE_RESERVE_BYTES;
}

static void make_run_id(char *dest, size_t dest_len)
{
    int written = snprintf(dest,
                           dest_len,
                           "%s-%" PRId64,
                           runtime_config_device_id(),
                           (int64_t)esp_timer_get_time());
    if (written <= 0 || written >= (int)dest_len)
    {
        snprintf(dest, dest_len, "device-run");
    }
}

static uint64_t segment_start_ms_for_index(uint32_t segment_index)
{
    if (segment_index == 0)
    {
        return 0;
    }
    return (uint64_t)(segment_index - 1) * (uint64_t)CONFIG_AUDIO_SEGMENT_SECONDS * 1000ULL;
}

static void segment_wav_path(char *dest, size_t dest_len, uint32_t segment_index)
{
    snprintf(dest,
             dest_len,
             SEGMENT_STORAGE_ROOT "/" SEGMENT_NAME_PREFIX "%06" PRIu32 SEGMENT_WAV_SUFFIX,
             segment_index);
}

static void segment_meta_path_for_index(char *dest, size_t dest_len, uint32_t segment_index)
{
    snprintf(dest,
             dest_len,
             SEGMENT_STORAGE_ROOT "/" SEGMENT_NAME_PREFIX "%06" PRIu32 SEGMENT_META_SUFFIX,
             segment_index);
}

static void segment_meta_path_for_upload(const upload_segment_t *segment, char *dest, size_t dest_len)
{
    segment_meta_path_for_index(dest, dest_len, segment->segment_index);
}

static bool parse_segment_meta_name(const char *name, uint32_t *segment_index)
{
    if (name == NULL || segment_index == NULL)
    {
        return false;
    }

    uint32_t parsed = 0;
    char suffix[sizeof(SEGMENT_META_SUFFIX)] = {0};
    if (sscanf(name, SEGMENT_NAME_PREFIX "%" SCNu32 "%10s", &parsed, suffix) != 2)
    {
        return false;
    }
    if (parsed == 0 || strcmp(suffix, SEGMENT_META_SUFFIX) != 0)
    {
        return false;
    }

    char expected[64];
    snprintf(expected, sizeof(expected), SEGMENT_NAME_PREFIX "%06" PRIu32 SEGMENT_META_SUFFIX, parsed);
    if (strcmp(name, expected) != 0)
    {
        return false;
    }

    *segment_index = parsed;
    return true;
}

static esp_err_t write_segment_metadata(const upload_segment_t *segment)
{
    if (segment == NULL || segment->segment_index == 0 || segment->run_id[0] == '\0')
    {
        return ESP_ERR_INVALID_ARG;
    }

    char meta_path[SEGMENT_PATH_LEN];
    char tmp_path[SEGMENT_PATH_LEN];
    segment_meta_path_for_upload(segment, meta_path, sizeof(meta_path));
    snprintf(tmp_path, sizeof(tmp_path), "%s" SEGMENT_TMP_SUFFIX, meta_path);

    cJSON *root = cJSON_CreateObject();
    if (root == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    esp_err_t ret = ESP_OK;
    if (cJSON_AddStringToObject(root, "type", "SEGMENT_UPLOAD_METADATA") == NULL ||
        cJSON_AddStringToObject(root, "run_id", segment->run_id) == NULL ||
        cJSON_AddNumberToObject(root, "segment_index", segment->segment_index) == NULL ||
        cJSON_AddNumberToObject(root, "segment_start_ms", (double)segment->segment_start_ms) == NULL ||
        cJSON_AddNumberToObject(root, "data_bytes", segment->data_bytes) == NULL ||
        cJSON_AddNumberToObject(root, "duration_ms", segment->duration_ms) == NULL ||
        cJSON_AddNumberToObject(root, "local_gap_count", segment->local_gap_count) == NULL ||
        cJSON_AddNumberToObject(root, "storage_overflow_count", segment->storage_overflow_count) == NULL ||
        cJSON_AddNumberToObject(root, "upload_retry_count", segment->upload_retry_count) == NULL)
    {
        ret = ESP_ERR_NO_MEM;
        goto cleanup;
    }

    char *json = cJSON_PrintUnformatted(root);
    if (json == NULL)
    {
        ret = ESP_ERR_NO_MEM;
        goto cleanup;
    }

    FILE *fp = fopen(tmp_path, "wb");
    if (fp == NULL)
    {
        free(json);
        ret = ESP_FAIL;
        goto cleanup;
    }
    size_t len = strlen(json);
    size_t written = fwrite(json, 1, len, fp);
    int close_ret = fclose(fp);
    free(json);
    if (written != len || close_ret != 0)
    {
        remove(tmp_path);
        ret = ESP_FAIL;
        goto cleanup;
    }

    remove(meta_path);
    if (rename(tmp_path, meta_path) != 0)
    {
        remove(tmp_path);
        ret = ESP_FAIL;
    }

cleanup:
    cJSON_Delete(root);
    return ret;
}

static bool json_u32(cJSON *root, const char *key, uint32_t *value)
{
    cJSON *item = cJSON_GetObjectItemCaseSensitive(root, key);
    if (!cJSON_IsNumber(item) || item->valuedouble < 0 || item->valuedouble > UINT32_MAX)
    {
        return false;
    }
    *value = (uint32_t)item->valuedouble;
    return true;
}

static bool json_u64(cJSON *root, const char *key, uint64_t *value)
{
    cJSON *item = cJSON_GetObjectItemCaseSensitive(root, key);
    if (!cJSON_IsNumber(item) || item->valuedouble < 0)
    {
        return false;
    }
    *value = (uint64_t)item->valuedouble;
    return true;
}

static esp_err_t read_segment_metadata(uint32_t segment_index, upload_segment_t *segment)
{
    if (segment == NULL || segment_index == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    char meta_path[SEGMENT_PATH_LEN];
    segment_meta_path_for_index(meta_path, sizeof(meta_path), segment_index);

    FILE *fp = fopen(meta_path, "rb");
    if (fp == NULL)
    {
        return ESP_FAIL;
    }
    if (fseek(fp, 0, SEEK_END) != 0)
    {
        fclose(fp);
        return ESP_FAIL;
    }
    long file_len = ftell(fp);
    if (file_len <= 0 || file_len > SEGMENT_ACK_MAX_BYTES || fseek(fp, 0, SEEK_SET) != 0)
    {
        fclose(fp);
        return ESP_FAIL;
    }

    char *json = calloc((size_t)file_len + 1, 1);
    if (json == NULL)
    {
        fclose(fp);
        return ESP_ERR_NO_MEM;
    }
    size_t read_len = fread(json, 1, (size_t)file_len, fp);
    fclose(fp);
    if (read_len != (size_t)file_len)
    {
        free(json);
        return ESP_FAIL;
    }

    cJSON *root = cJSON_Parse(json);
    free(json);
    if (root == NULL)
    {
        return ESP_FAIL;
    }

    esp_err_t ret = ESP_OK;
    memset(segment, 0, sizeof(*segment));
    cJSON *run_id = cJSON_GetObjectItemCaseSensitive(root, "run_id");
    cJSON *type = cJSON_GetObjectItemCaseSensitive(root, "type");
    if (!cJSON_IsString(type) ||
        strcmp(type->valuestring, "SEGMENT_UPLOAD_METADATA") != 0 ||
        !cJSON_IsString(run_id) ||
        run_id->valuestring == NULL ||
        run_id->valuestring[0] == '\0')
    {
        ret = ESP_FAIL;
        goto cleanup;
    }

    snprintf(segment->run_id, sizeof(segment->run_id), "%s", run_id->valuestring);
    segment_wav_path(segment->path, sizeof(segment->path), segment_index);
    if (!json_u32(root, "segment_index", &segment->segment_index) ||
        segment->segment_index != segment_index ||
        !json_u64(root, "segment_start_ms", &segment->segment_start_ms) ||
        !json_u32(root, "data_bytes", &segment->data_bytes) ||
        !json_u32(root, "duration_ms", &segment->duration_ms) ||
        !json_u32(root, "local_gap_count", &segment->local_gap_count) ||
        !json_u32(root, "storage_overflow_count", &segment->storage_overflow_count) ||
        !json_u32(root, "upload_retry_count", &segment->upload_retry_count))
    {
        ret = ESP_FAIL;
    }

cleanup:
    cJSON_Delete(root);
    return ret;
}

static esp_err_t open_active_segment(uint32_t segment_index)
{
    memset(&active_segment, 0, sizeof(active_segment));
    active_segment.segment_index = segment_index;
    active_segment.segment_start_ms = segment_start_ms_for_index(segment_index);
    segment_wav_path(active_segment.path, sizeof(active_segment.path), segment_index);

    struct stat st;
    if (stat(active_segment.path, &st) == 0)
    {
        ESP_LOGE(TAG,
                 "Refusing to overwrite unacked audio segment path=%s segment_index=%" PRIu32,
                 active_segment.path,
                 segment_index);
        recorder_stats.storage_overflow_count++;
        return ESP_ERR_INVALID_STATE;
    }

    active_segment.fp = fopen(active_segment.path, "wb");
    if (active_segment.fp == NULL)
    {
        ESP_LOGE(TAG, "Failed to open audio segment: %s", active_segment.path);
        recorder_stats.local_gap_count++;
        return ESP_FAIL;
    }

    esp_err_t ret = write_wav_header(active_segment.fp, 0);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to write placeholder segment WAV header");
        fclose(active_segment.fp);
        active_segment.fp = NULL;
        recorder_stats.local_gap_count++;
        return ret;
    }

    recorder_stats.segments_started++;
    recorder_stats.last_segment_index = segment_index;
    ESP_LOGI(TAG, "Started audio segment run_id=%s segment_index=%" PRIu32 " path=%s",
             recorder_stats.run_id,
             segment_index,
             active_segment.path);
    return ESP_OK;
}

static esp_err_t enqueue_finalized_segment(const upload_segment_t *segment)
{
    if (upload_queue == NULL)
    {
        recorder_stats.queue_overflow_count++;
        ESP_LOGW(TAG,
                 "Audio segment upload queue unavailable; durable file remains pending path=%s segment_index=%" PRIu32,
                 segment->path,
                 segment->segment_index);
        return ESP_OK;
    }

    if (xQueueSend(upload_queue, segment, 0) != pdPASS)
    {
        recorder_stats.queue_overflow_count++;
        ESP_LOGW(TAG,
                 "Audio segment upload queue full; durable file remains pending path=%s segment_index=%" PRIu32,
                 segment->path,
                 segment->segment_index);
        return ESP_OK;
    }

    recorder_stats.segments_enqueued++;
    return ESP_OK;
}

static esp_err_t finalize_active_segment(bool enqueue)
{
    if (active_segment.fp == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    uint32_t data_bytes = active_segment.data_bytes;
    esp_err_t ret = ESP_OK;
    if (fseek(active_segment.fp, 0, SEEK_SET) != 0)
    {
        ret = ESP_FAIL;
    }
    else
    {
        ret = write_wav_header(active_segment.fp, data_bytes);
    }

    if (ret == ESP_OK && fflush(active_segment.fp) != 0)
    {
        ret = ESP_FAIL;
    }
    if (fclose(active_segment.fp) != 0)
    {
        ret = ESP_FAIL;
    }
    active_segment.fp = NULL;

    if (ret != ESP_OK)
    {
        recorder_stats.local_gap_count++;
        ESP_LOGE(TAG,
                 "Failed to finalize segment run_id=%s segment_index=%" PRIu32,
                 recorder_stats.run_id,
                 active_segment.segment_index);
        return ret;
    }

    recorder_stats.segments_finalized++;
    if (!enqueue || data_bytes == 0)
    {
        return ESP_OK;
    }

    upload_segment_t upload = {0};
    snprintf(upload.path, sizeof(upload.path), "%s", active_segment.path);
    snprintf(upload.run_id, sizeof(upload.run_id), "%s", recorder_stats.run_id);
    upload.segment_index = active_segment.segment_index;
    upload.segment_start_ms = active_segment.segment_start_ms;
    upload.data_bytes = data_bytes;
    upload.duration_ms = (uint32_t)(((uint64_t)data_bytes * 1000ULL) / BYTES_PER_SECOND);
    upload.local_gap_count = recorder_stats.local_gap_count;
    upload.storage_overflow_count = recorder_stats.storage_overflow_count;
    upload.upload_retry_count = recorder_stats.upload_retry_count;

    ESP_LOGI(TAG,
             "Finalized audio segment run_id=%s segment_index=%" PRIu32 " duration_ms=%" PRIu32 " data_bytes=%" PRIu32,
             upload.run_id,
             upload.segment_index,
             upload.duration_ms,
             upload.data_bytes);
    ret = write_segment_metadata(&upload);
    if (ret != ESP_OK)
    {
        recorder_stats.local_gap_count++;
        ESP_LOGE(TAG,
                 "Failed to persist segment metadata run_id=%s segment_index=%" PRIu32,
                 upload.run_id,
                 upload.segment_index);
        return ret;
    }
    return enqueue_finalized_segment(&upload);
}

static esp_err_t rotate_active_segment(void)
{
    uint32_t next_index = active_segment.segment_index + 1;
    esp_err_t ret = finalize_active_segment(true);
    if (ret != ESP_OK)
    {
        return ret;
    }

    return open_active_segment(next_index);
}

static esp_err_t sha256_file(const char *path, char hex[65], size_t *file_size)
{
    FILE *fp = fopen(path, "rb");
    if (fp == NULL)
    {
        return ESP_FAIL;
    }

    psa_status_t status = psa_crypto_init();
    if (status != PSA_SUCCESS)
    {
        fclose(fp);
        return ESP_FAIL;
    }

    psa_hash_operation_t operation = PSA_HASH_OPERATION_INIT;
    status = psa_hash_setup(&operation, PSA_ALG_SHA_256);
    if (status != PSA_SUCCESS)
    {
        fclose(fp);
        return ESP_FAIL;
    }

    uint8_t buffer[SEGMENT_UPLOAD_CHUNK_BYTES];
    size_t total = 0;
    while (1)
    {
        size_t read_len = fread(buffer, 1, sizeof(buffer), fp);
        if (read_len > 0)
        {
            status = psa_hash_update(&operation, buffer, read_len);
            if (status != PSA_SUCCESS)
            {
                fclose(fp);
                psa_hash_abort(&operation);
                return ESP_FAIL;
            }
            total += read_len;
        }
        if (read_len < sizeof(buffer))
        {
            if (ferror(fp))
            {
                fclose(fp);
                psa_hash_abort(&operation);
                return ESP_FAIL;
            }
            break;
        }
    }

    uint8_t digest[32];
    size_t digest_len = 0;
    status = psa_hash_finish(&operation, digest, sizeof(digest), &digest_len);
    if (status != PSA_SUCCESS || digest_len != sizeof(digest))
    {
        fclose(fp);
        psa_hash_abort(&operation);
        return ESP_FAIL;
    }
    fclose(fp);

    for (size_t i = 0; i < sizeof(digest); ++i)
    {
        snprintf(&hex[i * 2], 3, "%02x", digest[i]);
    }
    hex[64] = '\0';
    if (file_size)
    {
        *file_size = total;
    }
    return ESP_OK;
}

static void set_header_u32(esp_http_client_handle_t client, const char *key, uint32_t value)
{
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%" PRIu32, value);
    esp_http_client_set_header(client, key, buffer);
}

static void set_header_u64(esp_http_client_handle_t client, const char *key, uint64_t value)
{
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%" PRIu64, value);
    esp_http_client_set_header(client, key, buffer);
}

static void refresh_segment_counters(upload_segment_t *segment)
{
    if (segment == NULL)
    {
        return;
    }

    segment_lock();
    if (strcmp(recorder_stats.run_id, segment->run_id) == 0)
    {
        segment->local_gap_count = recorder_stats.local_gap_count;
        segment->storage_overflow_count = recorder_stats.storage_overflow_count;
        segment->upload_retry_count = recorder_stats.upload_retry_count;
    }
    segment_unlock();
}

static bool ack_matches_upload(const char *body, const upload_segment_t *segment, const char *sha256_hex)
{
    if (body == NULL || segment == NULL || sha256_hex == NULL)
    {
        return false;
    }

    cJSON *root = cJSON_Parse(body);
    if (root == NULL)
    {
        return false;
    }

    cJSON *type = cJSON_GetObjectItemCaseSensitive(root, "type");
    cJSON *run_id = cJSON_GetObjectItemCaseSensitive(root, "run_id");
    cJSON *segment_index = cJSON_GetObjectItemCaseSensitive(root, "segment_index");
    cJSON *sha256 = cJSON_GetObjectItemCaseSensitive(root, "sha256");
    bool matches = cJSON_IsString(type) &&
                   strcmp(type->valuestring, "SEGMENT_ACK") == 0 &&
                   cJSON_IsString(run_id) &&
                   strcmp(run_id->valuestring, segment->run_id) == 0 &&
                   cJSON_IsNumber(segment_index) &&
                   (uint32_t)segment_index->valuedouble == segment->segment_index &&
                   cJSON_IsString(sha256) &&
                   strcmp(sha256->valuestring, sha256_hex) == 0;

    cJSON_Delete(root);
    return matches;
}

static esp_err_t upload_segment_file(upload_segment_t *segment)
{
    if (!audio_segment_recorder_upload_enabled())
    {
        return ESP_ERR_INVALID_STATE;
    }
    refresh_segment_counters(segment);

    esp_err_t ret = wifi_manager_wait_connected(pdMS_TO_TICKS(SEGMENT_UPLOAD_WIFI_WAIT_MS));
    if (ret != ESP_OK)
    {
        return ret;
    }

    char sha256_hex[65] = {0};
    size_t file_size = 0;
    ret = sha256_file(segment->path, sha256_hex, &file_size);
    if (ret != ESP_OK)
    {
        return ret;
    }

    FILE *fp = fopen(segment->path, "rb");
    if (fp == NULL)
    {
        return ESP_FAIL;
    }

    esp_http_client_config_t config = {
        .url = runtime_config_cloud_audio_segment_url(),
        .method = HTTP_METHOD_POST,
        .timeout_ms = SEGMENT_UPLOAD_HTTP_TIMEOUT_MS,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL)
    {
        fclose(fp);
        return ESP_ERR_NO_MEM;
    }

    esp_http_client_set_header(client, "Content-Type", "audio/wav");
    esp_http_client_set_header(client, "X-Device-Id", runtime_config_device_id());
    esp_http_client_set_header(client, "X-Run-Id", segment->run_id);
    esp_http_client_set_header(client, "X-Content-Sha256", sha256_hex);
    esp_http_client_set_header(client, "X-Collection-Token", runtime_config_collection_token());
    set_header_u32(client, "X-Segment-Index", segment->segment_index);
    set_header_u64(client, "X-Segment-Start-Ms", segment->segment_start_ms);
    set_header_u32(client, "X-Sample-Rate", SAMPLE_RATE);
    set_header_u32(client, "X-Channels", CHANNELS);
    set_header_u32(client, "X-Bits-Per-Sample", BITS_PER_SAMPLE);
    set_header_u32(client, "X-Device-Local-Gap-Count", segment->local_gap_count);
    set_header_u32(client, "X-Device-Storage-Overflow-Count", segment->storage_overflow_count);
    set_header_u32(client, "X-Upload-Retry-Count", segment->upload_retry_count);

    ret = esp_http_client_open(client, (int)file_size);
    if (ret == ESP_OK)
    {
        uint8_t buffer[SEGMENT_UPLOAD_CHUNK_BYTES];
        size_t total_written = 0;
        while (total_written < file_size)
        {
            size_t read_len = fread(buffer, 1, sizeof(buffer), fp);
            if (read_len == 0)
            {
                ret = ESP_FAIL;
                break;
            }
            int written = esp_http_client_write(client, (const char *)buffer, read_len);
            if (written < 0 || (size_t)written != read_len)
            {
                ret = ESP_FAIL;
                break;
            }
            total_written += read_len;
        }

        if (ret == ESP_OK)
        {
            int content_length = esp_http_client_fetch_headers(client);
            (void)content_length;
            int status = esp_http_client_get_status_code(client);
            if (status < 200 || status >= 300)
            {
                ESP_LOGW(TAG,
                         "Segment upload rejected status=%d run_id=%s segment_index=%" PRIu32,
                         status,
                         segment->run_id,
                         segment->segment_index);
                ret = ESP_FAIL;
            }
            else
            {
                char response[SEGMENT_ACK_MAX_BYTES] = {0};
                int response_len = esp_http_client_read_response(client, response, sizeof(response) - 1);
                if (response_len <= 0)
                {
                    ESP_LOGW(TAG,
                             "Segment upload returned no SEGMENT_ACK body run_id=%s segment_index=%" PRIu32,
                             segment->run_id,
                             segment->segment_index);
                    ret = ESP_FAIL;
                }
                else
                {
                    response[response_len] = '\0';
                    if (!ack_matches_upload(response, segment, sha256_hex))
                    {
                        ESP_LOGW(TAG,
                                 "Segment upload ACK mismatch run_id=%s segment_index=%" PRIu32,
                                 segment->run_id,
                                 segment->segment_index);
                        ret = ESP_FAIL;
                    }
                }
            }
        }
    }

    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    fclose(fp);
    return ret;
}

static void delete_uploaded_segment(const upload_segment_t *segment)
{
    char meta_path[SEGMENT_PATH_LEN];
    segment_meta_path_for_upload(segment, meta_path, sizeof(meta_path));
    if (remove(segment->path) != 0)
    {
        ESP_LOGW(TAG, "Failed to delete ACKed segment WAV: %s", segment->path);
    }
    if (remove(meta_path) != 0)
    {
        ESP_LOGW(TAG, "Failed to delete ACKed segment metadata: %s", meta_path);
    }
}

static esp_err_t find_next_pending_segment(upload_segment_t *segment)
{
    if (segment == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    DIR *dir = opendir(SEGMENT_STORAGE_ROOT);
    if (dir == NULL)
    {
        return ESP_FAIL;
    }

    bool found = false;
    uint32_t found_index = UINT32_MAX;
    upload_segment_t candidate = {0};

    struct dirent *entry = NULL;
    while ((entry = readdir(dir)) != NULL)
    {
        uint32_t segment_index = 0;
        if (!parse_segment_meta_name(entry->d_name, &segment_index))
        {
            continue;
        }
        if (segment_index >= found_index)
        {
            continue;
        }

        upload_segment_t parsed = {0};
        if (read_segment_metadata(segment_index, &parsed) != ESP_OK)
        {
            ESP_LOGW(TAG, "Ignoring unreadable segment metadata: %s", entry->d_name);
            continue;
        }

        struct stat st;
        if (stat(parsed.path, &st) != 0)
        {
            char meta_path[SEGMENT_PATH_LEN];
            segment_meta_path_for_upload(&parsed, meta_path, sizeof(meta_path));
            ESP_LOGW(TAG, "Deleting orphan segment metadata without WAV: %s", meta_path);
            remove(meta_path);
            continue;
        }

        found = true;
        found_index = segment_index;
        candidate = parsed;
    }

    closedir(dir);
    if (!found)
    {
        return ESP_ERR_NOT_FOUND;
    }

    *segment = candidate;
    return ESP_OK;
}

static void upload_segment_until_acked(upload_segment_t *segment)
{
    if (segment == NULL)
    {
        return;
    }

    while (1)
    {
        struct stat st;
        if (stat(segment->path, &st) != 0)
        {
            char meta_path[SEGMENT_PATH_LEN];
            segment_meta_path_for_upload(segment, meta_path, sizeof(meta_path));
            ESP_LOGW(TAG,
                     "Pending segment WAV missing; dropping metadata path=%s segment_index=%" PRIu32,
                     meta_path,
                     segment->segment_index);
            remove(meta_path);
            break;
        }

        esp_err_t ret = upload_segment_file(segment);
        if (ret == ESP_OK)
        {
            segment_lock();
            recorder_stats.segments_uploaded++;
            segment_unlock();
            ESP_LOGI(TAG,
                     "Segment upload ACKed; deleting local segment run_id=%s segment_index=%" PRIu32 " path=%s",
                     segment->run_id,
                     segment->segment_index,
                     segment->path);
            delete_uploaded_segment(segment);
            break;
        }

        segment_lock();
        recorder_stats.upload_retry_count++;
        recorder_stats.upload_failure_count++;
        if (strcmp(recorder_stats.run_id, segment->run_id) == 0)
        {
            segment->local_gap_count = recorder_stats.local_gap_count;
            segment->storage_overflow_count = recorder_stats.storage_overflow_count;
            segment->upload_retry_count = recorder_stats.upload_retry_count;
        }
        else
        {
            segment->upload_retry_count++;
        }
        segment_unlock();
        if (write_segment_metadata(segment) != ESP_OK)
        {
            ESP_LOGW(TAG,
                     "Failed to update retry metadata run_id=%s segment_index=%" PRIu32,
                     segment->run_id,
                     segment->segment_index);
        }
        ESP_LOGW(TAG,
                 "Segment upload failed; will retry run_id=%s segment_index=%" PRIu32 " error=%s",
                 segment->run_id,
                 segment->segment_index,
                 esp_err_to_name(ret));
        vTaskDelay(pdMS_TO_TICKS(SEGMENT_UPLOAD_RETRY_DELAY_MS));
    }
}

static void upload_task(void *pvParameters)
{
    (void)pvParameters;

    upload_segment_t segment;
    while (1)
    {
        if (xQueueReceive(upload_queue, &segment, pdMS_TO_TICKS(1000)) != pdPASS)
        {
            if (find_next_pending_segment(&segment) != ESP_OK)
            {
                continue;
            }
        }

        upload_segment_until_acked(&segment);
    }
}

static esp_err_t ensure_upload_task(void)
{
    if (upload_task_started)
    {
        return ESP_OK;
    }

    if (xTaskCreate(upload_task,
                    "seg_upload",
                    SEGMENT_UPLOAD_TASK_STACK_SIZE,
                    NULL,
                    SEGMENT_UPLOAD_TASK_PRIORITY,
                    &upload_task_handle) != pdPASS)
    {
        return ESP_ERR_NO_MEM;
    }
    upload_task_started = true;
    return ESP_OK;
}

esp_err_t audio_segment_recorder_start_pending_uploads(void)
{
    if (!audio_segment_recorder_upload_enabled())
    {
        ESP_LOGW(TAG, "Segment upload disabled by incomplete cloud upload config");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = ensure_runtime_objects();
    if (ret != ESP_OK)
    {
        return ret;
    }

    ret = ensure_segment_storage();
    if (ret != ESP_OK)
    {
        return ret;
    }

    return ensure_upload_task();
}

esp_err_t audio_segment_recorder_start(void)
{
    esp_err_t ret = ensure_runtime_objects();
    if (ret != ESP_OK)
    {
        return ret;
    }

    ret = ensure_segment_storage();
    if (ret != ESP_OK)
    {
        return ret;
    }

    if (!storage_can_accept(WAV_HEADER_SIZE + SEGMENT_UPLOAD_CHUNK_BYTES))
    {
        ESP_LOGE(TAG, "Not enough SPIFFS space to start segmented recording");
        return ESP_ERR_NO_MEM;
    }

    if (audio_segment_recorder_upload_enabled())
    {
        ret = ensure_upload_task();
        if (ret != ESP_OK)
        {
            return ret;
        }
    }

    segment_lock();
    memset(&recorder_stats, 0, sizeof(recorder_stats));
    make_run_id(recorder_stats.run_id, sizeof(recorder_stats.run_id));
    recorder_active = true;
    ret = open_active_segment(1);
    if (ret != ESP_OK)
    {
        recorder_active = false;
    }
    segment_unlock();

    ESP_LOGI(TAG,
             "Segmented recording %s run_id=%s segment_seconds=%d",
             ret == ESP_OK ? "started" : "failed",
             recorder_stats.run_id,
             CONFIG_AUDIO_SEGMENT_SECONDS);
    return ret;
}

esp_err_t audio_segment_recorder_write_pcm(const void *data, size_t len)
{
    if (data == NULL || len == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    segment_lock();
    if (!recorder_active || active_segment.fp == NULL)
    {
        segment_unlock();
        return ESP_ERR_INVALID_STATE;
    }

    if (len > UINT32_MAX - active_segment.data_bytes)
    {
        recorder_stats.local_gap_count++;
        segment_unlock();
        return ESP_ERR_INVALID_SIZE;
    }

    if (!storage_can_accept(len))
    {
        recorder_stats.storage_overflow_count++;
        recorder_stats.local_gap_count++;
        segment_unlock();
        ESP_LOGE(TAG, "SPIFFS reserve exhausted while writing active audio segment");
        return ESP_ERR_NO_MEM;
    }

    if (fwrite(data, 1, len, active_segment.fp) != len)
    {
        recorder_stats.local_gap_count++;
        segment_unlock();
        return ESP_FAIL;
    }
    active_segment.data_bytes += (uint32_t)len;

    uint32_t target_bytes = (uint32_t)CONFIG_AUDIO_SEGMENT_SECONDS * BYTES_PER_SECOND;
    esp_err_t ret = ESP_OK;
    if (active_segment.data_bytes >= target_bytes)
    {
        ret = rotate_active_segment();
    }

    segment_unlock();
    return ret;
}

esp_err_t audio_segment_recorder_stop(void)
{
    segment_lock();
    if (!recorder_active)
    {
        segment_unlock();
        return ESP_OK;
    }

    esp_err_t ret = ESP_OK;
    if (active_segment.fp != NULL)
    {
        ret = finalize_active_segment(true);
    }
    recorder_active = false;

    audio_segment_recorder_stats_t stats = recorder_stats;
    segment_unlock();

    ESP_LOGI(TAG,
             "Segmented recording stopped run_id=%s started=%" PRIu32 " finalized=%" PRIu32 " enqueued=%" PRIu32 " uploaded=%" PRIu32 " retries=%" PRIu32 " local_gap_count=%" PRIu32 " storage_overflow_count=%" PRIu32,
             stats.run_id,
             stats.segments_started,
             stats.segments_finalized,
             stats.segments_enqueued,
             stats.segments_uploaded,
             stats.upload_retry_count,
             stats.local_gap_count,
             stats.storage_overflow_count);
    return ret;
}

void audio_segment_recorder_abort(void)
{
    segment_lock();
    if (active_segment.fp != NULL)
    {
        fclose(active_segment.fp);
        active_segment.fp = NULL;
        remove(active_segment.path);
    }
    recorder_active = false;
    recorder_stats.local_gap_count++;
    segment_unlock();
}

bool audio_segment_recorder_is_active(void)
{
    segment_lock();
    bool active = recorder_active;
    segment_unlock();
    return active;
}

bool audio_segment_recorder_upload_enabled(void)
{
    return runtime_config_audio_upload_enabled();
}

void audio_segment_recorder_get_stats(audio_segment_recorder_stats_t *stats)
{
    if (stats == NULL)
    {
        return;
    }

    segment_lock();
    *stats = recorder_stats;
    segment_unlock();
}
