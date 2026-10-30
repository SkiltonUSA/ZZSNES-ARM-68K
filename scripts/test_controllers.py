#!/usr/bin/env python3
"""Native checks; no physical USB/gameport access."""
from pathlib import Path
import os
import subprocess
import tempfile
import shutil
from prepare_controllers import prepare_player

ROOT = Path(__file__).resolve().parents[1]

with tempfile.TemporaryDirectory(prefix='zzsnes-controllers-') as temp:
    out = Path(temp)
    # The transport test supplies mock APIs; production still uses the real SDK.
    for name in ('proto/poseidon.h', 'proto/potgo.h', 'clib/alib_protos.h', 'utility/hooks.h', 'libraries/lowlevel.h', 'proto/lowlevel.h'):
        path = out / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.touch()
    shutil.copy2(ROOT/'development/expanded-20261019/host/joystick.h', out/'joystick.h')
    shutil.copy2(ROOT/'development/expanded-20261019/common/input_config.h', out/'input_config.h')
    prepare_player(out)
    # Use the same production header already included by the transport model.
    (out/'pad_amiga.h').unlink()
    for name, source in [('decoder', 'development/controllers-20261030/test_pad.c'),
                         ('transport', 'tests/controllers/transport.c'),
                         ('player', 'tests/controllers/player.c')]:
        executable = out / name
        subprocess.run([os.environ.get('CC', 'cc'), '-std=c99', '-Wall', '-Wextra',
                        '-Werror', '-Wno-unused-function', '-Wno-misleading-indentation',
                        '-fsanitize=address,undefined', '-I'+str(out), '-I'+str(ROOT/'development/controllers-20261030'),
                        '-Wno-format',
                        str(ROOT/source), '-o', str(executable)], check=True)
        subprocess.run([str(executable)], cwd=out, check=True)
