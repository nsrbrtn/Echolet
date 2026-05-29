from __future__ import annotations

import os
from pathlib import Path


BASE_DIR = Path(__file__).resolve().parent.parent
DEFAULT_STORAGE_DIR = BASE_DIR / "storage"
DEFAULT_WHISPER_MODEL = "medium"


def get_api_token() -> str:
    return os.getenv("ECHOLET_API_TOKEN", "dev-token-change-me")


def get_storage_root() -> Path:
    return Path(os.getenv("ECHOLET_STORAGE_DIR", str(DEFAULT_STORAGE_DIR)))


def get_audio_root() -> Path:
    return get_storage_root() / "audio"


def get_events_root() -> Path:
    return get_storage_root() / "events"


def get_whisper_command() -> str:
    configured = os.getenv("WHISPER_COMMAND")
    if configured:
        return configured

    venv_whisper = BASE_DIR / ".venv" / "bin" / "whisper"
    if venv_whisper.exists():
        return str(venv_whisper)

    return "whisper"


def get_whisper_model() -> str:
    return os.getenv("WHISPER_MODEL", DEFAULT_WHISPER_MODEL)


def get_whisper_language() -> str:
    return os.getenv("WHISPER_LANGUAGE", "ru")


def ensure_storage_dirs() -> None:
    get_audio_root().mkdir(parents=True, exist_ok=True)
    get_events_root().mkdir(parents=True, exist_ok=True)
