from __future__ import annotations

import logging
from pathlib import Path

from app.config import (
    get_whisper_compute_type,
    get_whisper_device,
    get_whisper_language,
    get_whisper_model,
)


logger = logging.getLogger(__name__)


class TranscriptionError(RuntimeError):
    pass


def _load_model():
    try:
        from faster_whisper import WhisperModel
    except ImportError as exc:  # pragma: no cover - dependency failure
        raise TranscriptionError("faster-whisper is not installed") from exc

    return WhisperModel(
        get_whisper_model(),
        device=get_whisper_device(),
        compute_type=get_whisper_compute_type(),
    )


def transcribe_audio_file(audio_path: Path) -> str:
    logger.info(
        "Starting faster-whisper transcription for %s with model=%s device=%s compute_type=%s",
        audio_path,
        get_whisper_model(),
        get_whisper_device(),
        get_whisper_compute_type(),
    )

    try:
        model = _load_model()
        segments, info = model.transcribe(
            str(audio_path),
            language=get_whisper_language(),
            vad_filter=True,
        )
        transcript = " ".join(segment.text.strip() for segment in segments if segment.text.strip()).strip()
    except Exception as exc:
        raise TranscriptionError(str(exc) or "unknown faster-whisper error") from exc

    if not transcript:
        raise TranscriptionError("faster-whisper returned an empty transcript")

    logger.info(
        "faster-whisper transcription finished for %s (%s chars, language=%s, language_probability=%.3f)",
        audio_path,
        len(transcript),
        info.language,
        info.language_probability,
    )
    return transcript
