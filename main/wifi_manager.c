#include "wifi_manager.h"

#include <inttypes.h>
#include <string.h>

#include "esp_check.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "runtime_config.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_STARTED_BIT BIT1

#define WIFI_MANAGER_MIN_RECONNECT_DELAY_MS 1000
#define WIFI_MANAGER_MAX_RECONNECT_DELAY_MS 30000
#define WIFI_MANAGER_RETRY_TASK_STACK_SIZE 3072
#define WIFI_MANAGER_RETRY_TASK_PRIORITY 3

static const char *TAG = "wifi_manager";

static EventGroupHandle_t wifi_event_group;
static esp_netif_t *wifi_sta_netif;
static TaskHandle_t reconnect_task_handle;
static uint32_t reconnect_attempt;
static bool wifi_started;
static bool wifi_configured;

static bool wifi_credentials_available(void)
{
    return runtime_config_wifi_credentials_configured();
}

static uint32_t reconnect_delay_ms(void)
{
    uint32_t delay_ms = WIFI_MANAGER_MIN_RECONNECT_DELAY_MS;
    uint32_t shift = reconnect_attempt < 5 ? reconnect_attempt : 5;

    delay_ms <<= shift;
    if (delay_ms > WIFI_MANAGER_MAX_RECONNECT_DELAY_MS)
    {
        delay_ms = WIFI_MANAGER_MAX_RECONNECT_DELAY_MS;
    }

    return delay_ms;
}

static void reconnect_task(void *pvParameters)
{
    (void)pvParameters;

    uint32_t delay_ms = reconnect_delay_ms();
    reconnect_attempt++;
    ESP_LOGI(TAG, "Reconnecting in %" PRIu32 " ms", delay_ms);
    vTaskDelay(pdMS_TO_TICKS(delay_ms));

    if (wifi_started && !wifi_manager_is_connected())
    {
        esp_err_t ret = esp_wifi_connect();
        if (ret != ESP_OK)
        {
            ESP_LOGW(TAG, "Wi-Fi reconnect request failed: %s", esp_err_to_name(ret));
        }
    }

    reconnect_task_handle = NULL;
    vTaskDelete(NULL);
}

static void schedule_reconnect(void)
{
    if (!wifi_started || !wifi_configured || reconnect_task_handle != NULL)
    {
        return;
    }

    BaseType_t ret = xTaskCreate(reconnect_task,
                                 "wifi_retry",
                                 WIFI_MANAGER_RETRY_TASK_STACK_SIZE,
                                 NULL,
                                 WIFI_MANAGER_RETRY_TASK_PRIORITY,
                                 &reconnect_task_handle);
    if (ret != pdPASS)
    {
        reconnect_task_handle = NULL;
        ESP_LOGE(TAG, "Failed to create Wi-Fi reconnect task");
    }
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    (void)arg;
    (void)event_data;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        xEventGroupSetBits(wifi_event_group, WIFI_STARTED_BIT);
        if (!wifi_configured)
        {
            ESP_LOGW(TAG, "Wi-Fi credentials are incomplete; station interface started without connecting");
            return;
        }

        reconnect_attempt = 0;
        esp_err_t ret = esp_wifi_connect();
        if (ret != ESP_OK)
        {
            ESP_LOGW(TAG, "Wi-Fi connect request failed: %s", esp_err_to_name(ret));
            schedule_reconnect();
        }
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
        ESP_LOGW(TAG, "Wi-Fi disconnected");
        schedule_reconnect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        reconnect_attempt = 0;
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        ESP_LOGI(TAG, "Wi-Fi connected, IP: " IPSTR, IP2STR(&event->ip_info.ip));
    }
}

static esp_err_t wifi_manager_nvs_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_LOGW(TAG, "NVS init needs erase: %s", esp_err_to_name(ret));
        ESP_RETURN_ON_ERROR(nvs_flash_erase(), TAG, "NVS erase failed");
        ret = nvs_flash_init();
    }

    return ret;
}

esp_err_t wifi_manager_start(void)
{
    if (wifi_started)
    {
        return ESP_OK;
    }

    if (wifi_event_group == NULL)
    {
        wifi_event_group = xEventGroupCreate();
        if (wifi_event_group == NULL)
        {
            return ESP_ERR_NO_MEM;
        }
    }

    ESP_RETURN_ON_ERROR(wifi_manager_nvs_init(), TAG, "NVS init failed");
    ESP_RETURN_ON_ERROR(esp_netif_init(), TAG, "esp_netif init failed");

    esp_err_t ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGE(TAG, "default event loop init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    if (wifi_sta_netif == NULL)
    {
        wifi_sta_netif = esp_netif_create_default_wifi_sta();
        if (wifi_sta_netif == NULL)
        {
            return ESP_FAIL;
        }
    }

    wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&init_config), TAG, "esp_wifi init failed");

    ESP_RETURN_ON_ERROR(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_START, wifi_event_handler, NULL),
                        TAG, "register STA_START handler failed");
    ESP_RETURN_ON_ERROR(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, wifi_event_handler, NULL),
                        TAG, "register STA_DISCONNECTED handler failed");
    ESP_RETURN_ON_ERROR(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL),
                        TAG, "register GOT_IP handler failed");

    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG, "set station mode failed");

    wifi_configured = wifi_credentials_available();
    if (!wifi_configured)
    {
        ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "start station failed");
        wifi_started = true;
        ESP_LOGW(TAG, "Wi-Fi station started with incomplete credentials; connection is skipped");
        return ESP_OK;
    }

    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        },
    };
    const char *ssid = runtime_config_wifi_ssid();
    const char *password = runtime_config_wifi_password();
    snprintf((char *)wifi_config.sta.ssid, sizeof(wifi_config.sta.ssid), "%s", ssid);
    snprintf((char *)wifi_config.sta.password, sizeof(wifi_config.sta.password), "%s", password);

    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &wifi_config), TAG, "set station config failed");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "start station failed");

    wifi_started = true;
    ESP_LOGI(TAG, "Wi-Fi station started for SSID '%s'", ssid);
    return ESP_OK;
}

bool wifi_manager_is_connected(void)
{
    if (wifi_event_group == NULL)
    {
        return false;
    }

    EventBits_t bits = xEventGroupGetBits(wifi_event_group);
    return (bits & WIFI_CONNECTED_BIT) != 0;
}

esp_err_t wifi_manager_wait_connected(TickType_t timeout_ticks)
{
    if (wifi_event_group == NULL || !wifi_configured)
    {
        return ESP_ERR_INVALID_STATE;
    }

    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                                           WIFI_CONNECTED_BIT,
                                           pdFALSE,
                                           pdTRUE,
                                           timeout_ticks);
    return (bits & WIFI_CONNECTED_BIT) != 0 ? ESP_OK : ESP_ERR_TIMEOUT;
}
