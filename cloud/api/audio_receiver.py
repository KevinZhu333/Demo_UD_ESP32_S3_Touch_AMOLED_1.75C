from __future__ import annotations

import logging

from fastapi import APIRouter, Body, FastAPI, Header, HTTPException

from cloud.runtime.audio_segments import (
    AudioSegmentError,
    SegmentMetadata,
    validate_wav_segment,
    write_segment_artifact,
)
from cloud.runtime.auth import expected_collection_token


logger = logging.getLogger(__name__)
router = APIRouter()


def _validate_segment_token(token: str | None) -> None:
    if not token:
        raise HTTPException(status_code=401, detail="collection token is required")
    expected_token = expected_collection_token()
    if expected_token is None:
        raise HTTPException(status_code=503, detail="collection token is not configured")
    if token != expected_token:
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
        "audio segment accepted run_id=%s device_id=%s segment_index=%d "
        "artifact_path=%s duration_seconds=%.3f file_size_bytes=%d proof_pass=%s",
        summary["run_id"],
        summary["device_id"],
        summary["segment_index"],
        summary["artifact_path"],
        summary["duration_seconds"],
        summary["file_size_bytes"],
        summary["proof_pass"],
    )
    return summary


app = FastAPI(title="Device Audio Segment Receiver")
app.include_router(router)
