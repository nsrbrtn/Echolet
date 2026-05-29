from __future__ import annotations

import json
from datetime import datetime, timezone
from pathlib import Path
from uuid import uuid4

from app.config import ensure_storage_dirs, get_events_root


TRANSCRIPTION_PENDING = "pending"
TRANSCRIPTION_COMPLETED = "completed"
TRANSCRIPTION_FAILED = "failed"


def _event_path(event_id: str) -> Path:
    return get_events_root() / f"{event_id}.json"


def create_event(
    *,
    device_id: str,
    created: str,
    battery: str,
    firmware_version: str | None,
    original_filename: str | None,
    stored_path: Path,
    whisper_model: str,
    transcript: str | None,
    transcription_status: str,
    transcription_error: str | None = None,
) -> str:
    ensure_storage_dirs()

    event_id = uuid4().hex
    event_payload = {
        "event_id": event_id,
        "kind": "audio_uploaded",
        "received_at": datetime.now(timezone.utc).isoformat(),
        "source": "echolet",
        "device_id": device_id,
        "type": "voice",
        "created": created,
        "battery": battery,
        "firmware_version": firmware_version,
        "original_filename": original_filename,
        "audio_path": str(stored_path),
        "whisper_model": whisper_model,
        "transcription_status": transcription_status,
        "transcript": transcript,
        "detected_command": None,
        "command_confidence": None,
        "clean_text": None,
        "status": "new",
        "transcription_error": transcription_error,
    }

    event_path = _event_path(event_id)
    event_path.write_text(json.dumps(event_payload, indent=2), encoding="utf-8")
    return event_id


def update_event_transcription(
    *,
    event_id: str,
    transcription_status: str,
    transcript: str | None,
    detected_command: str | None,
    command_confidence: str | None,
    clean_text: str | None,
    transcription_error: str | None,
) -> None:
    event_path = _event_path(event_id)
    event_payload = json.loads(event_path.read_text(encoding="utf-8"))
    event_payload["transcription_status"] = transcription_status
    event_payload["transcript"] = transcript
    event_payload["detected_command"] = detected_command
    event_payload["command_confidence"] = command_confidence
    event_payload["clean_text"] = clean_text
    event_payload["transcription_error"] = transcription_error
    event_payload["transcription_finished_at"] = datetime.now(timezone.utc).isoformat()
    event_path.write_text(json.dumps(event_payload, indent=2), encoding="utf-8")
