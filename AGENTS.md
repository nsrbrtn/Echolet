# AGENTS.md

## Project Purpose

echolet is a pocket voice-input device for OpenClaw. Its job is narrow by design: capture audio reliably, store it safely, and upload it to a laptop for processing.

Core principle:

> The device should be a simple and reliable input collector. All intelligence lives on the laptop.

## Primary Flow

1. The user holds a button.
2. The device records audio.
3. The user releases the button.
4. The device saves a WAV file to microSD.
5. If Wi-Fi is available, the device uploads the file to the laptop.
6. If Wi-Fi is unavailable, the file stays in the queue.
7. A FastAPI server on the laptop accepts the file.
8. Whisper transcribes the speech locally.
9. OpenClaw creates an Obsidian note, task, or Telegram reminder.

## Hard Constraints

- Do not implement speech recognition on ESP32.
- Do not implement command parsing on ESP32.
- Do not add photo support in MVP 0.1.
- Do not add video support in MVP 0.1.
- Do not add a display.
- Do not add Bluetooth.
- Do not add cloud or VPS sync in MVP 0.1.
- Do not complicate the architecture without a clear need.
- Do not rewrite the project to ESP-IDF without an explicit reason.
- Do not store Wi-Fi credentials or tokens in git if the repository becomes public.

## Firmware Stack

- PlatformIO
- Arduino framework
- C++
- `board = seeed_xiao_esp32s3`

## Why This Stack

- Faster project start.
- Ready-made examples from Seeed exist.
- Easier to delegate concrete tasks to AI agents.
- Fully sufficient for MVP 0.1.
- PlatformIO does not block future migration to ESP-IDF if needed.

## Code Architecture Requirement

The codebase must stay modular.

Preferred structure:

```text
echolet-firmware/
  platformio.ini
  src/
    main.cpp
    config.h
    button.cpp
    button.h
    led.cpp
    led.h
    audio_recorder.cpp
    audio_recorder.h
    storage.cpp
    storage.h
    wifi_client.cpp
    wifi_client.h
    uploader.cpp
    uploader.h
    queue.cpp
    queue.h
```

`main.cpp` must not contain the entire implementation. It should only connect modules together and coordinate state transitions.

## Development Rules

- Work in small steps.
- After each stage, the project must still build.
- Do not add several large features at once.
- Implement button and LED first.
- Then implement microSD.
- Then WAV recording.
- Then Wi-Fi.
- Then upload.
- Then queue management.
- Then storage cleanup.
- Every change must be testable.
- If a change risks breaking a working feature, explain that risk first.

## Rules For AI Agents

- Inspect the existing project structure before making changes.
- Do not delete existing files unless necessary.
- Do not change APIs or folder structure without a reason.
- Do not invent new features outside MVP.
- If information is missing, make the smallest safe assumption and mark it clearly in comments or docs.
- Do not add heavy libraries without a real need.
- Do not write speculative “future-proof” code that is not needed for the current stage.
- Do not mix Arduino and ESP-IDF without an explicit project decision.

## Current MVP 0.1 Scope

1. Recording while the button is held.
2. Stop recording when the button is released.
3. Red RGB LED while recording.
4. WAV mono 16 kHz 16 bit PCM.
5. Save audio to microSD.
6. Queue unsent files.
7. Wi-Fi only on the home network.
8. HTTP POST upload to a FastAPI server.
9. Bearer token in the request header.
10. Move successfully uploaded files to `/sent/`.
11. Delete old files from `/sent/` if microSD usage exceeds 80%.
12. Audio only, with no photo or video.

## Hardware Assumptions

- Board: ESP32-S3 Sense.
- Firmware upload via USB-C.
- There may be two buttons.
- RGB LED is a standard 4-pin component.
- No display is used.
- No buzzer or speaker is used.
- microSD: 32 GB.
- Battery: 1000 mAh.
- Wi-Fi SSID and password live in `config.h` for MVP.
- Upload happens only on the home network.
- Camera and photo features are not used in v0.1.

## Upload API

Expected endpoint:

```http
POST /api/v1/upload-audio
Authorization: Bearer <TOKEN>
Content-Type: multipart/form-data
```

Fields:

- `file`
- `device_id`
- `created_at`
- `battery`

Successful response:

```json
{
  "ok": true,
  "status": "received",
  "event_id": "..."
}
```

## microSD Layout

Legacy compatibility with `/golosok/` is acceptable if older docs require it, but new documentation should use the project name consistently.

Preferred layout:

```text
/echolet/
  audio/
    new/
    sent/
    failed/
  meta/
  queue.json
  logs/
    device.log
```

If this path is ever changed back to `/golosok/`, document it explicitly and update all documentation consistently.

## File Statuses

Use these statuses:

- `new`
- `sending`
- `sent`
- `failed`

Do not delete `new` or `failed` files automatically.

## Storage Cleanup

If microSD usage exceeds 80%, delete the oldest files from `/echolet/audio/sent/`.

Never delete automatically from:

- `/echolet/audio/new/`
- `/echolet/audio/failed/`

## RGB LED States

| State | Indication |
|---|---|
| Ready | off or dim green |
| Recording | red |
| Uploading | blue |
| Upload success | short green flash |
| Wi-Fi error | blinking yellow |
| Upload error | blinking red |
| Low battery | slow red blink |

## Do Not Implement Yet

- camera
- photo notes
- video
- Bluetooth sync
- display UI
- speech-to-text on device
- command parsing on device
- VPS or cloud sync
- encryption
- OTA updates
- deep sleep optimization
- advanced power management
