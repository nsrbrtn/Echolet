from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True)
class CommandDetectionResult:
    detected_command: str
    command_confidence: str
    clean_text: str


_PREFIXES: tuple[tuple[str, str], ...] = (
    ("заметка", "note"),
    ("запись", "note"),
    ("мысль", "note"),
    ("идея", "idea"),
    ("задача", "task"),
    ("напоминание", "reminder"),
    ("напомни", "reminder"),
)


def detect_command(transcript: str) -> CommandDetectionResult:
    stripped_transcript = transcript.strip()
    lowered = stripped_transcript.lower()

    for prefix, command in _PREFIXES:
        prefix_with_colon = f"{prefix}:"
        if lowered.startswith(prefix_with_colon):
            clean_text = stripped_transcript[len(prefix_with_colon) :].strip()
            return CommandDetectionResult(
                detected_command=command,
                command_confidence="explicit",
                clean_text=clean_text,
            )

    return CommandDetectionResult(
        detected_command="note",
        command_confidence="fallback",
        clean_text=stripped_transcript,
    )
