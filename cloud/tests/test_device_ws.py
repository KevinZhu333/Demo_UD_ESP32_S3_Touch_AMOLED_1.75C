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

from cloud.api.device_ws import app
from cloud.runtime.audio_frame import build_test_audio_frame


@pytest.fixture(autouse=True)
def clear_collection_token(monkeypatch):
    monkeypatch.delenv("CLOUD_COLLECTION_TOKEN", raising=False)


def _hello():
    return {
        "type": "HELLO",
        "device_id": "esp32-test",
        "device_class": "hw",
        "caps": ["audio.pcm16"],
        "collection_token": "dev-token",
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

    wav_paths = list(tmp_path.glob("*.wav"))
    assert len(wav_paths) == 1
    with wave.open(str(wav_paths[0]), "rb") as wav_file:
        assert wav_file.getnchannels() == 2
        assert wav_file.getsampwidth() == 2
        assert wav_file.getframerate() == 16000
        assert wav_file.readframes(wav_file.getnframes()) == payload


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
