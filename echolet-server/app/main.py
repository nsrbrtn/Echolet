from __future__ import annotations

import logging
from datetime import datetime
from pathlib import Path

from fastapi import BackgroundTasks, Depends, FastAPI, File, Form, HTTPException, UploadFile, status

from app.auth import verify_bearer_token
from app.command_detection import detect_command
from app.config import get_whisper_model
from app.events import (
    TRANSCRIPTION_COMPLETED,
    TRANSCRIPTION_FAILED,
    TRANSCRIPTION_PENDING,
    create_event,
    update_event_transcription,
)
from app.storage import save_audio_file
from app.transcription import TranscriptionError, transcribe_audio_file


app = FastAPI(title="echolet-server")
logger = logging.getLogger(__name__)


def parse_created_at(value: str) -> datetime:
    try:
        return datetime.fromisoformat(value.replace("Z", "+00:00"))
    except ValueError as exc:
        raise HTTPException(
            status_code=status.HTTP_422_UNPROCESSABLE_ENTITY,
            detail="created_at must be a valid ISO 8601 datetime",
        ) from exc


def process_transcription(event_id: str, stored_path: Path) -> None:
    try:
        transcript = transcribe_audio_file(stored_path)
    except TranscriptionError as exc:
        logger.exception("Whisper transcription failed for %s", stored_path)
        update_event_transcription(
            event_id=event_id,
            transcription_status=TRANSCRIPTION_FAILED,
            transcript=None,
            detected_command=None,
            command_confidence=None,
            clean_text=None,
            transcription_error=str(exc),
        )
        return
    except Exception as exc:  # pragma: no cover - defensive safety net
        logger.exception("Unexpected transcription failure for %s", stored_path)
        update_event_transcription(
            event_id=event_id,
            transcription_status=TRANSCRIPTION_FAILED,
            transcript=None,
            detected_command=None,
            command_confidence=None,
            clean_text=None,
            transcription_error=f"unexpected transcription error: {exc}",
        )
        return

    detection = detect_command(transcript)
    update_event_transcription(
        event_id=event_id,
        transcription_status=TRANSCRIPTION_COMPLETED,
        transcript=transcript,
        detected_command=detection.detected_command,
        command_confidence=detection.command_confidence,
        clean_text=detection.clean_text,
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
    content = await file.read()
    stored_path = save_audio_file(file, created_at_dt, content)
    logger.info("Saved audio upload to %s", stored_path)

    whisper_model = get_whisper_model()
    event_id = create_event(
        device_id=device_id,
        created=created_at,
        battery=battery,
        firmware_version=firmware_version,
        original_filename=file.filename,
        stored_path=stored_path,
        whisper_model=whisper_model,
        transcript=None,
        transcription_status=TRANSCRIPTION_PENDING,
        transcription_error=None,
    )
    background_tasks.add_task(process_transcription, event_id, stored_path)

    return {
        "ok": True,
        "status": "received",
        "event_id": event_id,
        "transcription_status": TRANSCRIPTION_PENDING,
    }
