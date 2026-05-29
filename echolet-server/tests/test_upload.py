from __future__ import annotations

from pathlib import Path
from unittest.mock import Mock

from fastapi.testclient import TestClient

from app.main import app
from app.transcription import TranscriptionError


client = TestClient(app)


def _configure_env(tmp_path, monkeypatch) -> Path:
    vault_path = tmp_path / "vault with spaces"
    monkeypatch.setenv("ECHOLET_TOKEN", "test-token")
    monkeypatch.setenv("ECHOLET_STORAGE_DIR", str(tmp_path / "storage"))
    monkeypatch.setenv("OBSIDIAN_VAULT_PATH", str(vault_path))
    monkeypatch.setenv("ECHOLET_INBOX_DIR", "80-89 AI/85 Echolet")
    monkeypatch.setenv("ECHOLET_ATTACHMENTS_DIR", "80-89 AI/85 Echolet/_attachments")
    monkeypatch.setenv("WHISPER_LANGUAGE", "ru")
    return vault_path


def _upload_sample_audio(expected_transcription_status: str = "pending") -> dict:
    response = client.post(
        "/api/v1/upload-audio",
        headers={"Authorization": "Bearer test-token"},
        files={"file": ("sample.wav", b"fake wav bytes", "audio/wav")},
        data={
            "device_id": "echolet-001",
            "created_at": "2026-05-29T10:30:45Z",
            "battery": "87",
            "firmware_version": "0.1.0",
        },
    )

    assert response.status_code == 200
    payload = response.json()
    assert payload["ok"] is True
    assert payload["status"] == "received"
    assert payload["event_id"]
    assert payload["transcription_status"] == expected_transcription_status
    assert payload["inbox_dir"] == "80-89 AI/85 Echolet"
    return payload


def test_upload_audio_creates_markdown_and_attachment_in_obsidian_inbox_with_transcription_by_default(
    tmp_path, monkeypatch
) -> None:
    vault_path = _configure_env(tmp_path, monkeypatch)
    transcription_mock = Mock(return_value="Заметка: привет мир")
    monkeypatch.setattr("app.main.transcribe_audio_file", transcription_mock)

    payload = _upload_sample_audio(expected_transcription_status="pending")

    temp_audio_dir = tmp_path / "storage" / "audio" / "2026" / "05"
    temp_audio_files = list(temp_audio_dir.glob("*.wav"))
    assert len(temp_audio_files) == 1
    transcription_mock.assert_called_once_with(temp_audio_files[0])
    assert payload["transcription_status"] == "pending"

    inbox_dir = vault_path / "80-89 AI" / "85 Echolet"
    markdown_files = list(inbox_dir.glob("*.md"))
    assert len(markdown_files) == 1

    markdown_text = markdown_files[0].read_text(encoding="utf-8")
    assert markdown_files[0].name == "2026-05-29 10-30-45 echolet.md"
    assert markdown_text.startswith("---\nsource: echolet\n")
    assert 'schema_version: "1.0"' in markdown_text
    assert "event_type: voice_capture" in markdown_text
    assert "status: new" in markdown_text
    assert 'device_id: "echolet-001"' in markdown_text
    assert 'language: "ru"' in markdown_text
    assert 'battery: "87"' in markdown_text
    assert 'firmware_version: "0.1.0"' in markdown_text
    assert 'audio: "_attachments/2026/05/audio-2026-05-29-10-30-45.wav"' in markdown_text
    assert "# Echolet capture 2026-05-29 10:30" in markdown_text
    assert "## Transcript" in markdown_text
    assert "Заметка: привет мир" in markdown_text
    assert "## Original audio" in markdown_text
    assert "[[audio-2026-05-29-10-30-45.wav]]" in markdown_text
    assert "## Instructions for OpenClaw" in markdown_text
    assert "create or schedule a Telegram reminder" in markdown_text
    assert 'detected_command_hint: "note"' in markdown_text
    assert 'command_confidence: "explicit"' in markdown_text

    attachment_dir = inbox_dir / "_attachments" / "2026" / "05"
    attachment_files = list(attachment_dir.glob("*.wav"))
    assert len(attachment_files) == 1
    assert attachment_files[0].name == "audio-2026-05-29-10-30-45.wav"
    assert attachment_files[0].read_bytes() == b"fake wav bytes"

    all_markdown = list(vault_path.rglob("*.md"))
    assert all_markdown == markdown_files


def test_upload_audio_creates_markdown_with_transcript_when_server_transcribe_enabled(tmp_path, monkeypatch) -> None:
    vault_path = _configure_env(tmp_path, monkeypatch)
    monkeypatch.setenv("SERVER_TRANSCRIBE", "true")
    transcription_mock = Mock(return_value="Заметка: привет мир")
    monkeypatch.setattr("app.main.transcribe_audio_file", transcription_mock)

    payload = _upload_sample_audio(expected_transcription_status="pending")

    temp_audio_dir = tmp_path / "storage" / "audio" / "2026" / "05"
    temp_audio_files = list(temp_audio_dir.glob("*.wav"))
    assert len(temp_audio_files) == 1
    transcription_mock.assert_called_once_with(temp_audio_files[0])
    assert payload["transcription_status"] == "pending"

    inbox_dir = vault_path / "80-89 AI" / "85 Echolet"
    markdown_files = list(inbox_dir.glob("*.md"))
    assert len(markdown_files) == 1

    markdown_text = markdown_files[0].read_text(encoding="utf-8")
    assert 'detected_command_hint: "note"' in markdown_text
    assert 'command_confidence: "explicit"' in markdown_text
    assert "Заметка: привет мир" in markdown_text


