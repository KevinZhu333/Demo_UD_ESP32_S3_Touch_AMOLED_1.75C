from __future__ import annotations

from datetime import datetime, timezone
from typing import Any

from cloud.runtime.audio_frame import AudioFrame


_DEVICE_COUNTER_FIELDS = (
    "device_audio_loss_count",
    "offline_overflow_count",
    "capture_gap_count",
)


class StreamMetrics:
    """Receiver-side debug health accounting for one audio WebSocket session."""

    def __init__(self, *, run_id: str, session_id: str, device_id: str, reconnect_count: int = 0):
        self.run_id = run_id
        self.session_id = session_id
        self.device_id = device_id
        self.started_at = datetime.now(timezone.utc)
        self.ended_at: datetime | None = None
        self.reconnect_count = reconnect_count
        self.disconnect_count = 0

        self.frames_received = 0
        self.bytes_received = 0
        self.received_audio_seconds = 0.0
        self.first_seq: int | None = None
        self.last_seq: int | None = None
        self.expected_next_seq: int | None = None
        self.highest_ack_seq: int | None = None
        self.missing_sequence_count = 0
        self.duplicate_or_old_sequence_count = 0
        self.out_of_order_sequence_count = 0

        self.sample_rate: int | None = None
        self.channels: int | None = None
        self.bit_depth: int | None = None
        self.device_audio_loss_count: int | None = None
        self.offline_overflow_count: int | None = None
        self.capture_gap_count: int | None = None
        self.device_counters_reported = False
        self.failure_reason: str | None = None

    def record_frame(self, frame: AudioFrame) -> None:
        if self.sample_rate is None:
            self.sample_rate = frame.sample_rate
            self.channels = frame.channels
            self.bit_depth = frame.bits_per_sample
        elif (
            frame.sample_rate != self.sample_rate
            or frame.channels != self.channels
            or frame.bits_per_sample != self.bit_depth
        ):
            raise ValueError("audio format changed within one stream metrics session")

        self._account_sequence(frame.seq)
        self.frames_received += 1
        self.bytes_received += frame.payload_bytes
        self.received_audio_seconds += frame.duration_ms / 1000
        self.highest_ack_seq = frame.seq if self.highest_ack_seq is None else max(self.highest_ack_seq, frame.seq)

    def _account_sequence(self, seq: int) -> None:
        if self.first_seq is None:
            self.first_seq = seq
            self.last_seq = seq
            self.expected_next_seq = seq + 1
            return

        if self.expected_next_seq is not None and seq == self.expected_next_seq:
            self.expected_next_seq = seq + 1
        elif self.expected_next_seq is not None and seq > self.expected_next_seq:
            self.missing_sequence_count += seq - self.expected_next_seq
            self.expected_next_seq = seq + 1
        else:
            self.duplicate_or_old_sequence_count += 1
            if self.last_seq is not None and seq < self.last_seq:
                self.out_of_order_sequence_count += 1

        self.last_seq = seq

    def record_device_stats(self, payload: dict[str, Any]) -> None:
        updated = False
        for field in _DEVICE_COUNTER_FIELDS:
            value = payload.get(field)
            if isinstance(value, bool):
                continue
            if isinstance(value, int) and value >= 0:
                setattr(self, field, value)
                updated = True
        if updated:
            self.device_counters_reported = all(getattr(self, field) is not None for field in _DEVICE_COUNTER_FIELDS)

    def record_disconnect(self) -> None:
        self.disconnect_count += 1

    def mark_failed(self, reason: str) -> None:
        self.failure_reason = reason

    def close(self, *, debug_summary: dict[str, object] | None = None) -> dict[str, object]:
        self.ended_at = datetime.now(timezone.utc)
        elapsed_seconds = max((self.ended_at - self.started_at).total_seconds(), 0.0)
        debug_summary = debug_summary or {}
        debug_wav_path = debug_summary.get("wav_path")
        debug_summary_path = debug_summary.get("summary_path")
        debug_wav_duration_seconds = debug_summary.get("received_audio_seconds")
        debug_wav_file_size_bytes = debug_summary.get("file_size_bytes")

        debug_failure_reasons = self._debug_failure_reasons()
        debug_pass = not debug_failure_reasons
        return {
            "run_id": self.run_id,
            "session_id": self.session_id,
            "device_id": self.device_id,
            "started_at": self.started_at.isoformat(),
            "ended_at": self.ended_at.isoformat(),
            "elapsed_seconds": elapsed_seconds,
            "frames_received": self.frames_received,
            "bytes_received": self.bytes_received,
            "received_audio_seconds": self.received_audio_seconds,
            "sample_rate": self.sample_rate,
            "channels": self.channels,
            "bit_depth": self.bit_depth,
            "first_seq": self.first_seq,
            "last_seq": self.last_seq,
            "expected_next_seq": self.expected_next_seq,
            "missing_sequence_count": self.missing_sequence_count,
            "duplicate_or_old_sequence_count": self.duplicate_or_old_sequence_count,
            "out_of_order_sequence_count": self.out_of_order_sequence_count,
            "disconnect_count": self.disconnect_count,
            "reconnect_count": self.reconnect_count,
            "highest_ack_seq": self.highest_ack_seq,
            "device_audio_loss_count": self.device_audio_loss_count,
            "offline_overflow_count": self.offline_overflow_count,
            "capture_gap_count": self.capture_gap_count,
            "device_counters_reported": self.device_counters_reported,
            "debug_wav_duration_seconds": debug_wav_duration_seconds,
            "debug_wav_file_size_bytes": debug_wav_file_size_bytes,
            "debug_wav_path": debug_wav_path,
            "debug_summary_path": debug_summary_path,
            "file_size_bytes": debug_wav_file_size_bytes,
            "has_sequence_gaps": self.missing_sequence_count > 0,
            "summary_path": debug_summary_path,
            "wav_path": debug_wav_path,
            "debug_failure_reasons": debug_failure_reasons,
            "debug_pass": debug_pass,
        }

    def _debug_failure_reasons(self) -> list[str]:
        reasons: list[str] = []
        if self.failure_reason is not None:
            reasons.append(self.failure_reason)
        if self.frames_received == 0:
            reasons.append("no audio frames received")
        if self.missing_sequence_count:
            reasons.append("missing audio sequence")
        if self.duplicate_or_old_sequence_count:
            reasons.append("duplicate or old audio sequence")
        if self.out_of_order_sequence_count:
            reasons.append("out-of-order audio sequence")
        if not self.device_counters_reported:
            reasons.append("device loss counters not reported")
        for field in _DEVICE_COUNTER_FIELDS:
            value = getattr(self, field)
            if value not in (None, 0):
                reasons.append(f"{field} is nonzero")
        return reasons
