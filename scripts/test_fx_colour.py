#!/usr/bin/env python3
"""Exercise GSU pixels, RAM mapping and execution with an original cartridge."""
import os
import subprocess
from pathlib import Path

from compatibility_native import build


def main():
    root = Path(__file__).resolve().parents[1]
    executable = build(
        root / 'development/expanded-20261019/tests/fx_colour.c',
        root / '.context/fx-colour', fx3=True, expanded=True,
    )
    subprocess.run(
        [executable], check=True,
        env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0'),
    )


if __name__ == '__main__':
    main()
