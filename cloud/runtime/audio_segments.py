from __future__ import annotations

from dataclasses import dataclass
from hashlib import sha256
import json
import os
from pathlib import Path
import re
from tempfile import NamedTemporaryFile
from time import time
import wave


RUN_ID_RE = re.compile(r"[^A-Za-z0-9_.:-]+")


class AudioSegmentError(ValueError):
    """Raised when an uploaded audio segment cannot be accepted as proof."""


@dataclass(frozen=True)
class SegmentMetadata:
    device_id: str
    run_id: str
    segment_index: int
    segment_start_ms: int
    sample_rate: int
    channels: int
    bits_per_sample: int
    content_sha256: str
    device_local_gap_count: int | None = None
    device_storage_overflow_count: int | None = None
    upload_retry_count: int | None = None


def segment_output_dir() -> Path:
    return Path(os.getenv("CLOUD_AUDIO_SEGMENT_DIR", "device-audio-segments"))


def sanitize_run_id(run_id: str) -> str:
    cleaned = RUN_ID_RE.sub("_", run_id.strip())
    return cleaned or "unknown-run"


def _run_dir(run_id: str) -> Path:
    return segment_output_dir() / sanitize_run_id(run_id)


def _state_path(run_dir: Path) -> Path:
    return run_dir / ".summary_state.json"


def _read_state(run_dir: Path) -> dict[str, int]:
    path = _state_path(run_dir)
    if not path.exists():
        return {"checksum_mismatch_count": 0}
    state = json.loads(path.read_text(encoding="utf-8"))
    checksum_mismatch_count = state.get("checksum_mismatch_count")
    return {
        "checksum_mismatch_count": checksum_mismatch_count
        if isinstance(checksum_mismatch_count, int) and checksum_mismatch_count >= 0
        else 0
    }


