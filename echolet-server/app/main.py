from __future__ import annotations

import logging
from datetime import datetime, timezone
from pathlib import Path

from fastapi import BackgroundTasks, Depends, FastAPI, File, Form, HTTPException, UploadFile, status

from app.auth import verify_bearer_token
from app.config import get_echolet_inbox_dir, get_whisper_model
from app.inbox import create_obsidian_capture
from app.storage import save_audio_file
from app.transcription import TranscriptionError, transcribe_audio_file


app = FastAPI(title="echolet-server")
logger = logging.getLogger(__name__)
TRANSCRIPTION_PENDING = "pending"


def parse_created_at(value: str) -> datetime:
    try:
        return datetime.fromisoformat(value.replace("Z", "+00:00"))
    except ValueError as exc:
        raise HTTPException(
            status_code=status.HTTP_422_UNPROCESSABLE_ENTITY,
            detail="created_at must be a valid ISO 8601 datetime",
        ) from exc


def process_transcription(
    stored_path: Path,
    original_filename: str | None,
    created_at: datetime,
    received_at: datetime,
    device_id: str,
    battery: str,
    firmware_version: str | None,
) -> None:
    try:
        transcript = transcribe_audio_file(stored_path)
    except TranscriptionError as exc:
        logger.exception("Whisper transcription failed for %s", stored_path)
        create_obsidian_capture(
            stored_path=stored_path,
            original_filename=original_filename,
            created_at=created_at,
            received_at=received_at,
            device_id=device_id,
            battery=battery,
            firmware_version=firmware_version,
            transcript=None,
            transcription_error=str(exc),
        )
        return
    except Exception as exc:  # pragma: no cover - defensive safety net
        logger.exception("Unexpected transcription failure for %s", stored_path)
        create_obsidian_capture(
            stored_path=stored_path,
            original_filename=original_filename,
            created_at=created_at,
            received_at=received_at,
            device_id=device_id,
            battery=battery,
            firmware_version=firmware_version,
            transcript=None,
            transcription_error=f"unexpected transcription error: {exc}",
        )
        return

    create_obsidian_capture(
        stored_path=stored_path,
        original_filename=original_filename,
        created_at=created_at,
        received_at=received_at,
        device_id=device_id,
        battery=battery,
        firmware_version=firmware_version,
        transcript=transcript,
        transcription_error=None,
    )


@app.post("/api/v1/upload-audio")
async def upload_audio(
    background_tasks: BackgroundTasks,
    _: None = Depends(verify_bearer_token),
    file: UploadFile = File(...),
    device_id: str = Form(...),
    created_at: str = Form(...),
    battery: str = Form(...),
    firmware_version: str = Form(...),
) -> dict[str, str | bool]:
    created_at_dt = parse_created_at(created_at)
    received_at = datetime.now(timezone.utc)
    content = await file.read()
    stored_path = save_audio_file(file, created_at_dt, content)
    logger.info("Saved audio upload to %s", stored_path)

    capture_id = f"echolet-{received_at.strftime('%Y%m%d%H%M%S')}"
    background_tasks.add_task(
        process_transcription,
        stored_path,
        file.filename,
        created_at_dt,
        received_at,
        device_id,
        battery,
        firmware_version,
    )

    return {
        "ok": True,
        "status": "received",
        "event_id": capture_id,
        "transcription_status": TRANSCRIPTION_PENDING,
        "whisper_model": get_whisper_model(),
        "inbox_dir": get_echolet_inbox_dir().as_posix(),
    }
