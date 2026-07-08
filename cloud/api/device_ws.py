from __future__ import annotations

import json
import logging
import os
from uuid import uuid4

from fastapi import APIRouter, Body, FastAPI, Header, HTTPException, WebSocket, WebSocketDisconnect
from starlette.websockets import WebSocketState

from cloud.runtime.audio_debug_sink import AudioDebugSink
from cloud.runtime.audio_frame import AudioFrameError, parse_audio_frame
from cloud.runtime.audio_segments import AudioSegmentError, SegmentMetadata, validate_wav_segment, write_segment_artifact
from cloud.runtime.auth import expected_collection_token, validate_device_hello
from cloud.runtime.stream_metrics import StreamMetrics


logger = logging.getLogger(__name__)

router = APIRouter()
_run_connection_counts: dict[str, int] = {}


def _debug_audio_enabled() -> bool:
    return os.getenv("CLOUD_AUDIO_DEBUG_WAV", "").lower() in {"1", "true", "yes", "on"}


def _debug_audio_dir() -> str | None:
    return os.getenv("CLOUD_AUDIO_DEBUG_DIR")


def _hello_run_id(hello: dict[str, object], *, session_id: str, device_id: str) -> str:
    run_id = hello.get("run_id")
    if isinstance(run_id, str) and run_id.strip():
        return run_id.strip()
    return f"{device_id}:{session_id}"


def _next_reconnect_count(run_id: str) -> int:
    count = _run_connection_counts.get(run_id, 0)
    _run_connection_counts[run_id] = count + 1
    return count


async def _close(websocket: WebSocket, code: int, reason: str) -> None:
    if websocket.application_state != WebSocketState.DISCONNECTED:
        await websocket.close(code=code, reason=reason)


def _validate_segment_token(token: str | None) -> None:
    if token is None or not token:
        raise HTTPException(status_code=401, detail="collection token is required")
    expected_token = expected_collection_token()
    if expected_token is not None and token != expected_token:
        raise HTTPException(status_code=403, detail="collection token rejected")


@router.post("/v1/device/audio-segments")
async def upload_audio_segment(
    body: bytes = Body(..., media_type="audio/wav"),
    x_device_id: str = Header(..., alias="X-Device-Id"),
    x_run_id: str = Header(..., alias="X-Run-Id"),
    x_segment_index: int = Header(..., alias="X-Segment-Index"),
    x_segment_start_ms: int = Header(..., alias="X-Segment-Start-Ms"),
    x_sample_rate: int = Header(..., alias="X-Sample-Rate"),
    x_channels: int = Header(..., alias="X-Channels"),
    x_bits_per_sample: int = Header(..., alias="X-Bits-Per-Sample"),
    x_content_sha256: str = Header(..., alias="X-Content-Sha256"),
    x_device_local_gap_count: int | None = Header(None, alias="X-Device-Local-Gap-Count"),
    x_device_storage_overflow_count: int | None = Header(None, alias="X-Device-Storage-Overflow-Count"),
    x_upload_retry_count: int | None = Header(None, alias="X-Upload-Retry-Count"),
    x_collection_token: str | None = Header(None, alias="X-Collection-Token"),
) -> dict[str, object]:
    _validate_segment_token(x_collection_token)
    if x_segment_index < 1:
        raise HTTPException(status_code=422, detail="segment index must be positive")
    for name, value in {
        "device local gap count": x_device_local_gap_count,
        "device storage overflow count": x_device_storage_overflow_count,
        "upload retry count": x_upload_retry_count,
    }.items():
        if value is not None and value < 0:
            raise HTTPException(status_code=422, detail=f"{name} must be non-negative")

    metadata = SegmentMetadata(
        device_id=x_device_id,
        run_id=x_run_id,
        segment_index=x_segment_index,
        segment_start_ms=x_segment_start_ms,
        sample_rate=x_sample_rate,
        channels=x_channels,
        bits_per_sample=x_bits_per_sample,
        content_sha256=x_content_sha256,
        device_local_gap_count=x_device_local_gap_count,
        device_storage_overflow_count=x_device_storage_overflow_count,
        upload_retry_count=x_upload_retry_count,
    )
    try:
        wav_info = validate_wav_segment(body, metadata)
        summary = write_segment_artifact(body, metadata, wav_info)
    except AudioSegmentError as exc:
        status_code = 409 if "already exists" in str(exc) else 400
        raise HTTPException(status_code=status_code, detail=str(exc)) from exc

    logger.info(
        "audio segment accepted run_id=%s device_id=%s segment_index=%d artifact_path=%s duration_seconds=%.3f file_size_bytes=%d proof_pass=%s",
        summary["run_id"],
        summary["device_id"],
        summary["segment_index"],
        summary["artifact_path"],
        summary["duration_seconds"],
        summary["file_size_bytes"],
        summary["proof_pass"],
    )
    return summary


