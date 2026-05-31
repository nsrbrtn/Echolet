# Echolet Hardware Smoke Test

This checklist is for quick validation of the MVP 0.1 firmware on the real device.

## Preconditions

- Firmware is flashed successfully.
- microSD is inserted and mounted.
- FastAPI upload server is running on the configured laptop endpoint.
- `kWifiSsid`, `kWifiPassword`, `kUploadEndpoint`, and `kUploadToken` in [src/config.h](/home/artem/develop/Echolet/echolet-firmware/src/config.h:1) match the test environment.
- Serial monitor is open at `115200`.

## 1. Boot And Idle

- Power on the device.
- Confirm startup LED animation: red -> green -> blue.
- Confirm serial shows `[echolet] boot`.
- Confirm serial shows `[boot] storage ready`.
- Confirm idle LED is off or dim green.

## 2. Record To SD

- Hold the record button.
- Confirm LED turns red while the button is held.
- Confirm serial shows `[recording] started: ...`.
- Release the record button after 2-3 seconds.
- Confirm serial shows `[recording] stopped: ...`.
- Confirm a new WAV file appears in `/echolet/audio/new/` or is immediately processed for upload.

## 3. Online Upload Happy Path

- Keep the home Wi-Fi available.
- Record one short clip.
- Confirm serial shows Wi-Fi connection and upload attempt.
- Confirm serial shows `[upload] success`.
- Confirm the uploaded file is moved to `/echolet/audio/sent/`.
- Confirm the server receives:
  - `file`
  - `device_id`
  - `created_at`
  - `battery`
- Confirm the created note/event appears on the laptop side.

## 4. Offline Queue

- Turn off the home Wi-Fi or make the configured SSID unavailable.
- Record one short clip.
- Confirm serial shows Wi-Fi connect failure.
- Confirm serial shows `[queue] enqueued: ...`.
- Confirm LED switches to queued/offline indication.
- Confirm `/echolet/queue.json` contains the queued item.
- Confirm the audio file remains on SD and is not moved to `sent/`.

## 5. Reboot Or Deep Sleep Recovery

- With at least one item queued, reboot the device or send it to deep sleep and wake it again.
- Confirm serial shows `[queue] restored items: ...` during boot.
- Confirm the queued item is still present in `/echolet/queue.json`.
- Confirm the device does not forget the pending upload after restart.

## 6. Retry After Wi-Fi Returns

- Restore the home Wi-Fi.
- Wait for retry or trigger a new loop cycle after boot.
- Confirm serial shows `[queue] retry pending file: ...`.
- Confirm serial shows `[upload] success`.
- Confirm the file moves from `/echolet/audio/new/` to `/echolet/audio/sent/`.
- Confirm the item is removed from `/echolet/queue.json`.

## 7. Sent Cleanup

- Fill the SD card enough that reported usage exceeds 80%.
- Ensure `/echolet/audio/sent/` contains several old uploaded files.
- Upload one more file successfully.
- Confirm serial shows cleanup starting because usage is above threshold.
- Confirm the oldest files are deleted only from `/echolet/audio/sent/`.
- Confirm files in `/echolet/audio/new/` are untouched.
- Confirm files in `/echolet/audio/failed/` are untouched.

## 8. Power Button Safety

- While idle, hold the power button for at least `kPowerButtonHoldMs`.
- Confirm serial shows power-off pending behavior.
- Release the button.
- Confirm the device enters deep sleep.
- Repeat the long hold during recording or upload.
- Confirm serial shows hold is ignored in non-safe states.

## Expected Result

The firmware should:

- record audio while the button is held,
- save WAV files to SD,
- upload immediately when Wi-Fi is available,
- queue uploads safely when offline,
- restore queued uploads after reboot or deep sleep,
- move successful uploads to `sent/`,
- and delete only the oldest `sent/` files when SD usage exceeds 80%.
