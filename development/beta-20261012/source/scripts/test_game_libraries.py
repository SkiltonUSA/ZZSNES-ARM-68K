#!/usr/bin/env python3
"""Native sanitizer tests for the shared ROM library model, for both systems."""
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / '.context/amiga/game-libraries'
OUT.mkdir(parents=True, exist_ok=True)
logs = []
for system, flags in [('nes', []), ('snes', ['-DLIBRARY_SNES', '-DUI_CONFIG_PREFIX="ZZSNESUI"'])]:
  for test in ['ui_model', 'library_saves', 'library_delete']:
    binary = OUT / f'test-{test}-{system}'
    subprocess.run(['clang', '-std=c99', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function',
                    '-fsanitize=address,undefined', '-fno-sanitize-recover=all', *flags,
                    '-I' + str(ROOT / 'amiga/fceux_player'),
                    '-I' + str(ROOT / 'amiga/pocketsnes/library'),
                    str(ROOT / f'tests/amiga/fceux_player/{test}.c'), '-o', str(binary)], check=True)
    result = subprocess.run([binary], text=True, capture_output=True)
    logs.append(system + ': ' + result.stdout + result.stderr)
    (OUT / 'tests.log').write_text('\n'.join(logs))
    print(logs[-1], end='')
    result.check_returncode()
    if test == 'library_saves':
        fixtures = ([ROOT / '.context/nes-ui/hardware-demo.zzs', ROOT / '.context/amiga/fceux-player/cold-state.zzs']
                    if system == 'nes' else [ROOT / '.context/amiga/pocketsnes-library/model-state.zzs'])
        for fixture in fixtures:
            if not fixture.exists():
                logs.append(f'{system}: SKIP optional local fixture {fixture.name}\n')
                continue
            result = subprocess.run([binary, fixture], text=True, capture_output=True)
            logs.append(system + ': ' + fixture.name + ': ' + result.stdout + result.stderr)
            (OUT / 'tests.log').write_text('\n'.join(logs))
            print(logs[-1], end='')
            result.check_returncode()
