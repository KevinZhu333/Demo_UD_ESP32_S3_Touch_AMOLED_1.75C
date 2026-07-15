idf_component_register(
    SRCS main.c runtime_config.c wifi_manager.c audio_segment_recorder.c
    INCLUDE_DIRS .
    PRIV_REQUIRES spiffs esp_driver_sdmmc esp_wifi esp_netif esp_event esp_timer nvs_flash esp_http_client mbedtls json esp_app_format esp32_s3_touch_amoled_1_75c bsp_extra lvgl__lvgl)

set(empty_storage_image_dir "${CMAKE_CURRENT_BINARY_DIR}/empty-storage")
add_custom_target(prepare_empty_storage_image_dir
    COMMAND "${CMAKE_COMMAND}" -E remove_directory "${empty_storage_image_dir}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${empty_storage_image_dir}"
    COMMENT "Preparing empty SPIFFS source directory")

# Operator-only provisioning/reset target. Intentionally omit FLASH_IN_PROJECT so
# ordinary `idf.py flash` never overwrites pending audio in the storage partition.
spiffs_create_partition_image(
    storage "${empty_storage_image_dir}"
    DEPENDS prepare_empty_storage_image_dir)
