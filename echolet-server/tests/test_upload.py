from __future__ import annotations

import json
from unittest.mock import Mock

from fastapi.testclient import TestClient

from app.main import app
from app.transcription import TranscriptionError


client = TestClient(app)


def test_upload_audio_success_with_transcript(tmp_path, monkeypatch) -> None:
    monkeypatch.setenv("ECHOLET_API_TOKEN", "test-token")
    monkeypatch.setenv("ECHOLET_STORAGE_DIR", str(tmp_path / "storage"))
    transcription_mock = Mock(return_value="Заметка: привет мир")
    monkeypatch.setattr("app.main.transcribe_audio_file", transcription_mock)

    response = client.post(
        "/api/v1/upload-audio",
        headers={"Authorization": "Bearer test-token"},
        files={"file": ("sample.wav", b"fake wav bytes", "audio/wav")},
        data={
            "device_id": "echolet-dev-1",
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
    assert payload["transcription_status"] == "pending"

    audio_dir = tmp_path / "storage" / "audio" / "2026" / "05"
    audio_files = list(audio_dir.glob("*.wav"))
    assert len(audio_files) == 1
    assert audio_files[0].read_bytes() == b"fake wav bytes"
    transcription_mock.assert_called_once_with(audio_files[0])

    event_path = tmp_path / "storage" / "events" / f"{payload['event_id']}.json"
    assert event_path.exists()
    event_payload = json.loads(event_path.read_text(encoding="utf-8"))
    assert event_payload["source"] == "echolet"
    assert event_payload["device_id"] == "echolet-dev-1"
    assert event_payload["type"] == "voice"
    assert event_payload["created"] == "2026-05-29T10:30:45Z"
    assert event_payload["battery"] == "87"
    assert event_payload["firmware_version"] == "0.1.0"
    assert event_payload["audio_path"] == str(audio_files[0])
    assert event_payload["whisper_model"] == "medium"
    assert event_payload["transcription_status"] == "completed"
    assert event_payload["transcript"] == "Заметка: привет мир"
    assert event_payload["detected_command"] == "note"
    assert event_payload["command_confidence"] == "explicit"
    assert event_payload["clean_text"] == "привет мир"
    assert event_payload["status"] == "new"
    assert event_payload["transcription_error"] is None
    assert event_payload["transcription_finished_at"]


def test_upload_audio_keeps_file_when_whisper_fails(tmp_path, monkeypatch) -> None:
    monkeypatch.setenv("ECHOLET_API_TOKEN", "test-token")
    monkeypatch.setenv("ECHOLET_STORAGE_DIR", str(tmp_path / "storage"))
    monkeypatch.setattr(
        "app.main.transcribe_audio_file",
        Mock(side_effect=TranscriptionError("mock whisper failure")),
    )

    response = client.post(
        "/api/v1/upload-audio",
        headers={"Authorization": "Bearer test-token"},
        files={"file": ("sample.wav", b"fake wav bytes", "audio/wav")},
        data={
            "device_id": "echolet-dev-1",
            "created_at": "2026-05-29T10:30:45Z",
            "battery": "87",
            "firmware_version": "0.1.0",
        },
    )

    assert response.status_code == 200
    payload = response.json()
    assert payload["transcription_status"] == "pending"
    audio_dir = tmp_path / "storage" / "audio" / "2026" / "05"
    audio_files = list(audio_dir.glob("*.wav"))
    assert len(audio_files) == 1

    event_path = tmp_path / "storage" / "events" / f"{payload['event_id']}.json"
    assert event_path.exists()
    event_payload = json.loads(event_path.read_text(encoding="utf-8"))
    assert event_payload["audio_path"] == str(audio_files[0])
    assert event_payload["whisper_model"] == "medium"
    assert event_payload["transcription_status"] == "failed"
    assert event_payload["transcript"] is None
    assert event_payload["detected_command"] is None
    assert event_payload["command_confidence"] is None
    assert event_payload["clean_text"] is None
    assert event_payload["status"] == "new"
    assert event_payload["transcription_error"] == "mock whisper failure"
    assert event_payload["transcription_finished_at"]


def test_upload_audio_rejects_missing_token(tmp_path, monkeypatch) -> None:
    monkeypatch.setenv("ECHOLET_API_TOKEN", "test-token")
    monkeypatch.setenv("ECHOLET_STORAGE_DIR", str(tmp_path / "storage"))

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
    monkeypatch.setenv("ECHOLET_API_TOKEN", "test-token")
    monkeypatch.setenv("ECHOLET_STORAGE_DIR", str(tmp_path / "storage"))

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
    monkeypatch.setenv("ECHOLET_API_TOKEN", "test-token")
    monkeypatch.setenv("ECHOLET_STORAGE_DIR", str(tmp_path / "storage"))

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
