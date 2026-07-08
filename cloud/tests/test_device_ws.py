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
from starlette.websockets import WebSocketDisconnect

import cloud.api.device_ws as device_ws_module
from cloud.api.device_ws import app
from cloud.runtime.audio_frame import build_test_audio_frame


@pytest.fixture(autouse=True)
def clear_collection_token(monkeypatch):
    monkeypatch.delenv("CLOUD_COLLECTION_TOKEN", raising=False)
    device_ws_module._run_connection_counts.clear()


def _hello():
    return {
        "type": "HELLO",
        "device_id": "esp32-test",
        "device_class": "hw",
        "caps": ["card_control", "status"],
        "collection_token": "dev-token",
    }


def _zero_audio_stats():
    return {
        "type": "AUDIO_STATS",
        "device_audio_loss_count": 0,
        "offline_overflow_count": 0,
        "capture_gap_count": 0,
    }


def test_rejects_binary_before_hello():
    client = TestClient(app)

    with pytest.raises(WebSocketDisconnect):
        with client.websocket_connect("/v1/device/ws") as ws:
            ws.send_bytes(build_test_audio_frame())
            ws.receive_json()


def test_rejects_invalid_token_when_expected_token_is_configured(monkeypatch):
    monkeypatch.setenv("CLOUD_COLLECTION_TOKEN", "expected-token")
    client = TestClient(app)
    message = _hello()
    message["collection_token"] = "wrong-token"

    with pytest.raises(WebSocketDisconnect):
        with client.websocket_connect("/v1/device/ws") as ws:
            ws.send_json(message)
            ws.receive_json()


def test_accepts_hello_and_acks_audio_frame():
    client = TestClient(app)

    with client.websocket_connect("/v1/device/ws") as ws:
        ws.send_json(_hello())
        hello_ack = ws.receive_json()
        assert hello_ack["type"] == "HELLO_ACK"
        ws.send_bytes(build_test_audio_frame(seq=7))

        assert ws.receive_json() == {"type": "AUDIO_ACK", "ack_seq": 7}


def test_hello_ack_echoes_device_run_id():
    client = TestClient(app)
    hello = _hello()
    hello["run_id"] = "device-boot-run"

    with client.websocket_connect("/v1/device/ws") as ws:
        ws.send_json(hello)
        hello_ack = ws.receive_json()

    assert hello_ack["type"] == "HELLO_ACK"
    assert hello_ack["run_id"] == "device-boot-run"


def test_format_change_rejected_without_counting_bad_frame():
    client = TestClient(app)

    with pytest.raises(WebSocketDisconnect):
        with client.websocket_connect("/v1/device/ws") as ws:
            ws.send_json(_hello())
            hello_ack = ws.receive_json()
            assert hello_ack["type"] == "HELLO_ACK"
            ws.send_bytes(build_test_audio_frame(seq=1, channels=2))
            assert ws.receive_json() == {"type": "AUDIO_ACK", "ack_seq": 1}
            ws.send_bytes(build_test_audio_frame(seq=2, channels=1))
            ws.receive_json()

    summary = app.state.last_stream_summary
    assert summary["frames_received"] == 1
    assert summary["last_seq"] == 1
    assert summary["expected_next_seq"] == 2
    assert summary["debug_pass"] is False
    assert "audio format changed within one stream metrics session" in summary["debug_failure_reasons"]


def test_debug_wav_enabled_empty_session_closes_without_traceback(monkeypatch, tmp_path):
    monkeypatch.setenv("CLOUD_AUDIO_DEBUG_WAV", "1")
    monkeypatch.setenv("CLOUD_AUDIO_DEBUG_DIR", str(tmp_path))
    client = TestClient(app)

    with client.websocket_connect("/v1/device/ws") as ws:
        ws.send_json(_hello())
        hello_ack = ws.receive_json()
        assert hello_ack["type"] == "HELLO_ACK"

    assert list(tmp_path.glob("*.wav")) == []


