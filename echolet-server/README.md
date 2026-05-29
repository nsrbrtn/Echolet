# echolet-server

Минимальный локальный FastAPI-сервер для приёма аудиофайлов от будущего устройства `echolet`.

## Что делает сервер

- принимает `multipart/form-data` на `POST /api/v1/upload-audio`
- проверяет `Authorization: Bearer <TOKEN>`
- сохраняет аудиофайл в `storage/audio/YYYY/MM/`
- создаёт event JSON в `storage/events/`
- пока не запускает Whisper и сохраняет `"transcript": null`

## Структура

```text
echolet-server/
  app/
    main.py
    config.py
    auth.py
    storage.py
    events.py
  storage/
    audio/
    events/
  tests/
    test_upload.py
  requirements.txt
  README.md
```

## Установка

```bash
cd echolet-server
python3 -m pip install -r requirements.txt
```

## Запуск

```bash
cd echolet-server
export ECHOLET_API_TOKEN="change-me"
uvicorn app.main:app --reload
```

По умолчанию данные пишутся в `echolet-server/storage/`.

Если нужен другой каталог:

```bash
export ECHOLET_STORAGE_DIR="/absolute/path/to/storage"
```

## Пример запроса

```bash
curl -X POST "http://127.0.0.1:8000/api/v1/upload-audio" \
  -H "Authorization: Bearer change-me" \
  -F "file=@sample.wav" \
  -F "device_id=echolet-dev-1" \
  -F "created_at=2026-05-29T10:30:45Z" \
  -F "battery=87"
```

Ожидаемый ответ:

```json
{
  "ok": true,
  "status": "received",
  "event_id": "..."
}
```

## Тесты

```bash
cd echolet-server
pytest
```
