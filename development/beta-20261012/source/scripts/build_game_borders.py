#!/usr/bin/env python3
"""Isolated matching player/library builds; reuse verified emulator core objects.

Requires the existing release-ui NES and LUT/STMIA SNES build caches. Rebuilds
only ARM presentation, repacks within original linker bounds, then builds 68k
hosts and MUI launchers. Never installs or launches on hardware.
"""
from pathlib import Path
import hashlib
import importlib.util
import json
import shutil
import subprocess
from amiga import PIN
from build_zz9000_debug import unpack_elf

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / '.context/amiga/game-borders'
COMMON = ROOT / 'amiga/fceux_player'

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def run(command):
    subprocess.run(list(map(str, command)), cwd=ROOT, check=True)

def inside(path):
    return '/work/' + str(path.relative_to(ROOT))

def main():
    record = {'hardware_tested': False, 'scope': 'Side-panel presentation and matching MUI choices; no core/save/cache/flip changes', 'apps': {}}
    for system in ['nes', 'snes']:
        snes = system == 'snes'
        stage = OUT / system
        stage.mkdir(parents=True, exist_ok=True)
        base = ROOT / ('.context/amiga/pocketsnes-schedule/lut-stmia' if snes else '.context/amiga/game-snapshots/nes')
        old = ROOT / ('.context/amiga/pocketsnes-library' if snes else '.context/amiga/fceux-player')
        src = ROOT / 'amiga/pocketsnes/library' if snes else COMMON
        elf = 'pocket.elf' if snes else 'fceux.elf'
        player = 'ZZSnes-ui' if snes else 'ZZFceux-ui'
        library = 'ZZSnesLibrary' if snes else 'ZZNesLibrary'
        for p in base.iterdir():
            if p.is_file() and p.suffix in ('.h', '.c', '.cpp', '.elf'):
                shutil.copy2(p, stage / p.name)
        for name in ['ui_model.h', 'session.h', 'library_mui.h', 'library_preview.h', 'library_saves.h','library_controller.h', 'snapshot_format.h']:
            shutil.copy2(COMMON / name, stage / name)
        for name in ['launcher.c', 'cartridge.h', 'view.h']:
            shutil.copy2(src / name, stage / name)
        for name in ['border_art.h', 'border_assets.h']:
            shutil.copy2(ROOT / 'amiga/borders' / name, stage / name)
        if snes:
            shutil.copy2(src / 'state_format.h', stage / 'state_format.h')
            p = stage / 'session.h'
            p.write_text(p.read_text().replace('fc_view=16', 'fc_view=4').replace('ZZSESSION1', 'ZZSNESSESSION2'))
            metadata = json.loads((base / 'present-build.json').read_text())
            flags = [v.replace(inside(base), inside(stage)) for v in metadata['arm_flags']]
            command = ['podman', '--connection', 'nuflix-converter-root', 'exec', 'zzffmpeg-toolchain']
            direct = stage / '31-direct.arm.o'
            run([*command, 'arm-none-eabi-g++', *flags, '-c', inside(stage / 'direct.cpp'), '-o', inside(direct)])
            objects = sorted(old.glob('*.arm.o'), key=lambda p: int(p.name.split('-')[0]))
            assert len(objects) == 38
            objects = [direct if p.name == direct.name else p for p in objects]
            run([*command, 'arm-none-eabi-ld', '-shared', '-Bsymbolic', '--gc-sections', '--exclude-libs=ALL', '--no-undefined',
                 '-T', inside(old / 'payload.ld'), '-Map='+inside(stage / 'link.map'), *map(inside, objects),
                 '--start-group', inside(old / 'libc.a'), '/usr/lib/gcc/arm-none-eabi/15/thumb/v7-a/nofp/libgcc.a', '--end-group', '-o', inside(stage / elf)])
            packpath = old / 'elf.py'
        else:
            shutil.copy2(old / 'direct.cpp', stage / 'direct.cpp')
            metadata = json.loads((old / 'build.json').read_text())
            flags = metadata['flags'] + ['-I'+str(stage), '-I'+str(ROOT / 'amiga/fceux_bench'), '-I'+str(ROOT / 'amiga/mesen_nes'), '-I'+str(ROOT / 'amiga/mesen_bench'), '-Iamiga/arm_debug', '-Iamiga/arm_debug/zz9000']
            direct = stage / 'fceux-player-direct.arm.o'
            run(['clang', *flags, '-std=c++14', '-fno-exceptions', '-fno-rtti', '-nostdinc++', '-c', stage / 'direct.cpp', '-o', direct])
            names = ['mesen_bench-mmu_asm', 'mesen_bench-mmu', 'mesen_bench-runtime', 'fceux_bench-runtime', 'fceux-player-worker', 'fceux_bench-core']
            names += ['fceux-player-'+n for n in ['x6502', 'ppu', 'sound', 'filter', 'cart', '90']]
            names += ['fceux_player-adapter']
            objects = [old / (n+'.arm.o') for n in names] + [direct]
            run(['/opt/homebrew/opt/lld/bin/ld.lld', '-shared', '-Bsymbolic', '--no-undefined', '-T', ROOT / 'amiga/fceux_bench/payload.ld', *objects, '-o', stage / elf])
            packpath = ROOT / 'amiga/fceux_bench/elf.py'
        spec = importlib.util.spec_from_file_location('border_pack', packpath)
        pack = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(pack)
        image, rel, entry = pack.unpack((stage / elf).read_bytes())
        mapping, mrel, point = unpack_elf((stage / 'mapping.elf').read_bytes())
        assert not mrel and not point and len(mapping) <= 1024
        array = lambda n,d: 'static const unsigned char '+n+'[]={'+','.join(map(str,d))+'};\n'
        (stage / 'zz_payload.h').write_text(f'#define ZZ_ENTRY {entry}u\n'+array('zz_image',image)+array('zz_mapping_probe',mapping)+'static const unsigned long zz_relocations[]={'+','.join(map(str,rel))+'};\n')
        size, control = (0x2000000, 0x180000) if snes else (0x400000, 0x80000)
        command = ['podman', '--connection', 'nuflix-converter-root', 'run', '--rm', '--platform', 'linux/amd64', '-v', f'{ROOT}:/work', '-w', '/work', PIN['image'], 'm68k-amigaos-gcc', '-std=c99', '-noixemul', '-m68020', '-O2', '-g', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function', '-Wno-misleading-indentation']
        flags = ['-DZZ_HEADLESS_BENCH', '-DZZ_RELEASE', '-DZZ_NES', f'-DZZ_BLOCK_SIZE={hex(size)}', f'-DZZ_CONTROL={hex(control)}', f'-DZZ_APP_NAME="{player}"', '-DZZ_APP_VERSION="border-preview"', '-DZZ_MIN_STACK=65536', f'-DMB_ALLOCATION={hex(size)}u', '-DMB_MMU', '-DMB_DCACHE', '-DZZ_HEADLESS_MMU', '-DZZ_HEADLESS_ICACHE', '-DNES_PLAYER', '-DNES_DIRECT', '-DNES_FULLSCREEN']
        relative = stage.relative_to(ROOT)
        run([*command, *flags, '-I'+str(relative), '-Iamiga/mesen_nes', '-Iamiga/mesen_bench', '-Iamiga/arm_debug', '-Iamiga/arm_debug/zz9000', 'amiga/arm_debug/zz9000/launcher.c', relative / 'host.c', '-lamiga', '-o', relative / player])
        run([*command, '-DLIBRARY_PREVIEW', relative / 'launcher.c', '-lamiga', '-o', relative / library])
        record['apps'][system] = {'base_elf_sha256': digest(base/elf), 'image_bytes': len(image), 'objects': {str(p.relative_to(ROOT)):digest(p) for p in objects}, 'outputs': {n:digest(stage/n) for n in [player,library,elf]}, 'sources': {str(p.relative_to(ROOT)):digest(p) for p in stage.glob('*') if p.suffix in ('.h','.c','.cpp')}}
        print('Built', system, player, library, 'ARM image bytes',len(image), flush=True)
    (OUT / 'build.json').write_text(json.dumps(record, indent=2)+'\n')

if __name__ == '__main__':
    main()
