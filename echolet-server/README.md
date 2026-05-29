# echolet-server

Локальный FastAPI-сервер для `echolet`, который принимает аудио, транскрибирует его через `faster-whisper`, складывает материалы в Obsidian vault и создаёт входящий markdown-файл для OpenClaw.

## Новая архитектура

```text
ESP32-S3 Sense
  ->
FastAPI server
  ->
faster-whisper transcription
  ->
Obsidian vault / 80-89 AI/85 Echolet
  ->
OpenClaw
  ->
финальные заметки / задачи / напоминания
```

Смысл простой:

- сервер принимает WAV и проверяет Bearer token
- сервер сохраняет исходное аудио во временное storage
- сервер по умолчанию транскрибирует аудио через `faster-whisper`
- сервер создаёт один markdown-файл входящей записи в `80-89 AI/85 Echolet`
- сервер копирует исходное аудио в `80-89 AI/85 Echolet/_attachments/YYYY/MM/`
- сервер не создаёт финальные заметки в других папках
- сервер не создаёт задачи
- сервер не отправляет Telegram
- сервер не решает, что делать с записью, кроме необязательного hint в frontmatter

## Структура проекта

```text
echolet-server/
  app/
    main.py
    config.py
    auth.py
    storage.py
    transcription.py
    inbox.py
    command_detection.py
    events.py
  storage/
    audio/
  tools/
    fake_device_upload.py
  tests/
    test_upload.py
    test_command_detection.py
  requirements.txt
  README.md
```

`events.py` можно держать как legacy-модуль, но основной формат теперь markdown во vault. JSON inbox дальше не развиваем без отдельной команды.

## Настройка

```bash
cd echolet-server
python3 -m venv .venv
. .venv/bin/activate
pip install -r requirements.txt
```

Пример `.env`:

```dotenv
ECHOLET_TOKEN=test-token
OBSIDIAN_VAULT_PATH=/absolute/path/to/obsidian/vault
ECHOLET_INBOX_DIR=80-89 AI/85 Echolet
ECHOLET_ATTACHMENTS_DIR=80-89 AI/85 Echolet/_attachments
SERVER_TRANSCRIBE=true
WHISPER_MODEL=small
WHISPER_LANGUAGE=ru
WHISPER_DEVICE=cpu
WHISPER_COMPUTE_TYPE=int8
```

Поддерживается и старый `ECHOLET_API_TOKEN`, но нормальное имя теперь `ECHOLET_TOKEN`.

Значения по умолчанию:

```dotenv
OBSIDIAN_VAULT_PATH=./dev_obsidian_vault
ECHOLET_INBOX_DIR=80-89 AI/85 Echolet
ECHOLET_ATTACHMENTS_DIR=80-89 AI/85 Echolet/_attachments
SERVER_TRANSCRIBE=true
WHISPER_MODEL=medium
WHISPER_LANGUAGE=ru
WHISPER_DEVICE=cpu
WHISPER_COMPUTE_TYPE=int8
```

Если нужных папок нет, сервер создаёт их сам. Пути задаются через `pathlib`, так что пробелы в названиях директорий работают нормально.

## Запуск

```bash
cd echolet-server
export ECHOLET_TOKEN="change-me"
export OBSIDIAN_VAULT_PATH="/absolute/path/to/obsidian/vault"
export ECHOLET_INBOX_DIR="80-89 AI/85 Echolet"
export ECHOLET_ATTACHMENTS_DIR="80-89 AI/85 Echolet/_attachments"
export SERVER_TRANSCRIBE="true"
export WHISPER_MODEL="medium"
export WHISPER_LANGUAGE="ru"
export WHISPER_DEVICE="cpu"
export WHISPER_COMPUTE_TYPE="int8"
uvicorn app.main:app --host 127.0.0.1 --port 8765 --reload
```

По умолчанию временное серверное аудио хранится в `echolet-server/storage/audio/`.

По умолчанию сервер запускает распознавание через `faster-whisper` и пишет транскрипт во входящий markdown.

Если захочешь временно отключить серверную транскрибацию и оставить только сохранение аудио с inbox-markdown:

```bash
export SERVER_TRANSCRIBE="false"
```

Когда `SERVER_TRANSCRIBE=true`, сервер использует `faster-whisper` с `WHISPER_MODEL`, `WHISPER_LANGUAGE`, `WHISPER_DEVICE` и `WHISPER_COMPUTE_TYPE`.

