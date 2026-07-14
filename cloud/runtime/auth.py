from __future__ import annotations

import os


def expected_collection_token() -> str | None:
    token = os.getenv("CLOUD_COLLECTION_TOKEN", "").strip()
    return token or None
