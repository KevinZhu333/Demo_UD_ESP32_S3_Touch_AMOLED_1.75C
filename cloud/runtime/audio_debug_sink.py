from __future__ import annotations

from pathlib import Path
import tempfile
import wave

from cloud.runtime.audio_frame import AudioFrame


class AudioDebugSink:
    """Optional local WAV sink for inspecting received PCM during development."""

    def __init__(self, session_id: str, output_dir: str | Path | None = None, enabled: bool = False):
        self.enabled = enabled
        self.path: Path | None = None
        self._wav: wave.Wave_write | None = None
        self._sample_rate: int | None = None
        self._channels: int | None = None
        self._bits_per_sample: int | None = None

        if not enabled:
            return

        base_dir = Path(output_dir) if output_dir is not None else Path(tempfile.gettempdir()) / "device-audio-debug"
        self.path = base_dir / f"{session_id}.wav"

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

        wav.writeframes(frame.payload)

    def close(self) -> None:
        if self._wav is not None:
            self._wav.close()
            self._wav = None
        self.enabled = False
