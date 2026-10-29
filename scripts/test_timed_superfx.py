#!/usr/bin/env python3
"""Original GSU timing fixture, under ASan/UBSan. No hardware access."""
import os
import subprocess
from pathlib import Path
from compatibility_native import build

ROOT = Path(__file__).resolve().parents[1]
if __name__ == '__main__':
    executable = build(ROOT / 'development/timed-superfx-20261029/tests/timing.c',
                       ROOT / '.context/timed-superfx-native', fx3=True,
                       expanded=True, timed_fx=True)
    subprocess.run([executable], check=True,
                   env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0'))
