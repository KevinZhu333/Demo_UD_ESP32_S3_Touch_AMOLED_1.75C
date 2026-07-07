from __future__ import annotations

import os
from typing import Any


def expected_collection_token() -> str | None:
    token = os.getenv("CLOUD_COLLECTION_TOKEN", "").strip()
    return token or None


def validate_device_hello(message: Any) -> tuple[bool, str]:
    if not isinstance(message, dict):
        return False, "HELLO must be a JSON object"
    if message.get("type") != "HELLO":
        return False, "first message must be HELLO"
    if not isinstance(message.get("device_id"), str) or not message["device_id"]:
        return False, "device_id is required"
    if not isinstance(message.get("collection_token"), str) or not message["collection_token"]:
        return False, "collection_token is required"

    caps = message.get("caps")
    if not isinstance(caps, list) or "audio.pcm16" not in caps:
        return False, "audio.pcm16 capability is required"

    expected_token = expected_collection_token()
    if expected_token is not None and message["collection_token"] != expected_token:
        return False, "collection_token rejected"

    return True, "ok"
