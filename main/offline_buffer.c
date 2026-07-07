#include "offline_buffer.h"

#include <dirent.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"

#define OFFLINE_BUFFER_MAX_FRAMES 64
#define OFFLINE_BUFFER_FILE_PREFIX "ab"
#define OFFLINE_BUFFER_FILE_SUFFIX ".bin"

typedef struct {
    uint32_t seq;
    size_t len;
    char path[32];
} offline_buffer_entry_t;

static const char *TAG = "offline_buffer";

static offline_buffer_entry_t offline_entries[OFFLINE_BUFFER_MAX_FRAMES];
static size_t offline_entry_count;
static size_t offline_bytes_used;
static bool offline_initialized;
static uint32_t offline_dropped_count;

static bool seq_acked(uint32_t seq, uint32_t ack_seq)
{
    return seq <= ack_seq;
}

static void offline_buffer_path(char *path, size_t path_len, uint32_t seq)
{
    snprintf(path, path_len, "%s/" OFFLINE_BUFFER_FILE_PREFIX "%08" PRIx32 OFFLINE_BUFFER_FILE_SUFFIX,
             CONFIG_BSP_SPIFFS_MOUNT_POINT,
             seq);
}

static bool offline_buffer_name_is_buffer_file(const char *name)
{
    size_t prefix_len = strlen(OFFLINE_BUFFER_FILE_PREFIX);
    size_t suffix_len = strlen(OFFLINE_BUFFER_FILE_SUFFIX);
    size_t name_len = strlen(name);
    return name_len == prefix_len + 8 + suffix_len &&
           strncmp(name, OFFLINE_BUFFER_FILE_PREFIX, prefix_len) == 0 &&
           strcmp(&name[name_len - suffix_len], OFFLINE_BUFFER_FILE_SUFFIX) == 0;
}

static esp_err_t offline_buffer_drop_oldest(void)
{
    if (offline_entry_count == 0)
    {
        return ESP_ERR_NOT_FOUND;
    }

    offline_buffer_entry_t dropped = offline_entries[0];
    if (unlink(dropped.path) != 0)
    {
        ESP_LOGW(TAG, "Failed to delete dropped frame %s", dropped.path);
    }

    memmove(&offline_entries[0],
            &offline_entries[1],
            (offline_entry_count - 1) * sizeof(offline_entries[0]));
    offline_entry_count--;
    offline_bytes_used -= dropped.len;
    offline_dropped_count++;
    ESP_LOGW(TAG, "Dropped oldest buffered audio frame seq=%" PRIu32 " dropped_total=%" PRIu32,
             dropped.seq,
             offline_dropped_count);
    return ESP_OK;
}

static esp_err_t offline_buffer_ensure_capacity(size_t incoming_len)
{
    if (incoming_len > CONFIG_OFFLINE_BUFFER_MAX_BYTES)
    {
        return ESP_ERR_INVALID_SIZE;
    }

    while (offline_entry_count >= OFFLINE_BUFFER_MAX_FRAMES ||
           offline_bytes_used + incoming_len > CONFIG_OFFLINE_BUFFER_MAX_BYTES)
    {
        esp_err_t ret = offline_buffer_drop_oldest();
        if (ret != ESP_OK)
        {
            return ret;
        }
    }

    size_t total = 0;
    size_t used = 0;
    esp_err_t ret = esp_spiffs_info(CONFIG_BSP_SPIFFS_PARTITION_LABEL, &total, &used);
    if (ret != ESP_OK)
    {
        return ret;
    }

    while (used + incoming_len + CONFIG_OFFLINE_BUFFER_MIN_FREE_BYTES > total && offline_entry_count > 0)
    {
        ret = offline_buffer_drop_oldest();
        if (ret != ESP_OK)
        {
            return ret;
        }
        ret = esp_spiffs_info(CONFIG_BSP_SPIFFS_PARTITION_LABEL, &total, &used);
        if (ret != ESP_OK)
        {
            return ret;
        }
    }

    return used + incoming_len + CONFIG_OFFLINE_BUFFER_MIN_FREE_BYTES <= total ? ESP_OK : ESP_ERR_NO_MEM;
}

