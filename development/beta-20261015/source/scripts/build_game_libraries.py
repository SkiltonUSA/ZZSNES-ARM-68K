#!/usr/bin/env python3
"""Build only the shared native MUI launchers. No ARM rebuild or deployment."""
import hashlib
import json
import shutil
import subprocess
import argparse
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / '.context/amiga/game-libraries'
COMMON = ROOT / 'amiga/fceux_player'
SNES = ROOT / 'amiga/pocketsnes/library'
IMAGE = 'docker.io/stefanreinauer/amiga-gcc@sha256:2ecf74c5a552a16251c3e311183b1718539d2e89af8e082b08c23aa815d7dbfd'

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--preview',action='store_true');args=parser.parse_args()
    record = {'scope': 'Native library launchers only; existing player and save ABI unchanged',
              'hardware_tested': False, 'deployed': False, 'parallel_preview': args.preview, 'image': IMAGE, 'apps': {}}
    for name, source in [('ZZNesLibrary', COMMON), ('ZZSnesLibrary', SNES)]:
        stage = OUT / name
        stage.mkdir(parents=True, exist_ok=True)
        inputs = [COMMON / p for p in ['ui_model.h', 'library_mui.h', 'library_preview.h','library_saves.h','library_controller.h','snapshot_format.h']]
        inputs += [source / 'launcher.c', source / 'cartridge.h']
        if source == SNES:
            inputs += [SNES / 'state_format.h']
        for p in inputs:
            shutil.copy2(p, stage / p.name)
        relative = stage.relative_to(ROOT)
        command = ['podman', '--connection', 'nuflix-converter-root', 'run', '--rm',
                   '--platform', 'linux/amd64', '-v', f'{ROOT}:/work', '-w', '/work', IMAGE,
                   'm68k-amigaos-gcc', '-std=c99', '-noixemul', '-m68020', '-O2', '-g',
                   '-Wall', '-Wextra', '-Werror', '-Wno-unused-function', '-Wno-misleading-indentation',
                   *(['-DLIBRARY_PREVIEW'] if args.preview else []),
                   str(relative / 'launcher.c'), '-lamiga', '-o', str(relative / name)]
        result = subprocess.run(command, cwd=ROOT, text=True, capture_output=True)
        (stage / 'build.log').write_text(result.stdout + result.stderr)
        print(result.stdout + result.stderr, end='')
        result.check_returncode()
        binary = (stage / name).read_bytes()
        assert binary[:4] == b'\0\0\x03\xf3'
        shutil.copy2(ROOT / 'docs/amiga-game-library-ui.md', stage / 'ReadMe.md')
        record['apps'][name] = {'bytes': len(binary), 'sha256': hashlib.sha256(binary).hexdigest(),
            'sources': {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs}}
        print(f'Built {name}: {len(binary)} bytes')
    (OUT / 'build.json').write_text(json.dumps(record, indent=2) + '\n')

if __name__ == '__main__':
    main()
