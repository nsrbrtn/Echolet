# Echolet

Echolet is a pocket voice-capture device for OpenClaw built around the ESP32-S3 Sense.

The goal is simple: record thoughts, tasks, and observations without a phone, screen, or app. You hold a button, speak, release it, and the device stores the audio on microSD and sends it to a laptop running OpenClaw when Wi-Fi is available.

## What It Should Do

- Record voice while a physical button is held.
- Save the original audio locally to microSD.
- Send recordings to OpenClaw on a laptop over Wi-Fi.
- Keep unsent files in a queue when the network is unavailable.
- Show status with an RGB LED instead of a screen.

## OpenClaw Flow

After the audio reaches the laptop, OpenClaw should be able to:

- transcribe speech with Whisper;
- create notes in Obsidian;
- create tasks in Obsidian;
- create Telegram reminders;
- later attach photos or video as extra context.

## MVP Direction

The first version focuses on voice notes only:

1. press and hold to record;
2. save WAV to microSD;
3. upload when Wi-Fi is available;
4. process the recording on the laptop;
5. never lose a note if the laptop is offline.

## Hardware Draft

- ESP32-S3 Sense
- microSD card
- record button
- RGB LED
- 1000 mAh battery
- compact 3D-printed enclosure

## Why This Exists

Phones are slow, distracting, and screen-heavy for quick capture. Echolet is meant to be a dedicated, low-friction input device for ideas that should go straight into an OpenClaw workflow.
