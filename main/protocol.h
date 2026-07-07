#pragma once

#include <stdint.h>

#define AUDIO_STREAM_PROTOCOL_MAGIC "AUD1"
#define AUDIO_STREAM_PROTOCOL_MAGIC_LEN 4
#define AUDIO_STREAM_PROTOCOL_HEADER_LEN 30
#define AUDIO_STREAM_PROTOCOL_BITS_PER_SAMPLE 16

/*
 * Binary audio frame v1:
 * magic[4], header_len u16, seq u32, timestamp_ms u64,
 * sample_rate u32, channels u8, bits_per_sample u8, duration_ms u16,
 * payload_bytes u32, payload[payload_bytes].
 *
 * Integer fields are encoded little-endian by hand. The payload is currently
 * signed little-endian interleaved PCM16; a later Opus payload can reuse the
 * same high-level flow by adding/negotiating format metadata.
 */
