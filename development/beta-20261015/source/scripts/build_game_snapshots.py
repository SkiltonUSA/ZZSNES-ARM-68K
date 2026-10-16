#!/usr/bin/env python3
"""Host-only snapshot candidates around the verified release-ui ARM images."""
from pathlib import Path
import hashlib, importlib.util, json, shutil, subprocess
from build_game_libraries import ROOT, COMMON
from amiga import PIN
IMAGE=PIN["image"]
from game_snapshots import frontend
from build_zz9000_debug import unpack_elf

OUT=ROOT/'.context/amiga/game-snapshots'
record={'scope':'Snapshot-enabled 68k hosts; exact existing ARM images', 'hardware_tested':False,'apps':{}}
for system, source, elf_name, binary, control, size in [
    ('nes','fceux-player','fceux.elf','ZZFceux-ui',0x80000,0x400000),
    ('snes','pocketsnes-library','pocket.elf','ZZSnes-ui',0x180000,0x2000000)]:
    old=ROOT/'.context/amiga'/source;stage=OUT/system;stage.mkdir(parents=True,exist_ok=True)
    meta=json.loads((old/('build.json' if system=='nes' else 'library-build.json')).read_text())
    expected=meta['outputs'][elf_name] if system=='nes' else meta['files'][elf_name]['sha256']
    assert hashlib.sha256((old/elf_name).read_bytes()).hexdigest()==expected
    for p in old.iterdir():
        if p.is_file() and p.suffix in ('.h','.c','.elf'):
            shutil.copy2(p,stage/p.name)
    # Keep view/theme validators paired with this unchanged historical ARM image.
    for name in ['snapshot_format.h','snapshot_host.h']:
        shutil.copy2(COMMON/name,stage/name)
    if system=='snes':
        p=stage/'session.h';p.write_text(p.read_text().replace('fc_view=16','fc_view=4').replace('ZZSESSION1','ZZSNESSESSION2'))
    p=stage/'host.c';p.write_text(frontend(p.read_text()))
    packpath=ROOT/('amiga/fceux_bench/elf.py' if system=='nes' else 'amiga/pocketsnes/elf.py')
    spec=importlib.util.spec_from_file_location('snapshot_packer',packpath);pack=importlib.util.module_from_spec(spec);spec.loader.exec_module(pack)
    # SNES library has the established larger BSS range in its staged packer.
    if system=='snes':
        spec=importlib.util.spec_from_file_location('snapshot_packer',old/'elf.py');pack=importlib.util.module_from_spec(spec);spec.loader.exec_module(pack)
    image,rel,entry=pack.unpack((stage/elf_name).read_bytes())
    mapping,mrel,point=unpack_elf((stage/'mapping.elf').read_bytes());assert not mrel and not point and len(mapping)<=1024
    array=lambda n,d:'static const unsigned char '+n+'[]={'+','.join(map(str,d))+'};\n'
    (stage/'zz_payload.h').write_text(f'#define ZZ_ENTRY {entry}u\n'+array('zz_image',image)+array('zz_mapping_probe',mapping)+'static const unsigned long zz_relocations[]={'+','.join(map(str,rel))+'};\n')
    relative=stage.relative_to(ROOT)
    flags=['-DZZ_HEADLESS_BENCH','-DZZ_RELEASE','-DZZ_NES',f'-DZZ_BLOCK_SIZE={hex(size)}',f'-DZZ_CONTROL={hex(control)}',
           f'-DZZ_APP_NAME="{binary}"','-DZZ_APP_VERSION="snapshot-preview"','-DZZ_MIN_STACK=65536',f'-DMB_ALLOCATION={hex(size)}u',
           '-DMB_MMU','-DMB_DCACHE','-DZZ_HEADLESS_MMU','-DZZ_HEADLESS_ICACHE','-DNES_PLAYER','-DNES_DIRECT','-DNES_FULLSCREEN']
    result=subprocess.run(['podman','--connection','nuflix-converter-root','run','--rm','--platform','linux/amd64','-v',f'{ROOT}:/work','-w','/work',IMAGE,
        'm68k-amigaos-gcc','-std=c99','-noixemul','-m68020','-O2','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-misleading-indentation',*flags,
        '-I'+str(relative),'-Iamiga/mesen_nes','-Iamiga/mesen_bench','-Iamiga/arm_debug','-Iamiga/arm_debug/zz9000',
        'amiga/arm_debug/zz9000/launcher.c',str(relative/'host.c'),'-lamiga','-o',str(relative/binary)],capture_output=True,text=True)
    (stage/'build.log').write_text(result.stdout+result.stderr);print(result.stdout+result.stderr,end='');result.check_returncode()
    data=(stage/binary).read_bytes();assert data[:4]==b'\0\0\x03\xf3'
    record['apps'][system]={'binary':binary,'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest(),'arm_elf_sha256':expected,
        'host_sha256':hashlib.sha256((stage/'host.c').read_bytes()).hexdigest(),
        'session_sha256':hashlib.sha256((stage/'session.h').read_bytes()).hexdigest(),
        'model_sha256':hashlib.sha256((stage/'ui_model.h').read_bytes()).hexdigest(),'arm_image_bytes':len(image)}
    print(f'Built {binary}: {len(data)} bytes; unchanged ARM ELF {expected}')
record['source_sha256']={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [COMMON/n for n in ['snapshot_format.h','snapshot_host.h']]+[ROOT/'scripts/game_snapshots.py',Path(__file__).resolve()]}
OUT.mkdir(parents=True,exist_ok=True)
(OUT/'build.json').write_text(json.dumps(record,indent=2)+'\n')