def test_upload_audio_skips_transcription_when_server_transcribe_disabled(tmp_path, monkeypatch) -> None:
    vault_path = _configure_env(tmp_path, monkeypatch)
    monkeypatch.setenv("SERVER_TRANSCRIBE", "false")
    transcription_mock = Mock(return_value="Заметка: привет мир")
    monkeypatch.setattr("app.main.transcribe_audio_file", transcription_mock)

    payload = _upload_sample_audio(expected_transcription_status="skipped")

    transcription_mock.assert_not_called()
    assert payload["transcription_status"] == "skipped"

    inbox_dir = vault_path / "80-89 AI" / "85 Echolet"
    markdown_files = list(inbox_dir.glob("*.md"))
    assert len(markdown_files) == 1

    markdown_text = markdown_files[0].read_text(encoding="utf-8")
    assert "Transcript was not generated by the server." in markdown_text
    assert 'detected_command_hint: "note"' not in markdown_text


def test_upload_audio_creates_failed_markdown_when_whisper_fails(tmp_path, monkeypatch) -> None:
    vault_path = _configure_env(tmp_path, monkeypatch)
    monkeypatch.setenv("SERVER_TRANSCRIBE", "true")
    monkeypatch.setattr("app.main.transcribe_audio_file", Mock(side_effect=TranscriptionError("mock whisper failure")))

    payload = _upload_sample_audio(expected_transcription_status="pending")

    assert payload["transcription_status"] == "pending"
    inbox_dir = vault_path / "80-89 AI" / "85 Echolet"
    markdown_files = list(inbox_dir.glob("*.md"))
    assert len(markdown_files) == 1

    markdown_text = markdown_files[0].read_text(encoding="utf-8")
    assert "status: transcription_failed" in markdown_text
    assert "Transcript unavailable because server-side transcription failed." in markdown_text
    assert "## Transcription error" in markdown_text
    assert "mock whisper failure" in markdown_text

    attachment_dir = inbox_dir / "_attachments" / "2026" / "05"
    attachment_files = list(attachment_dir.glob("*.wav"))
    assert len(attachment_files) == 1
    assert attachment_files[0].read_bytes() == b"fake wav bytes"


def test_upload_audio_uses_unique_names_when_note_and_audio_already_exist(tmp_path, monkeypatch) -> None:
    vault_path = _configure_env(tmp_path, monkeypatch)

    inbox_dir = vault_path / "80-89 AI" / "85 Echolet"
    attachment_dir = inbox_dir / "_attachments" / "2026" / "05"
    inbox_dir.mkdir(parents=True, exist_ok=True)
    attachment_dir.mkdir(parents=True, exist_ok=True)
    (inbox_dir / "2026-05-29 10-30-45 echolet.md").write_text("existing", encoding="utf-8")
    (attachment_dir / "audio-2026-05-29-10-30-45.wav").write_bytes(b"old")

    _upload_sample_audio()

    new_markdown = inbox_dir / "2026-05-29 10-30-45 echolet 2.md"
    assert new_markdown.exists()

    new_audio = attachment_dir / "audio-2026-05-29-10-30-45-2.wav"
    assert new_audio.exists()
    assert new_audio.read_bytes() == b"fake wav bytes"


def test_upload_audio_rejects_missing_token(tmp_path, monkeypatch) -> None:
    _configure_env(tmp_path, monkeypatch)

    response = client.post(
        "/api/v1/upload-audio",
        files={"file": ("sample.wav", b"fake wav bytes", "audio/wav")},
        data={
            "device_id": "echolet-dev-1",
            "created_at": "2026-05-29T10:30:45Z",
            "battery": "87",
            "firmware_version": "0.1.0",
        },
    )

    assert response.status_code == 401
    assert response.json()["detail"] == "Missing Authorization header"


def test_upload_audio_rejects_invalid_token(tmp_path, monkeypatch) -> None:
    _configure_env(tmp_path, monkeypatch)

    response = client.post(
        "/api/v1/upload-audio",
        headers={"Authorization": "Bearer nope"},
        files={"file": ("sample.wav", b"fake wav bytes", "audio/wav")},
        data={
            "device_id": "echolet-dev-1",
            "created_at": "2026-05-29T10:30:45Z",
            "battery": "87",
            "firmware_version": "0.1.0",
        },
    )

    assert response.status_code == 401
    assert response.json()["detail"] == "Invalid bearer token"


def test_upload_audio_rejects_missing_file(tmp_path, monkeypatch) -> None:
    _configure_env(tmp_path, monkeypatch)

    response = client.post(
        "/api/v1/upload-audio",
        headers={"Authorization": "Bearer test-token"},
        data={
            "device_id": "echolet-dev-1",
            "created_at": "2026-05-29T10:30:45Z",
            "battery": "87",
        },
    )

    assert response.status_code == 422
    assert response.json()["detail"][0]["loc"] == ["body", "file"]
