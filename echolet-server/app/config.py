from __future__ import annotations

import os
from pathlib import Path


BASE_DIR = Path(__file__).resolve().parent.parent
DEFAULT_STORAGE_DIR = BASE_DIR / "storage"


def get_api_token() -> str:
    return os.getenv("ECHOLET_API_TOKEN", "dev-token-change-me")


def get_storage_root() -> Path:
    return Path(os.getenv("ECHOLET_STORAGE_DIR", str(DEFAULT_STORAGE_DIR)))


def get_audio_root() -> Path:
    return get_storage_root() / "audio"


def get_events_root() -> Path:
    return get_storage_root() / "events"


def ensure_storage_dirs() -> None:
    get_audio_root().mkdir(parents=True, exist_ok=True)
    get_events_root().mkdir(parents=True, exist_ok=True)
