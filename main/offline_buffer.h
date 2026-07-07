#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

esp_err_t offline_buffer_init(void);
esp_err_t offline_buffer_push(const void *frame, size_t len, uint32_t seq);
esp_err_t offline_buffer_peek_next(uint8_t **frame, size_t *len, uint32_t *seq);
esp_err_t offline_buffer_peek_next_seq(uint32_t *seq);
esp_err_t offline_buffer_pop_acked(uint32_t ack_seq);
size_t offline_buffer_bytes_used(void);
