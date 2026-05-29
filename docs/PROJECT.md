# PROJECT

## What echolet Is

echolet is a pocket voice-capture device for OpenClaw. It is meant to capture thoughts, tasks, and observations with a physical button and no screen-first workflow.

## Why This Project Exists

The project exists to reduce friction between having a thought and saving it into a real workflow. The device should feel closer to a диктофон than to a mini smartphone.

## Why Not a Phone

- A phone takes too long to pull out.
- You have to open an app.
- The screen is distracting.
- Sometimes you want less screen time, not more.
- Speaking in a continuous flow is easier than tapping through UI.
- A physical record button is the whole point.

## Main Scenarios

- Hold button, speak a note, release button, and get a note in Obsidian.
- Hold button, speak a task, and let OpenClaw turn it into a task.
- Hold button, speak a reminder, and let OpenClaw route it to Telegram reminders.

Main scenario:

> Hold button -> say the thought -> release button -> the note appears in Obsidian.

## MVP 0.1

MVP 0.1 is intentionally narrow:

- audio only
- record on button hold
- save WAV to microSD
- upload over home Wi-Fi
- receive audio on a laptop via FastAPI
- transcribe locally with `faster-whisper`
- create an inbox markdown item plus attached audio in Obsidian
- let OpenClaw turn the capture into notes, tasks, or reminders

## v0.2

Possible additions for v0.2:

- stronger OpenClaw automation for inbox processing
- photo capture
- attaching photos to notes
- Telegram confirmations for tasks and reminders
- smarter routing and sorting
- operation outside the home network
- improved storage cleanup
- encryption

## Deferred For Later

- video
- on-device speech recognition
- command parsing on the ESP32
- display-based UI
- Bluetooth sync
- cloud or VPS sync

## Open Questions

- Which exact microphone/input stack is best on the selected ESP32-S3 Sense board?
- Should the final device use one button or two?
- What exact retry policy should be used for failed uploads?
- How should battery level be measured and reported?
- Should reminders be created only from explicit phrases or also from inferred intent?
