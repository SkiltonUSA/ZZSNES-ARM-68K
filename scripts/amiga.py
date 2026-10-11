"""Standalone cross-toolchain configuration; no connection or deployment code."""
import json
import os
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PIN = json.loads((ROOT / "amiga/upstream.json").read_text())

def container_command():
    command = json.loads(os.environ.get("AMIGA_CONTAINER_COMMAND", '["docker"]'))
    if not isinstance(command, list) or not command or not all(isinstance(x, str) and x for x in command):
        raise SystemExit("AMIGA_CONTAINER_COMMAND must be a nonempty JSON array of strings.")
    return command
