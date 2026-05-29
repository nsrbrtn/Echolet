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
local Whisper
  ->
event file for OpenClaw
  ->
OpenClaw
  ->
Obsidian / Telegram
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
- launching Whisper
- creating an event for OpenClaw

## Whisper Role

Whisper runs locally on the laptop and converts audio into text.

## OpenClaw Role

OpenClaw is responsible for:

- classifying the command or intent
- creating a note
- creating a task
- creating a reminder via Telegram

## Obsidian Role

Obsidian is the main storage layer for notes and tasks.

## Telegram Role

Telegram is the delivery channel for reminders and possible future confirmations.

## Boundary Rules

- Device-side logic stays simple.
- Server-side logic handles interpretation.
- Audio must survive network failures.
- MVP stays local-first and home-network-first.
