# FIRMWARE

## Stack

- PlatformIO
- Arduino framework
- C++
- `board = seeed_xiao_esp32s3`

## Board

Target hardware for MVP is ESP32-S3 Sense, flashed over USB-C.

## Pins And Configuration

Current confirmed MVP wiring:

- record button: `D1 -> GND`
- power / wake button: `D2 -> GND`
- WS2812B `DI -> D3`
- WS2812B `5V -> 3V3`
- WS2812B `GND -> GND`

For MVP, Wi-Fi SSID and password may live in `config.h`.

Configuration assumptions for MVP:

- home Wi-Fi only
- bearer token stored locally on device
- no cloud configuration service

## Buttons

Recording flow:

```text
button pressed -> start recording
button released -> stop recording
```

One or two buttons are acceptable in hardware, but MVP behavior should stay simple and deterministic.

Current firmware behavior:

- `D1` is the record button
- `D2` is the power / wake button
- holding `D2` for 3 seconds in idle or queued state arms power-off
- releasing `D2` after that enters deep sleep
- pressing `D2` wakes the device from deep sleep
- power-off is ignored during recording, save, and active upload attempt

## RGB LED

Required states:

- startup: short red -> green -> blue animation
- ready: dim green
- recording: red
- uploading: blue
- success: short green flash
- queued offline: slow dim yellow pulse
- Wi-Fi error: reserved for explicit Wi-Fi fault indication
- upload error: blinking red
- low battery: rare red blink
- power off pending: LED off before deep sleep

## Audio

Required audio format:

- WAV
- mono
- 16 kHz
- 16 bit PCM

The ESP32 only records and stores audio. It does not transcribe or classify it.

## microSD

microSD is required for local durability.

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

## Wi-Fi

For MVP:

- SSID and password live in `config.h`
- only the home network is targeted
- the device may skip upload if the expected network is unavailable

## HTTP Upload

Upload requirements:

- `HTTP POST`
- `multipart/form-data`
- `Authorization: Bearer <TOKEN>`

Expected fields:

- `file`
- `device_id`
- `created_at`
- `battery`

## Queue

If upload fails or Wi-Fi is unavailable:

- the audio file must remain on microSD
- the file must stay discoverable for retry
- unsent files must not be lost

Status model:

- `new`
- `sending`
- `sent`
- `failed`

## Storage Cleanup

If microSD usage exceeds 80%, delete the oldest files from `/echolet/audio/sent/`.

Never auto-delete from:

- `/echolet/audio/new/`
- `/echolet/audio/failed/`

## What To Verify At Each Stage

- Button stage: press and release are detected reliably.
- LED stage: the expected state is visible and stable.
- microSD stage: a test file can be created and read back.
- WAV stage: produced files are valid WAV mono 16 kHz 16 bit PCM.
- Wi-Fi stage: the board connects reliably to the target network.
- Upload stage: the server accepts multipart upload with bearer token.
- Queue stage: failed uploads remain retryable after reboot.
- Cleanup stage: only old `sent` files are removed when storage crosses 80%.
