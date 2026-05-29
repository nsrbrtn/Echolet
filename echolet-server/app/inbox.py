from __future__ import annotations

import shutil
from datetime import datetime
from pathlib import Path
from uuid import uuid4

from app.command_detection import detect_command
from app.config import (
    ensure_storage_dirs,
    get_echolet_attachments_dir,
    get_echolet_attachments_root,
    get_echolet_inbox_root,
    get_whisper_language,
)


def _safe_suffix(filename: str | None) -> str:
    if not filename:
        return ".wav"

    suffix = Path(filename).suffix
    return suffix if suffix else ".wav"


def _format_timestamp(value: datetime) -> str:
    return value.astimezone().replace(tzinfo=None, microsecond=0).isoformat()


def _quote_yaml(value: str) -> str:
    escaped = value.replace("\\", "\\\\").replace('"', '\\"')
    return f'"{escaped}"'


def _build_unique_path(directory: Path, filename: str, duplicate_separator: str) -> Path:
    candidate = directory / filename
    if not candidate.exists():
        return candidate

    stem = candidate.stem
    suffix = candidate.suffix
    index = 2
    while True:
        duplicate_name = f"{stem}{duplicate_separator}{index}{suffix}"
        candidate = directory / duplicate_name
        if not candidate.exists():
            return candidate
        index += 1


def copy_audio_to_obsidian(
    *,
    stored_path: Path,
    original_filename: str | None,
    created_at: datetime,
) -> tuple[Path, Path]:
    ensure_storage_dirs()

    target_dir = get_echolet_attachments_root() / created_at.strftime("%Y") / created_at.strftime("%m")
    target_dir.mkdir(parents=True, exist_ok=True)

    audio_name = f"audio-{created_at.strftime('%Y-%m-%d-%H-%M-%S')}{_safe_suffix(original_filename)}"
    audio_path = _build_unique_path(target_dir, audio_name, "-")
    shutil.copy2(stored_path, audio_path)

    relative_audio_path = audio_path.relative_to(get_echolet_inbox_root())
    return audio_path, relative_audio_path


def create_obsidian_inbox_markdown(
    *,
    created_at: datetime,
    received_at: datetime,
    device_id: str,
    battery: str,
    firmware_version: str | None,
    transcript: str | None,
    transcription_error: str | None,
    audio_relative_path: Path,
) -> Path:
    ensure_storage_dirs()

    inbox_root = get_echolet_inbox_root()
    note_name = f"{created_at.strftime('%Y-%m-%d %H-%M-%S')} echolet.md"
    note_path = _build_unique_path(inbox_root, note_name, " ")

    status_value = "transcription_failed" if transcription_error else "new"
    frontmatter_lines = [
        "---",
        "source: echolet",
        'schema_version: "1.0"',
        "event_type: voice_capture",
        f"status: {status_value}",
        f"created_at: {_quote_yaml(_format_timestamp(created_at))}",
        f"received_at: {_quote_yaml(_format_timestamp(received_at))}",
        f"device_id: {_quote_yaml(device_id)}",
        f"language: {_quote_yaml(get_whisper_language())}",
        f"audio: {_quote_yaml(audio_relative_path.as_posix())}",
        f"battery: {_quote_yaml(str(battery))}",
        f"firmware_version: {_quote_yaml(firmware_version or 'unknown')}",
    ]

    if transcript:
        detection = detect_command(transcript)
        if detection.command_confidence == "explicit":
            frontmatter_lines.append(f"detected_command_hint: {_quote_yaml(detection.detected_command)}")
            frontmatter_lines.append(f"command_confidence: {_quote_yaml(detection.command_confidence)}")

    frontmatter_lines.append("---")

    title = created_at.strftime("%Y-%m-%d %H:%M")
    transcript_block = transcript if transcript else "Transcript unavailable."
    audio_filename = Path(audio_relative_path).name

    body_lines = [
        f"# Echolet capture {title}",
        "",
        "## Transcript",
        "",
        transcript_block,
        "",
        "## Original audio",
        "",
        f"[[{audio_filename}]]",
        "",
        "## Instructions for OpenClaw",
        "",
        "OpenClaw should process this capture and decide what to do with it.",
        "",
        "Rules:",
        "",
        "- If the transcript starts with `заметка`, create a regular Obsidian note.",
        "- If the transcript starts with `идея`, create an idea note.",
        "- If the transcript starts with `задача`, create a task in Obsidian.",
        "- If the transcript starts with `напоминание` or `напомни`, create or schedule a Telegram reminder.",
        "- If unsure, save as a regular note.",
        "- Reference or attach the original audio file when useful.",
        "- After successful processing, OpenClaw may update `status` from `new` to `processed`.",
    ]

    if transcription_error:
        body_lines.extend(
            [
                "",
                "## Transcription error",
                "",
                transcription_error,
            ]
        )

    note_content = "\n".join(frontmatter_lines + [""] + body_lines) + "\n"
    note_path.write_text(note_content, encoding="utf-8")
    return note_path


def create_obsidian_capture(
    *,
    stored_path: Path,
    original_filename: str | None,
    created_at: datetime,
    received_at: datetime,
    device_id: str,
    battery: str,
    firmware_version: str | None,
    transcript: str | None,
    transcription_error: str | None,
) -> tuple[str, Path, Path]:
    capture_id = uuid4().hex
    _, relative_audio_path = copy_audio_to_obsidian(
        stored_path=stored_path,
        original_filename=original_filename,
        created_at=created_at,
    )
    note_path = create_obsidian_inbox_markdown(
        created_at=created_at,
        received_at=received_at,
        device_id=device_id,
        battery=battery,
        firmware_version=firmware_version,
        transcript=transcript,
        transcription_error=transcription_error,
        audio_relative_path=relative_audio_path,
    )
    return capture_id, note_path, relative_audio_path
