from __future__ import annotations

import json
import logging
import os
from uuid import uuid4

from fastapi import APIRouter, FastAPI, WebSocket, WebSocketDisconnect
from starlette.websockets import WebSocketState

from cloud.runtime.audio_debug_sink import AudioDebugSink
from cloud.runtime.audio_frame import AudioFrameError, parse_audio_frame
from cloud.runtime.auth import validate_device_hello


logger = logging.getLogger(__name__)

router = APIRouter()


def _debug_audio_enabled() -> bool:
    return os.getenv("CLOUD_AUDIO_DEBUG_WAV", "").lower() in {"1", "true", "yes", "on"}


def _debug_audio_dir() -> str | None:
    return os.getenv("CLOUD_AUDIO_DEBUG_DIR")


async def _close(websocket: WebSocket, code: int, reason: str) -> None:
    if websocket.application_state != WebSocketState.DISCONNECTED:
        await websocket.close(code=code, reason=reason)


@router.websocket("/v1/device/ws")
async def device_ws(websocket: WebSocket) -> None:
    await websocket.accept()
    session_id = uuid4().hex
    sink: AudioDebugSink | None = None
    highest_seq: int | None = None
    device_id = "unknown"

    try:
        first_message = await websocket.receive()
        hello_text = first_message.get("text")
        if hello_text is None:
            await _close(websocket, 1008, "HELLO required before audio frames")
            return

        try:
            hello = json.loads(hello_text)
        except json.JSONDecodeError:
            await _close(websocket, 1008, "invalid HELLO JSON")
            return

        hello_valid, hello_reason = validate_device_hello(hello)
        if not hello_valid:
            await _close(websocket, 1008, hello_reason)
            return

        device_id = hello["device_id"]
        sink = AudioDebugSink(session_id, output_dir=_debug_audio_dir(), enabled=_debug_audio_enabled())
        logger.info("device session accepted session_id=%s device_id=%s", session_id, device_id)
        await websocket.send_json({"type": "HELLO_ACK", "session_id": session_id})

        while True:
            message = await websocket.receive()
            if message.get("type") == "websocket.disconnect":
                break

            text = message.get("text")
            if text is not None:
                try:
                    control = json.loads(text)
                except json.JSONDecodeError:
                    await _close(websocket, 1003, "invalid JSON control message")
                    return
                if isinstance(control, dict) and control.get("type") == "PING":
                    await websocket.send_json({"type": "PONG"})
                    continue
                await _close(websocket, 1003, "unsupported JSON control message")
                return

            data = message.get("bytes")
            if data is None:
                continue

            try:
                frame = parse_audio_frame(data)
                if sink is not None:
                    sink.write_frame(frame)
            except (AudioFrameError, ValueError) as exc:
                logger.warning("rejecting malformed audio frame session_id=%s device_id=%s error=%s",
                               session_id,
                               device_id,
                               exc)
                await _close(websocket, 1003, "malformed audio frame")
                return

            highest_seq = frame.seq if highest_seq is None else max(highest_seq, frame.seq)
            logger.info(
                "audio frame session_id=%s device_id=%s seq=%d bytes=%d sample_rate=%d channels=%d",
                session_id,
                device_id,
                frame.seq,
                frame.payload_bytes,
                frame.sample_rate,
                frame.channels,
            )
            await websocket.send_json({"type": "AUDIO_ACK", "ack_seq": highest_seq})
    except WebSocketDisconnect:
        pass
    finally:
        if sink is not None:
            sink.close()
        logger.info("device session closed session_id=%s device_id=%s", session_id, device_id)


app = FastAPI(title="Device Audio Debug Receiver")
app.include_router(router)