def test_debug_wav_enabled_rejected_session_closes_without_traceback(monkeypatch, tmp_path):
    monkeypatch.setenv("CLOUD_AUDIO_DEBUG_WAV", "1")
    monkeypatch.setenv("CLOUD_AUDIO_DEBUG_DIR", str(tmp_path))
    client = TestClient(app)

    with pytest.raises(WebSocketDisconnect):
        with client.websocket_connect("/v1/device/ws") as ws:
            ws.send_bytes(build_test_audio_frame())
            ws.receive_json()

    assert list(tmp_path.glob("*.wav")) == []


def test_debug_wav_enabled_valid_audio_session_writes_wav(monkeypatch, tmp_path):
    monkeypatch.setenv("CLOUD_AUDIO_DEBUG_WAV", "1")
    monkeypatch.setenv("CLOUD_AUDIO_DEBUG_DIR", str(tmp_path))
    client = TestClient(app)
    payload = b"\x00\x00\x01\x00\x02\x00\x03\x00"

    with client.websocket_connect("/v1/device/ws") as ws:
        ws.send_json(_hello())
        hello_ack = ws.receive_json()
        assert hello_ack["type"] == "HELLO_ACK"
        ws.send_bytes(build_test_audio_frame(seq=7, payload=payload))
        assert ws.receive_json() == {"type": "AUDIO_ACK", "ack_seq": 7}
        ws.send_json(_zero_audio_stats())
        assert ws.receive_json()["type"] == "AUDIO_STATS_ACK"

    wav_paths = list(tmp_path.glob("*.wav"))
    summary_paths = list(tmp_path.glob("*.summary.json"))
    assert len(wav_paths) == 1
    assert len(summary_paths) == 1
    summary = json.loads(summary_paths[0].read_text(encoding="utf-8"))
    assert summary["wav_path"] == str(wav_paths[0])
    assert summary["summary_path"] == str(summary_paths[0])
    assert summary["sample_rate"] == 16000
    assert summary["channels"] == 2
    assert summary["bit_depth"] == 16
    assert summary["frames_received"] == 1
    assert summary["received_audio_seconds"] == 0.032
    assert summary["file_size_bytes"] > 0
    assert summary["missing_sequence_count"] == 0
    assert summary["duplicate_or_old_sequence_count"] == 0
    assert summary["device_audio_loss_count"] == 0
    assert summary["offline_overflow_count"] == 0
    assert summary["capture_gap_count"] == 0
    assert summary["debug_pass"] is True
    with wave.open(str(wav_paths[0]), "rb") as wav_file:
        assert wav_file.getnchannels() == 2
        assert wav_file.getsampwidth() == 2
        assert wav_file.getframerate() == 16000
        assert wav_file.readframes(wav_file.getnframes()) == payload


def test_debug_wav_gap_session_writes_failed_summary(monkeypatch, tmp_path):
    monkeypatch.setenv("CLOUD_AUDIO_DEBUG_WAV", "1")
    monkeypatch.setenv("CLOUD_AUDIO_DEBUG_DIR", str(tmp_path))
    client = TestClient(app)

    with client.websocket_connect("/v1/device/ws") as ws:
        ws.send_json(_hello())
        hello_ack = ws.receive_json()
        assert hello_ack["type"] == "HELLO_ACK"
        ws.send_bytes(build_test_audio_frame(seq=1))
        assert ws.receive_json() == {"type": "AUDIO_ACK", "ack_seq": 1}
        ws.send_bytes(build_test_audio_frame(seq=3))
        assert ws.receive_json() == {"type": "AUDIO_ACK", "ack_seq": 3}
        ws.send_json(_zero_audio_stats())
        assert ws.receive_json()["type"] == "AUDIO_STATS_ACK"

    summary_paths = list(tmp_path.glob("*.summary.json"))
    assert len(summary_paths) == 1
    summary = json.loads(summary_paths[0].read_text(encoding="utf-8"))
    assert summary["missing_sequence_count"] == 1
    assert summary["has_sequence_gaps"] is True
    assert summary["debug_pass"] is False
    assert "missing audio sequence" in summary["debug_failure_reasons"]


