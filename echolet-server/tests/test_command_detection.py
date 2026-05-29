from app.command_detection import detect_command


def test_detects_note_from_zametka_prefix() -> None:
    result = detect_command("Заметка: тест")

    assert result.detected_command == "note"
    assert result.command_confidence == "explicit"
    assert result.clean_text == "тест"


def test_detects_idea_prefix() -> None:
    result = detect_command("Идея: новый корпус")

    assert result.detected_command == "idea"
    assert result.command_confidence == "explicit"
    assert result.clean_text == "новый корпус"


def test_detects_task_prefix() -> None:
    result = detect_command("Задача: проверить сервер")

    assert result.detected_command == "task"
    assert result.command_confidence == "explicit"
    assert result.clean_text == "проверить сервер"


def test_detects_reminder_prefix() -> None:
    result = detect_command("Напоминание: завтра оплатить интернет")

    assert result.detected_command == "reminder"
    assert result.command_confidence == "explicit"
    assert result.clean_text == "завтра оплатить интернет"


def test_detects_napomni_prefix() -> None:
    result = detect_command("Напомни: вечером позвонить")

    assert result.detected_command == "reminder"
    assert result.command_confidence == "explicit"
    assert result.clean_text == "вечером позвонить"


def test_falls_back_to_note_for_plain_text() -> None:
    result = detect_command("Просто обычная мысль")

    assert result.detected_command == "note"
    assert result.command_confidence == "fallback"
    assert result.clean_text == "Просто обычная мысль"
