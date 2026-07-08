from __future__ import annotations

from hashlib import sha256
from io import BytesIO
import json
import unittest
import wave

try:
    import pytest
except ImportError as exc:
    raise unittest.SkipTest("pytest is not installed") from exc

pytest.importorskip("fastapi")
pytest.importorskip("httpx")

from fastapi.testclient import TestClient

from cloud.api.device_ws import app


def _wav_bytes(*, frames: int = 16000, sample_rate: int = 16000, channels: int = 2) -> bytes:
    buffer = BytesIO()
    with wave.open(buffer, "wb") as wav_file:
        wav_file.setnchannels(channels)
        wav_file.setsampwidth(2)
        wav_file.setframerate(sample_rate)
        wav_file.writeframes(b"\x00\x00" * channels * frames)
    return buffer.getvalue()


def _headers(
    body: bytes,
    *,
    run_id: str = "proof-run",
    segment_index: int = 1,
    include_counters: bool = True,
) -> dict[str, str]:
    headers = {
        "Content-Type": "audio/wav",
        "X-Device-Id": "esp32-test",
        "X-Run-Id": run_id,
        "X-Segment-Index": str(segment_index),
        "X-Segment-Start-Ms": str((segment_index - 1) * 60000),
        "X-Sample-Rate": "16000",
        "X-Channels": "2",
        "X-Bits-Per-Sample": "16",
        "X-Content-Sha256": sha256(body).hexdigest(),
        "X-Collection-Token": "dev-token",
    }
    if include_counters:
        headers.update(
            {
                "X-Device-Local-Gap-Count": "0",
                "X-Device-Storage-Overflow-Count": "0",
                "X-Upload-Retry-Count": "0",
            }
        )
    return headers


@pytest.fixture(autouse=True)
def segment_env(monkeypatch, tmp_path):
    monkeypatch.setenv("CLOUD_AUDIO_SEGMENT_DIR", str(tmp_path))
    monkeypatch.setenv("CLOUD_COLLECTION_TOKEN", "dev-token")


def test_valid_segment_upload_creates_artifact_and_run_summary(tmp_path):
    client = TestClient(app)
    body = _wav_bytes(frames=16000)

    response = client.post("/v1/device/audio-segments", content=body, headers=_headers(body))

    assert response.status_code == 200
    ack = response.json()
    assert ack["type"] == "SEGMENT_ACK"
    assert ack["run_id"] == "proof-run"
    assert ack["segment_index"] == 1
    assert ack["duration_seconds"] == 1.0
    assert ack["proof_pass"] is True
    artifact_path = tmp_path / "proof-run" / "segment_000001.wav"
    assert artifact_path.read_bytes() == body
    run_summary = json.loads((tmp_path / "proof-run" / "summary.json").read_text(encoding="utf-8"))
    assert run_summary["segments_received"] == 1
    assert run_summary["missing_segment_count"] == 0
    assert run_summary["checksum_mismatch_count"] == 0
    assert run_summary["device_local_gap_count"] == 0
    assert run_summary["device_storage_overflow_count"] == 0
    assert run_summary["upload_retry_count"] == 0
    assert run_summary["proof_failure_reasons"] == []
    assert run_summary["proof_pass"] is True


def test_duplicate_same_checksum_is_idempotent():
    client = TestClient(app)
    body = _wav_bytes()
    headers = _headers(body, segment_index=1)

    first = client.post("/v1/device/audio-segments", content=body, headers=headers)
    second = client.post("/v1/device/audio-segments", content=body, headers=headers)

    assert first.status_code == 200
    assert second.status_code == 200
    assert second.json()["sha256"] == sha256(body).hexdigest()


def test_duplicate_different_checksum_is_rejected_and_fails_summary(tmp_path):
    client = TestClient(app)
    first_body = _wav_bytes(frames=8000)
    second_body = _wav_bytes(frames=16000)

    assert client.post("/v1/device/audio-segments", content=first_body, headers=_headers(first_body)).status_code == 200
    response = client.post("/v1/device/audio-segments", content=second_body, headers=_headers(second_body))

    assert response.status_code == 409
    assert "different checksum" in response.json()["detail"]
    run_summary = json.loads((tmp_path / "proof-run" / "summary.json").read_text(encoding="utf-8"))
    assert run_summary["checksum_mismatch_count"] == 1
    assert run_summary["proof_pass"] is False


def test_missing_segment_index_fails_run_summary(tmp_path):
    client = TestClient(app)
    body1 = _wav_bytes()
    body3 = _wav_bytes()

    assert client.post("/v1/device/audio-segments", content=body1, headers=_headers(body1, segment_index=1)).status_code == 200
    assert client.post("/v1/device/audio-segments", content=body3, headers=_headers(body3, segment_index=3)).status_code == 200

    run_summary = json.loads((tmp_path / "proof-run" / "summary.json").read_text(encoding="utf-8"))
    assert run_summary["missing_segment_count"] == 1
    assert run_summary["missing_segment_indexes"] == [2]
    assert run_summary["proof_pass"] is False


def test_missing_device_counters_accepts_segment_but_fails_run_summary(tmp_path):
    client = TestClient(app)
    body = _wav_bytes()

    response = client.post("/v1/device/audio-segments", content=body, headers=_headers(body, include_counters=False))

    assert response.status_code == 200
    run_summary = json.loads((tmp_path / "proof-run" / "summary.json").read_text(encoding="utf-8"))
    assert run_summary["device_local_gap_count"] is None
    assert run_summary["proof_pass"] is False
    assert "device local gap counter missing" in run_summary["proof_failure_reasons"]


def test_nonzero_device_counter_fails_run_summary(tmp_path):
    client = TestClient(app)
    body = _wav_bytes()
    headers = _headers(body)
    headers["X-Device-Local-Gap-Count"] = "1"

    response = client.post("/v1/device/audio-segments", content=body, headers=headers)

    assert response.status_code == 200
    run_summary = json.loads((tmp_path / "proof-run" / "summary.json").read_text(encoding="utf-8"))
    assert run_summary["device_local_gap_count"] == 1
    assert run_summary["proof_pass"] is False
    assert "device local gap counter is nonzero" in run_summary["proof_failure_reasons"]


def test_bad_checksum_bad_wav_and_missing_token_are_rejected():
    client = TestClient(app)
    body = _wav_bytes()
    headers = _headers(body)

    bad_checksum_headers = dict(headers)
    bad_checksum_headers["X-Content-Sha256"] = "0" * 64
    assert client.post("/v1/device/audio-segments", content=body, headers=bad_checksum_headers).status_code == 400

    bad_wav = b"not a wav"
    assert client.post("/v1/device/audio-segments", content=bad_wav, headers=_headers(bad_wav, segment_index=1)).status_code == 400

    missing_token_headers = dict(headers)
    missing_token_headers.pop("X-Collection-Token")
    assert client.post("/v1/device/audio-segments", content=body, headers=missing_token_headers).status_code == 401

    assert client.post("/v1/device/audio-segments", content=body, headers=_headers(body, segment_index=0)).status_code == 422