def test_stream_summary_without_debug_wav_reports_required_debug_fields():
    client = TestClient(app)

    with client.websocket_connect("/v1/device/ws") as ws:
        ws.send_json(_hello())
        hello_ack = ws.receive_json()
        assert hello_ack["type"] == "HELLO_ACK"
        ws.send_bytes(build_test_audio_frame(seq=1))
        assert ws.receive_json() == {"type": "AUDIO_ACK", "ack_seq": 1}
        ws.send_bytes(build_test_audio_frame(seq=2))
        assert ws.receive_json() == {"type": "AUDIO_ACK", "ack_seq": 2}
        ws.send_json(_zero_audio_stats())
        assert ws.receive_json()["type"] == "AUDIO_STATS_ACK"

    summary = app.state.last_stream_summary
    assert summary["frames_received"] == 2
    assert summary["bytes_received"] > 0
    assert summary["received_audio_seconds"] == 0.064
    assert summary["first_seq"] == 1
    assert summary["last_seq"] == 2
    assert summary["expected_next_seq"] == 3
    assert summary["highest_ack_seq"] == 2
    assert summary["missing_sequence_count"] == 0
    assert summary["duplicate_or_old_sequence_count"] == 0
    assert summary["out_of_order_sequence_count"] == 0
    assert summary["disconnect_count"] == 1
    assert summary["reconnect_count"] == 0
    assert summary["device_audio_loss_count"] == 0
    assert summary["offline_overflow_count"] == 0
    assert summary["capture_gap_count"] == 0
    assert summary["debug_wav_path"] is None
    assert summary["debug_pass"] is True


def test_stream_summary_without_device_stats_fails_debug_health():
    client = TestClient(app)

    with client.websocket_connect("/v1/device/ws") as ws:
        ws.send_json(_hello())
        hello_ack = ws.receive_json()
        assert hello_ack["type"] == "HELLO_ACK"
        ws.send_bytes(build_test_audio_frame(seq=1))
        assert ws.receive_json() == {"type": "AUDIO_ACK", "ack_seq": 1}

    summary = app.state.last_stream_summary
    assert summary["device_counters_reported"] is False
    assert summary["debug_pass"] is False
    assert "device loss counters not reported" in summary["debug_failure_reasons"]


def test_reconnect_count_uses_run_id_from_hello():
    client = TestClient(app)
    hello = _hello()
    hello["run_id"] = "proof-run-123"

    with client.websocket_connect("/v1/device/ws") as ws:
        ws.send_json(hello)
        hello_ack = ws.receive_json()
        assert hello_ack["run_id"] == "proof-run-123"

    with client.websocket_connect("/v1/device/ws") as ws:
        ws.send_json(hello)
        hello_ack = ws.receive_json()
        assert hello_ack["run_id"] == "proof-run-123"

    summary = app.state.last_stream_summary
    assert summary["run_id"] == "proof-run-123"
    assert summary["reconnect_count"] == 1


def test_acks_highest_sequence_seen():
    client = TestClient(app)

    with client.websocket_connect("/v1/device/ws") as ws:
        ws.send_json(_hello())
        hello_ack = ws.receive_json()
        assert hello_ack["type"] == "HELLO_ACK"
        ws.send_bytes(build_test_audio_frame(seq=2))
        assert ws.receive_json() == {"type": "AUDIO_ACK", "ack_seq": 2}
        ws.send_bytes(build_test_audio_frame(seq=5))
        assert ws.receive_json() == {"type": "AUDIO_ACK", "ack_seq": 5}
        ws.send_bytes(build_test_audio_frame(seq=4))
        assert ws.receive_json() == {"type": "AUDIO_ACK", "ack_seq": 5}


def test_json_ping_gets_pong_after_hello():
    client = TestClient(app)

    with client.websocket_connect("/v1/device/ws") as ws:
        ws.send_json(_hello())
        hello_ack = ws.receive_json()
        assert hello_ack["type"] == "HELLO_ACK"
        ws.send_json({"type": "PING"})

        assert ws.receive_json() == {"type": "PONG"}