@router.websocket("/v1/device/ws")
async def device_ws(websocket: WebSocket) -> None:
    await websocket.accept()
    session_id = uuid4().hex
    sink: AudioDebugSink | None = None
    metrics: StreamMetrics | None = None
    device_id = "unknown"
    run_id = session_id

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
        run_id = _hello_run_id(hello, session_id=session_id, device_id=device_id)
        reconnect_count = _next_reconnect_count(run_id)
        metrics = StreamMetrics(
            run_id=run_id,
            session_id=session_id,
            device_id=device_id,
            reconnect_count=reconnect_count,
        )
        sink = AudioDebugSink(session_id, output_dir=_debug_audio_dir(), enabled=_debug_audio_enabled())
        logger.info(
            "device session accepted run_id=%s session_id=%s device_id=%s reconnect_count=%d",
            run_id,
            session_id,
            device_id,
            reconnect_count,
        )
        await websocket.send_json({"type": "HELLO_ACK", "session_id": session_id, "run_id": run_id})

        while True:
            message = await websocket.receive()
            if message.get("type") == "websocket.disconnect":
                if metrics is not None:
                    metrics.record_disconnect()
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
                if isinstance(control, dict) and control.get("type") == "AUDIO_STATS":
                    if metrics is not None:
                        metrics.record_device_stats(control)
                    await websocket.send_json({"type": "AUDIO_STATS_ACK", "run_id": run_id})
                    continue
                await _close(websocket, 1003, "unsupported JSON control message")
                return

            data = message.get("bytes")
            if data is None:
                continue

            try:
                frame = parse_audio_frame(data)
                if metrics is not None:
                    metrics.record_frame(frame)
                if sink is not None:
                    sink.write_frame(frame)
            except (AudioFrameError, ValueError) as exc:
                if sink is not None:
                    sink.mark_failed(str(exc))
                if metrics is not None:
                    metrics.mark_failed(str(exc))
                logger.warning("rejecting malformed audio frame session_id=%s device_id=%s error=%s",
                               session_id,
                               device_id,
                               exc)
                await _close(websocket, 1003, "malformed audio frame")
                return

            logger.info(
                "audio frame run_id=%s session_id=%s device_id=%s seq=%d ack_seq=%s bytes=%d sample_rate=%d channels=%d",
                run_id,
                session_id,
                device_id,
                frame.seq,
                metrics.highest_ack_seq if metrics is not None else frame.seq,
                frame.payload_bytes,
                frame.sample_rate,
                frame.channels,
            )
            ack_seq = metrics.highest_ack_seq if metrics is not None else frame.seq
            await websocket.send_json({"type": "AUDIO_ACK", "ack_seq": ack_seq})
    except WebSocketDisconnect:
        if metrics is not None:
            metrics.record_disconnect()
        pass
    finally:
        debug_summary: dict[str, object] | None = None
        if sink is not None:
            debug_summary = sink.close()
        if metrics is not None:
            summary = metrics.close(debug_summary=debug_summary)
            if sink is not None and debug_summary is not None:
                sink.write_summary(summary)
            websocket.app.state.last_stream_summary = summary
            logger.info(
                "audio debug stream summary run_id=%s session_id=%s device_id=%s elapsed_seconds=%.3f "
                "received_audio_seconds=%.3f frames_received=%s bytes_received=%s first_seq=%s "
                "last_seq=%s expected_next_seq=%s missing_sequence_count=%s "
                "duplicate_or_old_sequence_count=%s out_of_order_sequence_count=%s "
                "disconnect_count=%s reconnect_count=%s highest_ack_seq=%s "
                "device_audio_loss_count=%s offline_overflow_count=%s capture_gap_count=%s "
                "debug_wav_path=%s debug_pass=%s debug_failure_reasons=%s",
                summary["run_id"],
                summary["session_id"],
                summary["device_id"],
                summary["elapsed_seconds"],
                summary["received_audio_seconds"],
                summary["frames_received"],
                summary["bytes_received"],
                summary["first_seq"],
                summary["last_seq"],
                summary["expected_next_seq"],
                summary["missing_sequence_count"],
                summary["duplicate_or_old_sequence_count"],
                summary["out_of_order_sequence_count"],
                summary["disconnect_count"],
                summary["reconnect_count"],
                summary["highest_ack_seq"],
                summary["device_audio_loss_count"],
                summary["offline_overflow_count"],
                summary["capture_gap_count"],
                summary["debug_wav_path"],
                summary["debug_pass"],
                summary["debug_failure_reasons"],
            )
            if debug_summary is not None:
                logger.info(
                    "audio debug artifact session_id=%s device_id=%s wav_path=%s summary_path=%s "
                    "sample_rate=%s channels=%s bit_depth=%s received_audio_seconds=%.3f "
                    "file_size_bytes=%s frames_received=%s missing_sequence_count=%s "
                    "duplicate_or_old_sequence_count=%s out_of_order_sequence_count=%s debug_pass=%s",
                    session_id,
                    device_id,
                    debug_summary["wav_path"],
                    debug_summary["summary_path"],
                    summary["sample_rate"],
                    summary["channels"],
                    summary["bit_depth"],
                    summary["debug_wav_duration_seconds"],
                    debug_summary["file_size_bytes"],
                    summary["frames_received"],
                    summary["missing_sequence_count"],
                    summary["duplicate_or_old_sequence_count"],
                    summary["out_of_order_sequence_count"],
                    summary["debug_pass"],
                )
        logger.info("device session closed run_id=%s session_id=%s device_id=%s", run_id, session_id, device_id)


app = FastAPI(title="Device Audio Debug Receiver")
app.include_router(router)
