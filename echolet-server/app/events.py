from __future__ import annotations

import json
from datetime import datetime, timezone
from pathlib import Path
from uuid import uuid4

from app.config import ensure_storage_dirs, get_events_root


def create_event(
    *,
    device_id: str,
    created_at: str,
    battery: str,
    original_filename: str | None,
    stored_path: Path,
) -> str:
    ensure_storage_dirs()

    event_id = uuid4().hex
    event_payload = {
        "event_id": event_id,
        "kind": "audio_uploaded",
        "received_at": datetime.now(timezone.utc).isoformat(),
        "device_id": device_id,
        "created_at": created_at,
        "battery": battery,
        "original_filename": original_filename,
        "stored_path": str(stored_path),
        "transcript": None,
    }

    event_path = get_events_root() / f"{event_id}.json"
    event_path.write_text(json.dumps(event_payload, indent=2), encoding="utf-8")
    return event_id
