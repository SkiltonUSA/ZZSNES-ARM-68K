#!/usr/bin/env python3
"""Sanitizer tests of generated FCEUX worker, pacing and fullscreen slot logic.

Build the desired scheduling variant first. This models ownership and sample
order, not real AmigaOS scheduling, audio quality or cache coherence.
"""
from pathlib import Path
import os
import subprocess

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / '.context/amiga/fceux-player'
source = (OUT / 'player_audio.h').read_text().split('static void player_audio_close')[0]
source = source.replace('#include <dos/dostags.h>', '')
# Remove wall-clock wrappers only; preserve the actual worker logic.
source = source.replace('profile_send_io(', 'SendIO(').replace('profile_wait_io(', 'WaitIO(')
(OUT / 'audio-worker-under-test.h').write_text(source)
for name, path, includes in [
    ('audio', 'tests/amiga/fceux_player/audio_worker.c', [OUT]),
    ('pace', 'tests/amiga/fceux_player/pace.c', [ROOT / 'amiga/fceux_player']),
    ('fast-pixels', 'tests/amiga/fceux_player/fast_pixels.c', [ROOT / 'amiga/fceux_player', ROOT / 'amiga/mesen_nes', OUT]),
    ('flip', 'tests/amiga/mesen_nes/flip_plan.c', [ROOT / 'amiga/mesen_nes']),
]:
    binary = OUT / (name + '-host-test')
    subprocess.run(['clang', '-std=c99', '-O2', '-g', '-fsanitize=address,undefined',
                    *(['-DFC_STATE_AUDIO'] if 'audio_state_drain' in source else []),
                    *['-I' + str(p) for p in includes], str(ROOT / path), '-o', str(binary)], check=True)
    subprocess.run([binary], check=True, env=os.environ | {'UBSAN_OPTIONS': 'halt_on_error=1'})
if '#define NES_FRAME_PRESENT' in (OUT / 'wire.h').read_text():
    import sys
    subprocess.run([sys.executable, ROOT / 'tests/amiga/fceux_player/check_pipeline.py'], check=True)
if '#include "joystick.h"' in (OUT / 'host.c').read_text():
    import sys
    subprocess.run([sys.executable, ROOT / 'tests/amiga/fceux_player/check_release.py'], check=True)
print('PASS generated FCEUX audio worker, bounded cadence and flip plan; no hardware accessed')
