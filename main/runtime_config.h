#pragma once

#include <stdbool.h>

const char *runtime_config_wifi_ssid(void);
const char *runtime_config_wifi_password(void);
const char *runtime_config_cloud_wss_url(void);
const char *runtime_config_cloud_audio_segment_url(void);
const char *runtime_config_device_id(void);
const char *runtime_config_collection_token(void);

bool runtime_config_local_wav_mirror_enabled(void);
bool runtime_config_wifi_credentials_configured(void);
bool runtime_config_cloud_endpoint_configured(void);
bool runtime_config_audio_upload_enabled(void);
bool runtime_config_collection_token_configured(void);

void runtime_config_log_startup_warnings(void);