def _write_state(run_dir: Path, state: dict[str, int]) -> None:
    _state_path(run_dir).write_text(json.dumps(state, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def record_checksum_mismatch(metadata: SegmentMetadata) -> dict[str, object]:
    run_dir = _run_dir(metadata.run_id)
    run_dir.mkdir(parents=True, exist_ok=True)
    state = _read_state(run_dir)
    state["checksum_mismatch_count"] += 1
    _write_state(run_dir, state)
    return write_run_summary(run_dir, metadata.run_id)


def validate_wav_segment(body: bytes, metadata: SegmentMetadata) -> dict[str, object]:
    if not body:
        raise AudioSegmentError("empty audio segment")

    actual_sha = sha256(body).hexdigest()
    if actual_sha != metadata.content_sha256.lower():
        raise AudioSegmentError("audio segment checksum mismatch")

    try:
        with wave.open(_BytesReader(body), "rb") as wav_file:
            channels = wav_file.getnchannels()
            sample_rate = wav_file.getframerate()
            sample_width = wav_file.getsampwidth()
            frame_count = wav_file.getnframes()
    except wave.Error as exc:
        raise AudioSegmentError(f"invalid WAV segment: {exc}") from exc

    bits_per_sample = sample_width * 8
    if channels != metadata.channels:
        raise AudioSegmentError("WAV channel count does not match segment metadata")
    if sample_rate != metadata.sample_rate:
        raise AudioSegmentError("WAV sample rate does not match segment metadata")
    if bits_per_sample != metadata.bits_per_sample:
        raise AudioSegmentError("WAV bit depth does not match segment metadata")
    if frame_count <= 0:
        raise AudioSegmentError("WAV segment contains no audio frames")

    duration_seconds = frame_count / sample_rate
    return {
        "sha256": actual_sha,
        "file_size_bytes": len(body),
        "duration_seconds": duration_seconds,
        "frame_count": frame_count,
        "sample_rate": sample_rate,
        "channels": channels,
        "bit_depth": bits_per_sample,
    }


class _BytesReader:
    def __init__(self, data: bytes):
        from io import BytesIO

        self._buffer = BytesIO(data)

    def read(self, size: int = -1) -> bytes:
        return self._buffer.read(size)

    def seek(self, offset: int, whence: int = 0) -> int:
        return self._buffer.seek(offset, whence)

    def tell(self) -> int:
        return self._buffer.tell()

    def close(self) -> None:
        self._buffer.close()


def write_segment_artifact(body: bytes, metadata: SegmentMetadata, wav_info: dict[str, object]) -> dict[str, object]:
    run_dir = _run_dir(metadata.run_id)
    run_dir.mkdir(parents=True, exist_ok=True)
    artifact_path = run_dir / f"segment_{metadata.segment_index:06d}.wav"
    summary_path = run_dir / f"segment_{metadata.segment_index:06d}.summary.json"

    if artifact_path.exists():
        existing_sha = sha256(artifact_path.read_bytes()).hexdigest()
        if existing_sha != wav_info["sha256"]:
            record_checksum_mismatch(metadata)
            raise AudioSegmentError("segment index already exists with different checksum")
    else:
        with NamedTemporaryFile("wb", delete=False, dir=run_dir) as tmp_file:
            tmp_file.write(body)
            tmp_path = Path(tmp_file.name)
        tmp_path.replace(artifact_path)

    segment_summary = {
        "type": "SEGMENT_ACK",
        "device_id": metadata.device_id,
        "run_id": metadata.run_id,
        "segment_index": metadata.segment_index,
        "segment_start_ms": metadata.segment_start_ms,
        "artifact_path": str(artifact_path),
        "summary_path": str(summary_path),
        "sha256": wav_info["sha256"],
        "file_size_bytes": wav_info["file_size_bytes"],
        "duration_seconds": wav_info["duration_seconds"],
        "frame_count": wav_info["frame_count"],
        "sample_rate": wav_info["sample_rate"],
        "channels": wav_info["channels"],
        "bit_depth": wav_info["bit_depth"],
        "device_local_gap_count": metadata.device_local_gap_count,
        "device_storage_overflow_count": metadata.device_storage_overflow_count,
        "upload_retry_count": metadata.upload_retry_count,
        "proof_pass": True,
    }
    summary_path.write_text(json.dumps(segment_summary, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    write_run_summary(run_dir, metadata.run_id)
    return segment_summary


def write_run_summary(run_dir: Path, run_id: str) -> dict[str, object]:
    segment_summaries = []
    for path in sorted(run_dir.glob("segment_*.summary.json")):
        segment_summaries.append(json.loads(path.read_text(encoding="utf-8")))
    segment_summaries.sort(key=lambda summary: int(summary["segment_index"]))

    indexes = sorted(summary["segment_index"] for summary in segment_summaries)
    missing = []
    if indexes:
        expected = set(range(1, indexes[-1] + 1))
        missing = sorted(expected - set(indexes))

    duplicate_count = len(indexes) - len(set(indexes))
    checksum_mismatch_count = _read_state(run_dir)["checksum_mismatch_count"]
    artifact_paths = [summary["artifact_path"] for summary in segment_summaries]
    received_audio_seconds = sum(float(summary["duration_seconds"]) for summary in segment_summaries)
    device_local_gap_count = _max_counter(segment_summaries, "device_local_gap_count")
    device_storage_overflow_count = _max_counter(segment_summaries, "device_storage_overflow_count")
    upload_retry_count = _max_counter(segment_summaries, "upload_retry_count")

    proof_failure_reasons = []
    if not indexes:
        proof_failure_reasons.append("no segments received")
    if missing:
        proof_failure_reasons.append("missing segments")
    if duplicate_count:
        proof_failure_reasons.append("duplicate segments")
    if checksum_mismatch_count:
        proof_failure_reasons.append("checksum mismatch")
    if device_local_gap_count is None:
        proof_failure_reasons.append("device local gap counter missing")
    elif device_local_gap_count != 0:
        proof_failure_reasons.append("device local gap counter is nonzero")
    if device_storage_overflow_count is None:
        proof_failure_reasons.append("device storage overflow counter missing")
    elif device_storage_overflow_count != 0:
        proof_failure_reasons.append("device storage overflow counter is nonzero")
    if upload_retry_count is None:
        proof_failure_reasons.append("upload retry counter missing")

    proof_pass = not proof_failure_reasons

    run_summary = {
        "run_id": run_id,
        "updated_at_epoch_seconds": time(),
        "segments_received": len(indexes),
        "first_segment_index": indexes[0] if indexes else None,
        "last_segment_index": indexes[-1] if indexes else None,
        "missing_segment_count": len(missing),
        "missing_segment_indexes": missing,
        "duplicate_segment_count": duplicate_count,
        "checksum_mismatch_count": checksum_mismatch_count,
        "received_audio_seconds": received_audio_seconds,
        "artifact_paths": artifact_paths,
        "device_local_gap_count": device_local_gap_count,
        "device_storage_overflow_count": device_storage_overflow_count,
        "upload_retry_count": upload_retry_count,
        "proof_failure_reasons": proof_failure_reasons,
        "proof_pass": proof_pass,
    }
    (run_dir / "summary.json").write_text(json.dumps(run_summary, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return run_summary


def _max_counter(segment_summaries: list[dict[str, object]], field: str) -> int | None:
    values = [summary.get(field) for summary in segment_summaries]
    counters = [value for value in values if isinstance(value, int) and not isinstance(value, bool) and value >= 0]
    if len(counters) != len(values):
        return None
    return max(counters) if counters else None
