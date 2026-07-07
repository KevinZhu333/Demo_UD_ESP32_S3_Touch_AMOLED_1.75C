idf_component_register(
    SRCS main.c runtime_config.c wifi_manager.c cloud_ws.c audio_streamer.c offline_buffer.c
    INCLUDE_DIRS .
    PRIV_REQUIRES spiffs esp_driver_sdmmc esp_wifi esp_netif esp_event esp_timer nvs_flash esp_websocket_client json esp_app_format esp32_s3_touch_amoled_1_75c bsp_extra lvgl__lvgl)
