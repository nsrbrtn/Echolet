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
- record button on `D1`
- power / wake button on `D2`
- WS2812B RGB LED on `D3`
- 1000 mAh battery
- compact 3D-printed enclosure

Current confirmed MVP wiring:

- `D1 -> GND`: record button
- `D2 -> GND`: power / wake button
- `D3`: WS2812B data input
- `3V3`: WS2812B power
- `GND`: WS2812B ground

Current firmware behavior:

- hold `D1` to record
- release `D1` to stop recording
- hold `D2` for 3 seconds in idle or queued state, then release to enter deep sleep
- press `D2` to wake the device from deep sleep

Current LED indications:

- startup: short red -> green -> blue animation
- ready: dim green
- recording: red
- uploading: blue
- queued offline: slow dim yellow pulse
- upload error: blinking red

## Why This Exists

Phones are slow, distracting, and screen-heavy for quick capture. Echolet is meant to be a dedicated, low-friction input device for ideas that should go straight into an OpenClaw workflow.