esp_err_t offline_buffer_init(void)
{
    if (offline_initialized)
    {
        return ESP_OK;
    }

    DIR *dir = opendir(CONFIG_BSP_SPIFFS_MOUNT_POINT);
    if (dir == NULL)
    {
        return ESP_FAIL;
    }

    struct dirent *entry = NULL;
    while ((entry = readdir(dir)) != NULL)
    {
        if (!offline_buffer_name_is_buffer_file(entry->d_name))
        {
            continue;
        }

        char path[32];
        snprintf(path, sizeof(path), "%s/%s", CONFIG_BSP_SPIFFS_MOUNT_POINT, entry->d_name);
        if (unlink(path) != 0)
        {
            ESP_LOGW(TAG, "Failed to remove stale buffered audio frame %s", path);
        }
    }
    closedir(dir);

    offline_entry_count = 0;
    offline_bytes_used = 0;
    offline_dropped_count = 0;
    offline_initialized = true;
    ESP_LOGI(TAG, "Offline audio buffer initialized: max_bytes=%d min_free=%d max_frames=%d",
             CONFIG_OFFLINE_BUFFER_MAX_BYTES,
             CONFIG_OFFLINE_BUFFER_MIN_FREE_BYTES,
             OFFLINE_BUFFER_MAX_FRAMES);
    return ESP_OK;
}

esp_err_t offline_buffer_push(const void *frame, size_t len, uint32_t seq)
{
    if (!offline_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (frame == NULL || len == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = offline_buffer_ensure_capacity(len);
    if (ret != ESP_OK)
    {
        return ret;
    }

    offline_buffer_entry_t new_entry = {
        .seq = seq,
        .len = len,
    };
    offline_buffer_path(new_entry.path, sizeof(new_entry.path), seq);

    FILE *file = fopen(new_entry.path, "wb");
    if (file == NULL)
    {
        return ESP_FAIL;
    }
    size_t written = fwrite(frame, 1, len, file);
    int close_ret = fclose(file);
    if (written != len || close_ret != 0)
    {
        unlink(new_entry.path);
        return ESP_FAIL;
    }

    offline_entries[offline_entry_count++] = new_entry;
    offline_bytes_used += len;
    ESP_LOGD(TAG, "Buffered audio frame seq=%" PRIu32 " len=%zu used=%zu",
             seq,
             len,
             offline_bytes_used);
    return ESP_OK;
}

esp_err_t offline_buffer_peek_next(uint8_t **frame, size_t *len, uint32_t *seq)
{
    if (!offline_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (frame == NULL || len == NULL || seq == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (offline_entry_count == 0)
    {
        return ESP_ERR_NOT_FOUND;
    }

    offline_buffer_entry_t *entry = &offline_entries[0];
    uint8_t *copy = malloc(entry->len);
    if (copy == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    FILE *file = fopen(entry->path, "rb");
    if (file == NULL)
    {
        ESP_LOGW(TAG, "Buffered frame file missing, dropping seq=%" PRIu32, entry->seq);
        free(copy);
        offline_buffer_drop_oldest();
        return ESP_FAIL;
    }
    size_t read_len = fread(copy, 1, entry->len, file);
    fclose(file);
    if (read_len != entry->len)
    {
        ESP_LOGW(TAG, "Buffered frame file truncated, dropping seq=%" PRIu32, entry->seq);
        free(copy);
        offline_buffer_drop_oldest();
        return ESP_FAIL;
    }

    *frame = copy;
    *len = entry->len;
    *seq = entry->seq;
    return ESP_OK;
}

esp_err_t offline_buffer_peek_next_seq(uint32_t *seq)
{
    if (!offline_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (seq == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (offline_entry_count == 0)
    {
        return ESP_ERR_NOT_FOUND;
    }

    *seq = offline_entries[0].seq;
    return ESP_OK;
}

esp_err_t offline_buffer_pop_acked(uint32_t ack_seq)
{
    if (!offline_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    while (offline_entry_count > 0 && seq_acked(offline_entries[0].seq, ack_seq))
    {
        offline_buffer_entry_t acked = offline_entries[0];
        if (unlink(acked.path) != 0)
        {
            ESP_LOGW(TAG, "Failed to delete acked frame %s", acked.path);
        }
        memmove(&offline_entries[0],
                &offline_entries[1],
                (offline_entry_count - 1) * sizeof(offline_entries[0]));
        offline_entry_count--;
        offline_bytes_used -= acked.len;
        ESP_LOGD(TAG, "Removed ACKed audio frame seq=%" PRIu32 " used=%zu", acked.seq, offline_bytes_used);
    }

    return ESP_OK;
}

size_t offline_buffer_bytes_used(void)
{
    return offline_bytes_used;
}
