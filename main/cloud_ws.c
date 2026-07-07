#include "cloud_ws.h"

#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "esp_app_desc.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_transport_ws.h"
#include "esp_websocket_client.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "runtime_config.h"
#include "wifi_manager.h"

#define CLOUD_WS_SOCKET_CONNECTED_BIT BIT0
#define CLOUD_WS_SESSION_READY_BIT BIT1

#define CLOUD_WS_OUTBOUND_QUEUE_LEN 8
#define CLOUD_WS_MAX_MESSAGE_BYTES 8192
#define CLOUD_WS_TASK_STACK_SIZE 6144
#define CLOUD_WS_TASK_PRIORITY 4
#define CLOUD_WS_CONNECT_WAIT_MS 15000
#define CLOUD_WS_RETRY_DELAY_MS 5000
#define CLOUD_WS_TX_TIMEOUT_MS 2500
#define CLOUD_WS_BUFFER_SIZE 2048
#define CLOUD_WS_PING_INTERVAL_SEC 15
#define CLOUD_WS_PONG_TIMEOUT_SEC 45

typedef enum {
    CLOUD_WS_MESSAGE_JSON,
    CLOUD_WS_MESSAGE_BINARY,
} cloud_ws_message_type_t;

typedef struct {
    cloud_ws_message_type_t type;
    uint8_t *data;
    size_t len;
} cloud_ws_message_t;

static const char *TAG = "cloud_ws";

static EventGroupHandle_t cloud_event_group;
static QueueHandle_t outbound_queue;
static TaskHandle_t cloud_task_handle;
static esp_websocket_client_handle_t ws_client;
static bool cloud_started;
static bool cloud_client_started;
// Audio ACK progress spans reconnects so the offline buffer can discard confirmed frames.
static bool cloud_have_audio_ack;
static bool cloud_logged_session_audio_ack;
static uint32_t cloud_highest_audio_ack_seq;

static bool cloud_ws_socket_connected(void)
{
    if (cloud_event_group == NULL)
    {
        return false;
    }

    EventBits_t bits = xEventGroupGetBits(cloud_event_group);
    return (bits & CLOUD_WS_SOCKET_CONNECTED_BIT) != 0;
}

static void cloud_ws_clear_connection_state(void)
{
    if (cloud_event_group != NULL)
    {
        xEventGroupClearBits(cloud_event_group, CLOUD_WS_SOCKET_CONNECTED_BIT | CLOUD_WS_SESSION_READY_BIT);
    }
    cloud_logged_session_audio_ack = false;
}

static void cloud_ws_free_message(cloud_ws_message_t *message)
{
    if (message == NULL)
    {
        return;
    }

    free(message->data);
    message->data = NULL;
    message->len = 0;
}

static void cloud_ws_flush_outbound_queue(void)
{
    if (outbound_queue == NULL)
    {
        return;
    }

    cloud_ws_message_t message;
    while (xQueueReceive(outbound_queue, &message, 0) == pdPASS)
    {
        cloud_ws_free_message(&message);
    }
}

