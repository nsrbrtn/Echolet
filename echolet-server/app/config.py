from __future__ import annotations

import os
from pathlib import Path


BASE_DIR = Path(__file__).resolve().parent.parent
DEFAULT_STORAGE_DIR = BASE_DIR / "storage"
DEFAULT_WHISPER_MODEL = "medium"
DEFAULT_WHISPER_DEVICE = "cpu"
DEFAULT_WHISPER_COMPUTE_TYPE = "int8"
DEFAULT_SERVER_TRANSCRIBE = True
DEFAULT_OBSIDIAN_VAULT_PATH = Path("./dev_obsidian_vault")
DEFAULT_ECHOLET_INBOX_DIR = Path("80-89 AI") / "85 Echolet"
DEFAULT_ECHOLET_ATTACHMENTS_DIR = DEFAULT_ECHOLET_INBOX_DIR / "_attachments"


def get_api_token() -> str:
    return os.getenv("ECHOLET_TOKEN") or os.getenv("ECHOLET_API_TOKEN", "dev-token-change-me")


def get_storage_root() -> Path:
    return Path(os.getenv("ECHOLET_STORAGE_DIR", str(DEFAULT_STORAGE_DIR)))


def get_audio_root() -> Path:
    return get_storage_root() / "audio"


def get_obsidian_vault_root() -> Path:
    return Path(os.getenv("OBSIDIAN_VAULT_PATH", str(DEFAULT_OBSIDIAN_VAULT_PATH)))


def get_echolet_inbox_dir() -> Path:
    return Path(os.getenv("ECHOLET_INBOX_DIR", str(DEFAULT_ECHOLET_INBOX_DIR)))


def get_echolet_attachments_dir() -> Path:
    return Path(os.getenv("ECHOLET_ATTACHMENTS_DIR", str(DEFAULT_ECHOLET_ATTACHMENTS_DIR)))


def get_echolet_inbox_root() -> Path:
    return get_obsidian_vault_root() / get_echolet_inbox_dir()


def get_echolet_attachments_root() -> Path:
    return get_obsidian_vault_root() / get_echolet_attachments_dir()


def get_whisper_model() -> str:
    return os.getenv("WHISPER_MODEL", DEFAULT_WHISPER_MODEL)


def get_whisper_language() -> str:
    return os.getenv("WHISPER_LANGUAGE", "ru")


def get_whisper_device() -> str:
    return os.getenv("WHISPER_DEVICE", DEFAULT_WHISPER_DEVICE)


def get_whisper_compute_type() -> str:
    return os.getenv("WHISPER_COMPUTE_TYPE", DEFAULT_WHISPER_COMPUTE_TYPE)


def get_server_transcribe() -> bool:
    raw_value = os.getenv("SERVER_TRANSCRIBE")
    if raw_value is None:
        return DEFAULT_SERVER_TRANSCRIBE

    normalized = raw_value.strip().lower()
    return normalized in {"1", "true", "yes", "on"}


def ensure_storage_dirs() -> None:
    get_audio_root().mkdir(parents=True, exist_ok=True)
    get_echolet_inbox_root().mkdir(parents=True, exist_ok=True)
    get_echolet_attachments_root().mkdir(parents=True, exist_ok=True)
