from __future__ import annotations

import logging
import os
import shutil
import subprocess
from pathlib import Path
from tempfile import TemporaryDirectory

from app.config import BASE_DIR, get_whisper_command, get_whisper_language, get_whisper_model


logger = logging.getLogger(__name__)


class TranscriptionError(RuntimeError):
    pass


def _build_subprocess_env() -> dict[str, str]:
    env = os.environ.copy()
    if shutil.which("ffmpeg", path=env.get("PATH")):
        return env

    try:
        from imageio_ffmpeg import get_ffmpeg_exe
    except Exception:
        return env

    ffmpeg_path = Path(get_ffmpeg_exe())
    shim_dir = BASE_DIR / ".venv" / "bin"
    shim_path = shim_dir / "ffmpeg"
    if not shim_path.exists():
        shim_dir.mkdir(parents=True, exist_ok=True)
        shim_path.symlink_to(ffmpeg_path)

    env["PATH"] = f"{shim_dir}{os.pathsep}{env.get('PATH', '')}"
    return env


def transcribe_audio_file(audio_path: Path) -> str:
    command_name = get_whisper_command()
    logger.info("Starting Whisper transcription for %s with %s", audio_path, command_name)

    with TemporaryDirectory(prefix="echolet-whisper-") as output_dir_name:
        output_dir = Path(output_dir_name)
        command = [
            command_name,
            str(audio_path),
            "--model",
            get_whisper_model(),
            "--language",
            get_whisper_language(),
            "--output_format",
            "txt",
            "--output_dir",
            str(output_dir),
            "--fp16",
            "False",
        ]

        try:
            completed = subprocess.run(
                command,
                check=True,
                capture_output=True,
                text=True,
                env=_build_subprocess_env(),
            )
        except FileNotFoundError as exc:
            raise TranscriptionError(f"Whisper command not found: {command_name}") from exc
        except subprocess.CalledProcessError as exc:
            stderr = exc.stderr.strip() if exc.stderr else ""
            stdout = exc.stdout.strip() if exc.stdout else ""
            details = stderr or stdout or "unknown Whisper error"
            raise TranscriptionError(details) from exc

        transcript_path = output_dir / f"{audio_path.stem}.txt"
        if not transcript_path.exists():
            raise TranscriptionError("Whisper did not produce a transcript file")

        transcript = transcript_path.read_text(encoding="utf-8").strip()
        if not transcript:
            raise TranscriptionError("Whisper returned an empty transcript")
        logger.info(
            "Whisper transcription finished for %s (%s chars)",
            audio_path,
            len(transcript),
        )
        if completed.stderr:
            logger.debug("Whisper stderr for %s: %s", audio_path, completed.stderr.strip())
        return transcript
