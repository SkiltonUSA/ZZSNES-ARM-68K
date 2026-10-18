"""Original-ROM regression, with a negative control from the archived core."""
from pathlib import Path
import subprocess
import build_mesen_snes_system as fixture
from compatibility_native import build

root = Path(__file__).resolve().parents[1]
out = root / '.context/state-sprites'
out.mkdir(parents=True, exist_ok=True)
fixture.OUT = out
fixture.fixture()
source = root / 'tests/amiga/pocketsnes/state_sprites.c'
for name, options, expected in [
    ('archived', {'baseline': True}, 42),
    ('normal', {'baseline': True, 'snapshot_fix': True}, 0),
    ('superfx', {'fx3': True}, 0),
]:
    exe = build(source, out / name, **options)
    result = subprocess.run([exe, out / 'System-Test.sfc'], capture_output=True, text=True)
    print(name + ': ' + result.stdout, end='', flush=True)
    assert result.returncode == expected, result.stderr
print('PASS: archived failure reproduced; normal and Super FX builds restore sprites')
