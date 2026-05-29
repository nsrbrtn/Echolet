# ROADMAP

## Stage 1 — Button + RGB LED

Goal:

Holding the button turns on the red LED and releasing it returns the device to idle state.

## Stage 2 — microSD

Goal:

The board creates a test file on microSD successfully.

## Stage 3 — WAV Recording

Goal:

Holding the button records a WAV file to microSD.

## Stage 4 — Wi-Fi

Goal:

The board connects to the home Wi-Fi network.

## Stage 5 — HTTP Upload

Goal:

The WAV file is uploaded to the FastAPI server.

## Stage 6 — Queue

Goal:

Unsent files are not lost and can be uploaded later.

## Stage 7 — Cleanup

Goal:

If microSD usage exceeds 80%, old files from `sent` are removed.

## Stage 8 — Server Integration

Goal:

FastAPI receives the file, verifies auth, runs `faster-whisper`, stores audio in the vault, and creates an inbox markdown file for OpenClaw.

## Stage 9 — Obsidian Note

Goal:

After a voice recording is uploaded, an inbox Markdown file with audio attachment appears in Obsidian.

## Stage 10 — OpenClaw Processing

Goal:

OpenClaw reads inbox items, creates final notes/tasks/reminders, and marks processed captures.
