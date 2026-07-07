#include "runtime_config.h"

#include "esp_log.h"

static const char *TAG = "runtime_config";

static bool config_value_present(const char *value)
{
    return value != NULL && value[0] != '\0';
}

const char *runtime_config_wifi_ssid(void)
{
    return CONFIG_WIFI_MANAGER_SSID;
}

const char *runtime_config_wifi_password(void)
{
    return CONFIG_WIFI_MANAGER_PASSWORD;
}

const char *runtime_config_cloud_wss_url(void)
{
    return CONFIG_RUNTIME_CONFIG_CLOUD_WSS_URL;
}

const char *runtime_config_device_id(void)
{
    return CONFIG_RUNTIME_CONFIG_DEVICE_ID;
}

const char *runtime_config_collection_token(void)
{
    return CONFIG_RUNTIME_CONFIG_COLLECTION_TOKEN;
}

bool runtime_config_local_wav_mirror_enabled(void)
{
    return CONFIG_RUNTIME_CONFIG_LOCAL_WAV_MIRROR;
}

bool runtime_config_wifi_credentials_configured(void)
{
    return config_value_present(runtime_config_wifi_ssid()) &&
           config_value_present(runtime_config_wifi_password());
}

bool runtime_config_cloud_endpoint_configured(void)
{
    return config_value_present(runtime_config_cloud_wss_url());
}

bool runtime_config_audio_upload_enabled(void)
{
    return runtime_config_wifi_credentials_configured() &&
           runtime_config_cloud_endpoint_configured() &&
           config_value_present(runtime_config_collection_token());
}

bool runtime_config_collection_token_configured(void)
{
    return config_value_present(runtime_config_collection_token());
}

void runtime_config_log_startup_warnings(void)
{
    if (!config_value_present(runtime_config_wifi_ssid()))
    {
        ESP_LOGW(TAG, "Wi-Fi SSID is empty; station will start offline");
    }
    else if (!config_value_present(runtime_config_wifi_password()))
    {
        ESP_LOGW(TAG, "Wi-Fi password is empty; station connection will be skipped");
    }

    if (!runtime_config_cloud_endpoint_configured())
    {
        ESP_LOGW(TAG, "Cloud WSS URL is empty; cloud transport will be skipped");
    }

    if (!runtime_config_collection_token_configured())
    {
        ESP_LOGW(TAG, "Collection token is empty; audio upload will be blocked");
    }

    if (!config_value_present(runtime_config_device_id()))
    {
        ESP_LOGW(TAG, "Device ID is empty; cloud identity is incomplete");
    }
}
