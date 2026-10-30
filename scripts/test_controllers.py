#!/usr/bin/env python3
"""Native checks; no physical USB/gameport access."""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]

with tempfile.TemporaryDirectory(prefix='zzsnes-controllers-') as temp:
    out = Path(temp)
    # The transport test supplies mock APIs; production still uses the real SDK.
    for name in ('proto/poseidon.h', 'proto/potgo.h', 'clib/alib_protos.h', 'utility/hooks.h'):
        path = out / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.touch()
    for name, source in [('decoder', 'development/controllers-20261030/test_pad.c'),
                         ('transport', 'tests/controllers/transport.c')]:
        executable = out / name
        subprocess.run([os.environ.get('CC', 'cc'), '-std=c99', '-Wall', '-Wextra',
                        '-Werror', '-Wno-unused-function', '-Wno-misleading-indentation',
                        '-fsanitize=address,undefined', '-I'+str(out),
                        str(ROOT/source), '-o', str(executable)], check=True)
        subprocess.run([str(executable)], cwd=out, check=True)