static esp_err_t cloud_ws_enqueue_owned(cloud_ws_message_type_t type, uint8_t *data, size_t len, TickType_t timeout_ticks)
{
    if (outbound_queue == NULL)
    {
        free(data);
        return ESP_ERR_INVALID_STATE;
    }

    cloud_ws_message_t message = {
        .type = type,
        .data = data,
        .len = len,
    };

    if (xQueueSend(outbound_queue, &message, timeout_ticks) != pdPASS)
    {
        free(data);
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}

static esp_err_t cloud_ws_enqueue_copy(cloud_ws_message_type_t type, const void *data, size_t len, TickType_t timeout_ticks)
{
    if (data == NULL || len == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (len > CLOUD_WS_MAX_MESSAGE_BYTES || len > INT_MAX)
    {
        return ESP_ERR_INVALID_SIZE;
    }
    if (!cloud_ws_is_connected())
    {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t *copy = malloc(len);
    if (copy == NULL)
    {
        return ESP_ERR_NO_MEM;
    }
    memcpy(copy, data, len);

    return cloud_ws_enqueue_owned(type, copy, len, timeout_ticks);
}

static esp_err_t cloud_ws_queue_hello(void)
{
    esp_err_t ret = ESP_OK;
    cJSON *root = cJSON_CreateObject();
    cJSON *caps = cJSON_CreateArray();
    cJSON *cap = NULL;
    if (root == NULL || caps == NULL)
    {
        ret = ESP_ERR_NO_MEM;
        goto cleanup;
    }

    const esp_app_desc_t *app_desc = esp_app_get_description();
    cap = cJSON_CreateString("audio.pcm16");
    if (cJSON_AddStringToObject(root, "type", "HELLO") == NULL ||
        cJSON_AddStringToObject(root, "device_id", runtime_config_device_id()) == NULL ||
        cJSON_AddStringToObject(root, "device_class", "hw") == NULL ||
        cJSON_AddStringToObject(root, "fw_ver", app_desc != NULL ? app_desc->version : "unknown") == NULL ||
        cap == NULL)
    {
        ret = ESP_ERR_NO_MEM;
        goto cleanup;
    }

    if (!cJSON_AddItemToArray(caps, cap))
    {
        ret = ESP_ERR_NO_MEM;
        goto cleanup;
    }
    cap = NULL;

    if (!cJSON_AddItemToObject(root, "caps", caps))
    {
        ret = ESP_ERR_NO_MEM;
        goto cleanup;
    }
    caps = NULL;

    if (cJSON_AddStringToObject(root, "collection_token", runtime_config_collection_token()) == NULL)
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

    size_t len = strlen(json);
    ret = cloud_ws_enqueue_owned(CLOUD_WS_MESSAGE_JSON, (uint8_t *)json, len, 0);
    if (ret != ESP_OK)
    {
        ESP_LOGW(TAG, "Failed to queue HELLO: %s", esp_err_to_name(ret));
    }
    else
    {
        ESP_LOGI(TAG, "HELLO queued");
    }

cleanup:
    cJSON_Delete(root);
    cJSON_Delete(caps);
    cJSON_Delete(cap);
    return ret;
}

static void cloud_ws_handle_text_message(const esp_websocket_event_data_t *data)
{
    if (data == NULL ||
        data->op_code != WS_TRANSPORT_OPCODES_TEXT ||
        data->data_ptr == NULL ||
        data->payload_offset != 0 ||
        data->data_len != data->payload_len ||
        data->data_len <= 0)
    {
        return;
    }

    cJSON *root = cJSON_ParseWithLength(data->data_ptr, (size_t)data->data_len);
    if (root == NULL)
    {
        return;
    }

    cJSON *type = cJSON_GetObjectItemCaseSensitive(root, "type");
    cJSON *ack_seq = cJSON_GetObjectItemCaseSensitive(root, "ack_seq");

    if (cJSON_IsString(type) &&
        type->valuestring != NULL &&
        strcmp(type->valuestring, "HELLO_ACK") == 0)
    {
        xEventGroupSetBits(cloud_event_group, CLOUD_WS_SESSION_READY_BIT);
        ESP_LOGI(TAG, "HELLO_ACK received; WebSocket session ready");
        cJSON_Delete(root);
        return;
    }

    if (cJSON_IsString(type) &&
        type->valuestring != NULL &&
        strcmp(type->valuestring, "AUDIO_ACK") == 0 &&
        cloud_ws_is_connected() &&
        cJSON_IsNumber(ack_seq) &&
        ack_seq->valuedouble >= 0 &&
        ack_seq->valuedouble <= UINT32_MAX)
    {
        uint32_t seq = (uint32_t)ack_seq->valuedouble;
        if (!cloud_have_audio_ack || seq > cloud_highest_audio_ack_seq)
        {
            cloud_highest_audio_ack_seq = seq;
            cloud_have_audio_ack = true;
            if (!cloud_logged_session_audio_ack)
            {
                cloud_logged_session_audio_ack = true;
                ESP_LOGI(TAG, "AUDIO_ACK received seq=%" PRIu32, seq);
            }
            ESP_LOGD(TAG, "Highest audio ACK advanced to %" PRIu32, seq);
        }
    }

    cJSON_Delete(root);
}

static void cloud_ws_event_handler(void *handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    (void)handler_arg;
    (void)event_base;

    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;

    switch (event_id)
    {
    case WEBSOCKET_EVENT_CONNECTED:
        xEventGroupClearBits(cloud_event_group, CLOUD_WS_SESSION_READY_BIT);
        xEventGroupSetBits(cloud_event_group, CLOUD_WS_SOCKET_CONNECTED_BIT);
        ESP_LOGI(TAG, "WebSocket connected");
        cloud_ws_queue_hello();
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
    case WEBSOCKET_EVENT_CLOSED:
        cloud_ws_clear_connection_state();
        cloud_ws_flush_outbound_queue();
        ESP_LOGW(TAG, "WebSocket disconnected");
        break;
    case WEBSOCKET_EVENT_DATA:
        if (data != NULL)
        {
            ESP_LOGD(TAG, "WebSocket data: opcode=0x%02x len=%d", data->op_code, data->data_len);
            cloud_ws_handle_text_message(data);
        }
        break;
    case WEBSOCKET_EVENT_ERROR:
        cloud_ws_clear_connection_state();
        if (data != NULL)
        {
            ESP_LOGW(TAG, "WebSocket error: type=%d esp_err=0x%x sock_errno=%d",
                     data->error_handle.error_type,
                     data->error_handle.esp_tls_last_esp_err,
                     data->error_handle.esp_transport_sock_errno);
        }
        else
        {
            ESP_LOGW(TAG, "WebSocket error");
        }
        break;
    case WEBSOCKET_EVENT_FINISH:
        cloud_client_started = false;
        cloud_ws_clear_connection_state();
        ESP_LOGW(TAG, "WebSocket client task finished");
        break;
    default:
        break;
    }
}

static esp_err_t cloud_ws_create_client(void)
{
    if (ws_client != NULL)
    {
        return ESP_OK;
    }

    esp_websocket_client_config_t config = {
        .uri = runtime_config_cloud_wss_url(),
        .disable_auto_reconnect = false,
        .enable_close_reconnect = true,
        .task_name = "ws_client",
        .task_stack = CLOUD_WS_TASK_STACK_SIZE,
        .task_prio = CLOUD_WS_TASK_PRIORITY,
        .buffer_size = CLOUD_WS_BUFFER_SIZE,
        .ping_interval_sec = CLOUD_WS_PING_INTERVAL_SEC,
        .pingpong_timeout_sec = CLOUD_WS_PONG_TIMEOUT_SEC,
        .reconnect_timeout_ms = CLOUD_WS_RETRY_DELAY_MS,
        .network_timeout_ms = CLOUD_WS_TX_TIMEOUT_MS,
        .keep_alive_enable = true,
    };

    ws_client = esp_websocket_client_init(&config);
    if (ws_client == NULL)
    {
        return ESP_FAIL;
    }

    esp_err_t ret = esp_websocket_register_events(ws_client, WEBSOCKET_EVENT_ANY, cloud_ws_event_handler, NULL);
    if (ret != ESP_OK)
    {
        esp_websocket_client_destroy(ws_client);
        ws_client = NULL;
        return ret;
    }

    return ESP_OK;
}

static esp_err_t cloud_ws_start_client_once(void)
{
    if (ws_client == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (cloud_client_started)
    {
        return ESP_OK;
    }

    esp_err_t ret = esp_websocket_client_start(ws_client);
    if (ret == ESP_OK)
    {
        cloud_client_started = true;
        return ESP_OK;
    }

    return ret;
}

static esp_err_t cloud_ws_send_message(const cloud_ws_message_t *message)
{
    if (ws_client == NULL || message == NULL || message->data == NULL || message->len > INT_MAX)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!cloud_ws_socket_connected())
    {
        return ESP_ERR_INVALID_STATE;
    }

    int sent = -1;
    if (message->type == CLOUD_WS_MESSAGE_JSON)
    {
        sent = esp_websocket_client_send_text(ws_client, (const char *)message->data, (int)message->len,
                                              pdMS_TO_TICKS(CLOUD_WS_TX_TIMEOUT_MS));
    }
    else
    {
        sent = esp_websocket_client_send_bin(ws_client, (const char *)message->data, (int)message->len,
                                             pdMS_TO_TICKS(CLOUD_WS_TX_TIMEOUT_MS));
    }

    return sent == (int)message->len ? ESP_OK : ESP_FAIL;
}

static void cloud_ws_task(void *pvParameters)
{
    (void)pvParameters;

    while (1)
    {
        esp_err_t ret = wifi_manager_wait_connected(pdMS_TO_TICKS(CLOUD_WS_CONNECT_WAIT_MS));
        if (ret != ESP_OK)
        {
            ESP_LOGW(TAG, "Waiting for Wi-Fi before WebSocket connect");
            vTaskDelay(pdMS_TO_TICKS(CLOUD_WS_RETRY_DELAY_MS));
            continue;
        }

        ret = cloud_ws_create_client();
        if (ret != ESP_OK)
        {
            ESP_LOGW(TAG, "WebSocket client init failed: %s", esp_err_to_name(ret));
            vTaskDelay(pdMS_TO_TICKS(CLOUD_WS_RETRY_DELAY_MS));
            continue;
        }

        ret = cloud_ws_start_client_once();
        if (ret != ESP_OK)
        {
            ESP_LOGW(TAG, "WebSocket start failed: %s", esp_err_to_name(ret));
            vTaskDelay(pdMS_TO_TICKS(CLOUD_WS_RETRY_DELAY_MS));
            continue;
        }

        cloud_ws_message_t message;
        if (xQueueReceive(outbound_queue, &message, pdMS_TO_TICKS(1000)) != pdPASS)
        {
            continue;
        }

        if (!cloud_ws_socket_connected())
        {
            cloud_ws_free_message(&message);
            continue;
        }

        ret = cloud_ws_send_message(&message);
        if (ret != ESP_OK)
        {
            ESP_LOGW(TAG, "WebSocket send failed: %s", esp_err_to_name(ret));
        }
        cloud_ws_free_message(&message);
    }
}

esp_err_t cloud_ws_start(void)
{
    if (cloud_started)
    {
        return ESP_OK;
    }

    if (!runtime_config_cloud_endpoint_configured())
    {
        ESP_LOGW(TAG, "Cloud WSS URL is empty; WebSocket transport is disabled");
        return ESP_OK;
    }
    if (!runtime_config_wifi_credentials_configured())
    {
        ESP_LOGW(TAG, "Wi-Fi credentials are incomplete; WebSocket transport is disabled");
        return ESP_OK;
    }

    if (cloud_event_group == NULL)
    {
        cloud_event_group = xEventGroupCreate();
        if (cloud_event_group == NULL)
        {
            return ESP_ERR_NO_MEM;
        }
    }

    if (outbound_queue == NULL)
    {
        outbound_queue = xQueueCreate(CLOUD_WS_OUTBOUND_QUEUE_LEN, sizeof(cloud_ws_message_t));
        if (outbound_queue == NULL)
        {
            return ESP_ERR_NO_MEM;
        }
    }

    if (xTaskCreate(cloud_ws_task,
                    "cloud_ws",
                    CLOUD_WS_TASK_STACK_SIZE,
                    NULL,
                    CLOUD_WS_TASK_PRIORITY,
                    &cloud_task_handle) != pdPASS)
    {
        return ESP_ERR_NO_MEM;
    }

    cloud_started = true;
    ESP_LOGI(TAG, "WebSocket transport task started");
    return ESP_OK;
}

bool cloud_ws_is_connected(void)
{
    if (cloud_event_group == NULL)
    {
        return false;
    }

    EventBits_t bits = xEventGroupGetBits(cloud_event_group);
    return (bits & CLOUD_WS_SESSION_READY_BIT) != 0;
}

bool cloud_ws_get_highest_audio_ack(uint32_t *ack_seq)
{
    if (ack_seq == NULL || !cloud_have_audio_ack)
    {
        return false;
    }

    *ack_seq = cloud_highest_audio_ack_seq;
    return true;
}

esp_err_t cloud_ws_send_json(const char *json, TickType_t timeout_ticks)
{
    if (json == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    return cloud_ws_enqueue_copy(CLOUD_WS_MESSAGE_JSON, json, strlen(json), timeout_ticks);
}

esp_err_t cloud_ws_send_binary(const void *data, size_t len, TickType_t timeout_ticks)
{
    return cloud_ws_enqueue_copy(CLOUD_WS_MESSAGE_BINARY, data, len, timeout_ticks);
}
