from __future__ import annotations

import json
from pathlib import Path
import wave

from cloud.runtime.audio_frame import AudioFrame


class AudioDebugSink:
    """Optional local WAV sink for inspecting received PCM during development."""

    def __init__(self, session_id: str, output_dir: str | Path | None = None, enabled: bool = False):
        self.enabled = enabled
        self.path: Path | None = None
        self.summary_path: Path | None = None
        self._wav: wave.Wave_write | None = None
        self._sample_rate: int | None = None
        self._channels: int | None = None
        self._bits_per_sample: int | None = None
        self._frames_received = 0
        self._payload_bytes = 0
        self._received_duration_ms = 0
        self._first_seq: int | None = None
        self._last_seq: int | None = None
        self._highest_seq: int | None = None
        self._missing_sequence_count = 0
        self._duplicate_or_old_sequence_count = 0
        self._out_of_order_sequence_count = 0
        self._failure_reason: str | None = None
        self._summary: dict[str, object] | None = None

        if not enabled:
            return

        base_dir = Path(output_dir) if output_dir is not None else Path.cwd() / "device-audio-debug"
        self.path = base_dir / f"{session_id}.wav"
        self.summary_path = base_dir / f"{session_id}.summary.json"

    def _open_wav(self) -> wave.Wave_write:
        if self.path is None:
            raise RuntimeError("debug WAV path is not configured")
        if self._wav is None:
            self.path.parent.mkdir(parents=True, exist_ok=True)
            self._wav = wave.open(str(self.path), "wb")
        return self._wav

    def write_frame(self, frame: AudioFrame) -> None:
        if not self.enabled:
            return

        wav = self._open_wav()
        if self._sample_rate is None:
            self._sample_rate = frame.sample_rate
            self._channels = frame.channels
            self._bits_per_sample = frame.bits_per_sample
            wav.setnchannels(frame.channels)
            wav.setsampwidth(frame.bits_per_sample // 8)
            wav.setframerate(frame.sample_rate)
        elif (
            frame.sample_rate != self._sample_rate
            or frame.channels != self._channels
            or frame.bits_per_sample != self._bits_per_sample
        ):
            raise ValueError("audio format changed within one debug WAV session")

        self._account_sequence(frame.seq)
        wav.writeframes(frame.payload)
        self._frames_received += 1
        self._payload_bytes += frame.payload_bytes
        self._received_duration_ms += frame.duration_ms

    def _account_sequence(self, seq: int) -> None:
        if self._first_seq is None:
            self._first_seq = seq
            self._last_seq = seq
            self._highest_seq = seq
            return

        if self._highest_seq is not None and seq <= self._highest_seq:
            self._duplicate_or_old_sequence_count += 1
            if self._last_seq is not None and seq < self._last_seq:
                self._out_of_order_sequence_count += 1
        elif self._highest_seq is not None and seq > self._highest_seq + 1:
            self._missing_sequence_count += seq - self._highest_seq - 1

        self._highest_seq = max(self._highest_seq or seq, seq)
        self._last_seq = seq

    def mark_failed(self, reason: str) -> None:
        self._failure_reason = reason

    def close(self) -> dict[str, object] | None:
        if self._wav is not None:
            self._wav.close()
            self._wav = None
        self.enabled = False
        if self._frames_received == 0 or self.path is None:
            return None

        self._summary = self._build_summary()
        if self.summary_path is not None:
            self.summary_path.parent.mkdir(parents=True, exist_ok=True)
            self.summary_path.write_text(json.dumps(self._summary, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        return self._summary

    def write_summary(self, summary: dict[str, object]) -> None:
        if self.summary_path is None:
            return
        self.summary_path.parent.mkdir(parents=True, exist_ok=True)
        self.summary_path.write_text(json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    def _build_summary(self) -> dict[str, object]:
        if self.path is None:
            raise RuntimeError("debug WAV path is not configured")

        file_size_bytes = self.path.stat().st_size if self.path.exists() else 0
        has_sequence_gaps = self._missing_sequence_count > 0
        has_sequence_errors = (
            has_sequence_gaps
            or self._duplicate_or_old_sequence_count > 0
            or self._out_of_order_sequence_count > 0
        )
        debug_pass = self._failure_reason is None and not has_sequence_errors and file_size_bytes > 0
        return {
            "artifact_status": "pass" if debug_pass else "failed",
            "bit_depth": self._bits_per_sample,
            "channels": self._channels,
            "debug_pass": debug_pass,
            "duplicate_or_old_sequence_count": self._duplicate_or_old_sequence_count,
            "failure_reason": self._failure_reason,
            "file_size_bytes": file_size_bytes,
            "first_seq": self._first_seq,
            "frames_received": self._frames_received,
            "has_sequence_gaps": has_sequence_gaps,
            "last_seq": self._last_seq,
            "missing_sequence_count": self._missing_sequence_count,
            "out_of_order_sequence_count": self._out_of_order_sequence_count,
            "payload_bytes": self._payload_bytes,
            "received_audio_seconds": self._received_duration_ms / 1000,
            "sample_rate": self._sample_rate,
            "summary_path": str(self.summary_path) if self.summary_path is not None else None,
            "wav_path": str(self.path),
        }
