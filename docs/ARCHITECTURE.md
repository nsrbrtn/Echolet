# ARCHITECTURE

## System Flow

```text
ESP32-S3 Sense
  ->
microSD
  ->
HTTP upload over Wi-Fi
  ->
FastAPI server on laptop
  ->
faster-whisper
  ->
Obsidian inbox markdown + audio attachment
  ->
OpenClaw
  ->
Obsidian notes / tasks / Telegram reminders
```

## ESP32 Role

The ESP32 device is responsible for:

- recording audio
- storing files on microSD
- keeping the upload queue
- sending files over Wi-Fi
- showing state via RGB LED

It is not responsible for speech recognition or command interpretation.

## FastAPI Server Role

The laptop-side FastAPI server is responsible for:

- accepting WAV uploads
- verifying the bearer token
- saving the uploaded file
- running `faster-whisper` transcription by default
- copying audio into the Obsidian vault
- creating an inbox markdown file for OpenClaw in `80-89 AI/85 Echolet`

The server does not create final notes, tasks, or Telegram reminders.

## Whisper Role

`faster-whisper` runs locally on the laptop and converts audio into text.
This step can be disabled with `SERVER_TRANSCRIBE=false`, but the default is enabled.

## OpenClaw Role

OpenClaw is responsible for:

- reading inbox markdown files from `80-89 AI/85 Echolet`
- deciding what the capture means
- creating a final note
- creating a task
- creating a reminder via Telegram
- updating inbox item status when processing is complete

## Obsidian Role

Obsidian is the main storage layer for notes and tasks.

## Telegram Role

Telegram is the delivery channel for reminders and possible future confirmations.

## Boundary Rules

- Device-side logic stays simple.
- Server-side logic handles ingest and default transcription.
- OpenClaw handles interpretation and downstream actions.
- Audio must survive network failures.
- MVP stays local-first and home-network-first.
