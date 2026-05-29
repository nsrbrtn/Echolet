from __future__ import annotations

import json

from fastapi.testclient import TestClient

from app.main import app


client = TestClient(app)


def test_upload_audio_success(tmp_path, monkeypatch) -> None:
    monkeypatch.setenv("ECHOLET_API_TOKEN", "test-token")
    monkeypatch.setenv("ECHOLET_STORAGE_DIR", str(tmp_path / "storage"))

    response = client.post(
        "/api/v1/upload-audio",
        headers={"Authorization": "Bearer test-token"},
        files={"file": ("sample.wav", b"fake wav bytes", "audio/wav")},
        data={
            "device_id": "echolet-dev-1",
            "created_at": "2026-05-29T10:30:45Z",
            "battery": "87",
        },
    )

    assert response.status_code == 200
    payload = response.json()
    assert payload["ok"] is True
    assert payload["status"] == "received"
    assert payload["event_id"]

    audio_dir = tmp_path / "storage" / "audio" / "2026" / "05"
    audio_files = list(audio_dir.glob("*.wav"))
    assert len(audio_files) == 1
    assert audio_files[0].read_bytes() == b"fake wav bytes"

    event_path = tmp_path / "storage" / "events" / f"{payload['event_id']}.json"
    assert event_path.exists()
    event_payload = json.loads(event_path.read_text(encoding="utf-8"))
    assert event_payload["device_id"] == "echolet-dev-1"
    assert event_payload["battery"] == "87"
    assert event_payload["transcript"] is None


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
        },
    )

    assert response.status_code == 401
    assert response.json()["detail"] == "Invalid bearer token"
