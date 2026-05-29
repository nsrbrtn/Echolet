from __future__ import annotations

from datetime import datetime
from pathlib import Path
from uuid import uuid4

from fastapi import UploadFile

from app.config import ensure_storage_dirs, get_audio_root


def _safe_suffix(filename: str | None) -> str:
    if not filename:
        return ".bin"

    suffix = Path(filename).suffix
    return suffix if suffix else ".bin"


def save_audio_file(upload_file: UploadFile, created_at: datetime, content: bytes) -> Path:
    ensure_storage_dirs()

    target_dir = get_audio_root() / created_at.strftime("%Y") / created_at.strftime("%m")
    target_dir.mkdir(parents=True, exist_ok=True)

    filename = f"{created_at.strftime('%Y%m%dT%H%M%S')}_{uuid4().hex}{_safe_suffix(upload_file.filename)}"
    target_path = target_dir / filename
    target_path.write_bytes(content)
    return target_path
