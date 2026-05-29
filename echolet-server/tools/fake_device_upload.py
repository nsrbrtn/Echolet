from __future__ import annotations

import argparse
import json
from datetime import datetime, timezone
from pathlib import Path

import httpx


DEFAULT_URL = "http://127.0.0.1:8765/api/v1/upload-audio"


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Upload a WAV file to echolet-server as a fake device."
    )
    parser.add_argument("wav_file", type=Path, help="Path to WAV file")
    parser.add_argument("--url", default=DEFAULT_URL, help="Upload endpoint URL")
    parser.add_argument("--token", required=True, help="Bearer token")
    parser.add_argument("--device-id", default="echolet-001", help="Device identifier")
    parser.add_argument(
        "--created-at",
        default=datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z"),
        help="ISO 8601 timestamp, for example 2026-05-29T10:30:45Z",
    )
    parser.add_argument("--battery", default="unknown", help="Battery percent or 'unknown'")
    parser.add_argument(
        "--firmware-version",
        default="0.1.0",
        help="Firmware version, for example 0.1.0",
    )
    return parser


def main() -> int:
    args = build_parser().parse_args()
    wav_path = args.wav_file.expanduser().resolve()

    if not wav_path.exists():
        raise SystemExit(f"File not found: {wav_path}")

    with wav_path.open("rb") as wav_file:
        files = {
            "file": (wav_path.name, wav_file, "audio/wav"),
        }
        data = {
            "device_id": args.device_id,
            "created_at": args.created_at,
            "battery": args.battery,
            "firmware_version": args.firmware_version,
        }
        headers = {"Authorization": f"Bearer {args.token}"}

        response = httpx.post(args.url, headers=headers, data=data, files=files, timeout=30.0)

    print(f"HTTP {response.status_code}")
    try:
        print(json.dumps(response.json(), indent=2, ensure_ascii=False))
    except json.JSONDecodeError:
        print(response.text)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