## Что создаёт сервер

Структура во vault:

```text
80-89 AI/
  85 Echolet/
    2026-05-29 18-30-00 echolet.md
    _attachments/
      2026/
        05/
          audio-2026-05-29-18-30-00.wav
```

Имена файлов:

- markdown: `YYYY-MM-DD HH-MM-SS echolet.md`
- audio: `audio-YYYY-MM-DD-HH-MM-SS.wav`

Если имя уже занято:

- `2026-05-29 18-30-00 echolet 2.md`
- `audio-2026-05-29-18-30-00-2.wav`

## Формат markdown-файла

Пример:

```md
---
source: echolet
schema_version: "1.0"
event_type: voice_capture
status: new
created_at: "2026-05-29T18:30:00"
received_at: "2026-05-29T18:30:05"
device_id: "echolet-001"
language: "ru"
audio: "_attachments/2026/05/audio-2026-05-29-18-30-00.wav"
battery: "83"
firmware_version: "0.1.0"
detected_command_hint: "note"
command_confidence: "explicit"
---

# Echolet capture 2026-05-29 18:30

## Transcript

Заметка: надо подумать над корпусом устройства.

## Original audio

[[audio-2026-05-29-18-30-00.wav]]

## Instructions for OpenClaw

OpenClaw should process this capture and decide what to do with it.

Rules:

- If the transcript starts with `заметка`, create a regular Obsidian note.
- If the transcript starts with `идея`, create an idea note.
- If the transcript starts with `задача`, create a task in Obsidian.
- If the transcript starts with `напоминание` or `напомни`, create or schedule a Telegram reminder.
- If unsure, save as a regular note.
- Reference or attach the original audio file when useful.
- After successful processing, OpenClaw may update `status` from `new` to `processed`.
```

Если `SERVER_TRANSCRIBE=true`, сервер записывает в markdown транскрипт и hint-поля `detected_command_hint` / `command_confidence`. Это дефолтный режим.

## Пример запроса

```bash
curl -X POST "http://127.0.0.1:8765/api/v1/upload-audio" \
  -H "Authorization: Bearer change-me" \
  -F "file=@sample.wav" \
  -F "device_id=echolet-001" \
  -F "created_at=2026-05-29T10:30:45Z" \
  -F "battery=87" \
  -F "firmware_version=0.1.0"
```

Ожидаемый ответ:

```json
{
  "ok": true,
  "status": "received",
  "event_id": "echolet-20260529133045",
  "transcription_status": "pending",
  "whisper_model": "medium",
  "inbox_dir": "80-89 AI/85 Echolet"
}
```

## Проверка через fake_device_upload.py

```bash
cd echolet-server
. .venv/bin/activate
python3 tools/fake_device_upload.py ./sample.wav \
  --token change-me \
  --device-id echolet-001 \
  --created-at 2026-05-29T10:30:45Z \
  --battery 87 \
  --firmware-version 0.1.0
```

После отправки проверь:

- markdown в `80-89 AI/85 Echolet`
- audio в `80-89 AI/85 Echolet/_attachments/YYYY/MM/`

## Тесты

```bash
cd echolet-server
pytest
```

Тесты проверяют:

- upload создаёт markdown в `80-89 AI/85 Echolet`
- markdown содержит YAML frontmatter
- по умолчанию markdown создаётся с серверным transcript
- audio копируется в `_attachments/YYYY/MM/`
- markdown содержит ссылку на audio
- `SERVER_TRANSCRIBE=true` включает серверную транскрибацию через `faster-whisper` и используется по умолчанию
- `SERVER_TRANSCRIBE=false` отключает серверную транскрибацию
- при ошибке Whisper markdown всё равно создаётся
- при ошибке Whisper ставится `status: transcription_failed`
- сервер не создаёт финальные markdown вне inbox-папки

## Что теперь делает OpenClaw

OpenClaw должен:

- читать входящие markdown-файлы из `80-89 AI/85 Echolet`
- создавать финальные заметки
- создавать задачи
- создавать или планировать напоминания
- при необходимости обновлять `status` у входящего файла

То есть сервер теперь готовит сырьё, а OpenClaw делает умную работу. Так и чище, и меньше дублирования логики.
