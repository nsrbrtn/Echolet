from __future__ import annotations

from datetime import datetime

from fastapi import Depends, FastAPI, File, Form, HTTPException, UploadFile, status

from app.auth import verify_bearer_token
from app.events import create_event
from app.storage import save_audio_file


app = FastAPI(title="echolet-server")


def parse_created_at(value: str) -> datetime:
    try:
        return datetime.fromisoformat(value.replace("Z", "+00:00"))
    except ValueError as exc:
        raise HTTPException(
            status_code=status.HTTP_422_UNPROCESSABLE_ENTITY,
            detail="created_at must be a valid ISO 8601 datetime",
        ) from exc


@app.post("/api/v1/upload-audio")
async def upload_audio(
    _: None = Depends(verify_bearer_token),
    file: UploadFile = File(...),
    device_id: str = Form(...),
    created_at: str = Form(...),
    battery: str = Form(...),
) -> dict[str, str | bool]:
    created_at_dt = parse_created_at(created_at)
    content = await file.read()
    stored_path = save_audio_file(file, created_at_dt, content)
    event_id = create_event(
        device_id=device_id,
        created_at=created_at,
        battery=battery,
        original_filename=file.filename,
        stored_path=stored_path,
    )

    return {
        "ok": True,
        "status": "received",
        "event_id": event_id,
    }
