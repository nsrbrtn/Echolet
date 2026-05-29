# echolet-server

Минимальный локальный FastAPI-сервер для приёма аудиофайлов от будущего устройства `echolet`.

## Что делает сервер

- принимает `multipart/form-data` на `POST /api/v1/upload-audio`
- проверяет `Authorization: Bearer <TOKEN>`
- сохраняет аудиофайл в `storage/audio/YYYY/MM/`
- создаёт event JSON в `storage/events/`
- сразу отвечает на upload и запускает локальный Whisper CLI в фоне
- дописывает транскрипт, тип команды и статус транскрибации в event JSON
- пишет в event JSON, какой моделью Whisper получен результат
- если Whisper падает, файл и event всё равно сохраняются

## Структура

```text
echolet-server/
  app/
    main.py
    config.py
    auth.py
    storage.py
    events.py
    transcription.py
    command_detection.py
  storage/
    audio/
    events/
  tools/
    fake_device_upload.py
  tests/
    test_upload.py
  requirements.txt
  README.md
```

## Установка

```bash
cd echolet-server
python3 -m venv .venv
. .venv/bin/activate
pip install -r requirements.txt
```

## Запуск

```bash
cd echolet-server
export ECHOLET_API_TOKEN="change-me"
export WHISPER_MODEL="medium"
export WHISPER_LANGUAGE="ru"
uvicorn app.main:app --host 127.0.0.1 --port 8765 --reload
```

По умолчанию данные пишутся в `echolet-server/storage/`.

Если `openai-whisper` установлен в проектную `.venv`, сервер сам найдёт `.venv/bin/whisper`. Явно задавать `WHISPER_COMMAND` нужно только если у тебя другой путь к CLI.

Если нужен другой каталог:

```bash
export ECHOLET_STORAGE_DIR="/absolute/path/to/storage"
```

## Настройка Whisper

Сервер вызывает локальный Whisper CLI. Нормальный базовый вариант для этого проекта:

```bash
cd echolet-server
python3 -m venv .venv
. .venv/bin/activate
pip install -r requirements.txt
```

`requirements.txt` включает `imageio-ffmpeg`, поэтому проектная `.venv` даёт Whisper рабочий `ffmpeg` без отдельной системной установки.

По умолчанию сервер использует модель `medium`. Это теперь дефолт проекта, так что `WHISPER_MODEL` нужен только если хочешь сознательно переопределить модель.

Минимальная конфигурация окружения:

```bash
export WHISPER_MODEL="medium"
export WHISPER_LANGUAGE="ru"
```

Если хочется быстрее, но хуже по качеству, можно вручную откатиться на `small`:

```bash
export WHISPER_MODEL="small"
```

Если `whisper` лежит не в проектной `.venv`, можно указать путь явно:

```bash
export WHISPER_COMMAND="/absolute/path/to/whisper"
```

Если хочешь использовать системный `ffmpeg` вместо bundled-варианта, это тоже нормально:

```bash
sudo apt-get update
sudo apt-get install -y ffmpeg
```

При загрузке сервер:

1. сохраняет WAV
2. сразу создаёт event JSON со статусом `pending`
3. возвращает `200 OK` без ожидания Whisper
4. в фоне запускает локальный Whisper CLI
5. после транскрибации определяет тип команды по префиксу
6. обновляет event JSON до `completed` или `failed`

Если Whisper не отработал, upload всё равно считается принятым: `ok=true`, файл остаётся на диске, а event JSON обновляется с `transcription_status: "failed"`, `transcript: null` и `transcription_error`.

## MVP command detection

После получения `transcript` сервер делает простую классификацию по началу текста.

Поддерживаемые голосовые префиксы:

- `заметка:` -> `note`
- `запись:` -> `note`
- `мысль:` -> `note`
- `идея:` -> `idea`
- `задача:` -> `task`
- `напоминание:` -> `reminder`
- `напомни:` -> `reminder`

Регистр не важен.

Если префикс найден явно, в event JSON пишется:

- `detected_command: "note" | "idea" | "task" | "reminder"`
- `command_confidence: "explicit"`
- `clean_text`: текст без префикса

Если префикс не найден, используется fallback:

- `detected_command: "note"`
- `command_confidence: "fallback"`
- `clean_text`: исходный transcript

Примеры:

```text
"Заметка: надо проверить страницу дизайн-поддержки"
-> detected_command: "note"
-> clean_text: "надо проверить страницу дизайн-поддержки"

"Напоминание: завтра оплатить интернет"
-> detected_command: "reminder"
-> clean_text: "завтра оплатить интернет"

"Надо подумать над корпусом устройства"
-> detected_command: "note"
-> command_confidence: "fallback"
-> clean_text: "Надо подумать над корпусом устройства"
```

Это специально тупая MVP-логика. Более умная классификация появится позже, когда сюда подключится OpenClaw.

## Пример запроса

```bash
curl -X POST "http://127.0.0.1:8765/api/v1/upload-audio" \
  -H "Authorization: Bearer change-me" \
  -F "file=@sample.wav" \
  -F "device_id=echolet-dev-1" \
  -F "created_at=2026-05-29T10:30:45Z" \
  -F "battery=87" \
  -F "firmware_version=0.1.0"
```

Ожидаемый ответ:

```json
{
  "ok": true,
  "status": "received",
  "event_id": "...",
  "transcription_status": "pending"
}
```

## Тестовый клиент

Скрипт `tools/fake_device_upload.py` имитирует устройство и отправляет WAV-файл на локальный сервер.

Пример запуска:

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

После отправки проверь event JSON в `storage/events/` или в каталоге из `ECHOLET_STORAGE_DIR`.

Пример ожидаемого event JSON:

```json
{
  "event_id": "8674fa22b23c4dfca17315bfa98f7e7c",
  "kind": "audio_uploaded",
  "received_at": "2026-05-29T13:51:42.727853+00:00",
  "source": "echolet",
  "device_id": "echolet-dev-1",
  "type": "voice",
  "created": "2026-05-29T10:30:45Z",
  "battery": "87",
  "firmware_version": "0.1.0",
  "original_filename": "sample.wav",
  "audio_path": "/absolute/path/to/storage/audio/2026/05/20260529T103045_deadbeef.wav",
  "whisper_model": "medium",
  "transcription_status": "completed",
  "transcript": "Напоминание: завтра оплатить интернет",
  "detected_command": "reminder",
  "command_confidence": "explicit",
  "clean_text": "завтра оплатить интернет",
  "status": "new",
  "transcription_error": null,
  "transcription_finished_at": "2026-05-29T13:52:10.123456+00:00"
}
```

## Тесты

```bash
cd echolet-server
pytest
```
