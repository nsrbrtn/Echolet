# FIRMWARE

## Stack

- PlatformIO
- Arduino framework
- C++
- `board = seeed_xiao_esp32s3`

## Board

Target hardware for MVP is ESP32-S3 Sense, flashed over USB-C.

## Pins And Configuration

Exact pin mapping is not fixed yet and should be documented once hardware wiring is confirmed. For MVP, Wi-Fi SSID and password may live in `config.h`.

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

## RGB LED

Required states:

- ready: off or dim green
- recording: red
- uploading: blue
- success: short green flash
- Wi-Fi error: blinking yellow
- upload error: blinking red
- low battery: rare red blink

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
