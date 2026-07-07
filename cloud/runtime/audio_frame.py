from __future__ import annotations

from dataclasses import dataclass
import struct


AUDIO_MAGIC = b"AUD1"
AUDIO_HEADER_LEN = 30
AUDIO_BITS_PER_SAMPLE = 16
_HEADER_STRUCT = struct.Struct("<4sHIQIBBHI")


class AudioFrameError(ValueError):
    """Raised when a firmware audio frame does not match the v1 contract."""


@dataclass(frozen=True)
class AudioFrame:
    seq: int
    timestamp_ms: int
    sample_rate: int
    channels: int
    bits_per_sample: int
    duration_ms: int
    payload: bytes

    @property
    def payload_bytes(self) -> int:
        return len(self.payload)


def parse_audio_frame(data: bytes) -> AudioFrame:
    if len(data) < AUDIO_HEADER_LEN:
        raise AudioFrameError("audio frame shorter than v1 header")

    (
        magic,
        header_len,
        seq,
        timestamp_ms,
        sample_rate,
        channels,
        bits_per_sample,
        duration_ms,
        payload_bytes,
    ) = _HEADER_STRUCT.unpack_from(data, 0)

    if magic != AUDIO_MAGIC:
        raise AudioFrameError("invalid audio frame magic")
    if header_len < AUDIO_HEADER_LEN:
        raise AudioFrameError("audio frame header_len is smaller than v1 header")
    if header_len > len(data):
        raise AudioFrameError("audio frame header_len exceeds frame length")
    if payload_bytes != len(data) - header_len:
        raise AudioFrameError("audio frame payload length mismatch")
    if sample_rate == 0:
        raise AudioFrameError("audio frame sample_rate must be nonzero")
    if channels == 0:
        raise AudioFrameError("audio frame channels must be nonzero")
    if bits_per_sample != AUDIO_BITS_PER_SAMPLE:
        raise AudioFrameError("audio frame bits_per_sample is not PCM16")

    return AudioFrame(
        seq=seq,
        timestamp_ms=timestamp_ms,
        sample_rate=sample_rate,
        channels=channels,
        bits_per_sample=bits_per_sample,
        duration_ms=duration_ms,
        payload=bytes(data[header_len:]),
    )


def build_test_audio_frame(
    *,
    seq: int = 1,
    timestamp_ms: int = 123,
    sample_rate: int = 16000,
    channels: int = 2,
    duration_ms: int = 32,
    payload: bytes = b"\x00\x00\x01\x00",
) -> bytes:
    header = _HEADER_STRUCT.pack(
        AUDIO_MAGIC,
        AUDIO_HEADER_LEN,
        seq,
        timestamp_ms,
        sample_rate,
        channels,
        AUDIO_BITS_PER_SAMPLE,
        duration_ms,
        len(payload),
    )
    return header + payload
