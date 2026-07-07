#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"

esp_err_t cloud_ws_start(void);
// True after the WebSocket session handshake is complete and audio upload is allowed.
bool cloud_ws_is_connected(void);
bool cloud_ws_get_highest_audio_ack(uint32_t *ack_seq);
esp_err_t cloud_ws_send_json(const char *json, TickType_t timeout_ticks);
esp_err_t cloud_ws_send_binary(const void *data, size_t len, TickType_t timeout_ticks);
