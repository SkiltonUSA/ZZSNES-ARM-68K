#!/usr/bin/env python3
"""Build the separate Mesen2 SNES CPU diagnostic. No deployment or game support."""
import argparse
import difflib
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / 'amiga/mesen_snes'
COMMON = ROOT / 'amiga/mesen_bench'
OUT = ROOT / '.context/amiga/mesen-snes'

def prepare():
    OUT.mkdir(parents=True, exist_ok=True)
    manifest = json.loads((SRC/'vendor/upstream.json').read_text())
    patch = ''
    for name, info in manifest['files'].items():
        raw = (SRC/'vendor'/name).read_bytes()
        assert hashlib.sha256(raw).hexdigest() == info['sha256'], name
        if name == 'LICENSE':
            (OUT/name).write_bytes(raw)
            continue
        text = raw.decode('utf-8-sig')
        text = re.sub(r'^#include .*\n', '', text, flags=re.M)
        if name == 'SnesCpu.h':
            text = text.replace('class SnesCpu : public ISerializable', 'class SnesCpu')
            text = text.replace('virtual ~SnesCpu();', '~SnesCpu();')
            text = text.replace('\tvoid Serialize(Serializer &s) override;', '')
            text = '#include "platform.h"\n#include "SnesCpuTypes.h"\n'+text
        elif name == 'SnesCpuTypes.h':
            text = '#include "platform.h"\n'+text
        elif name == 'SnesCpu.Shared.h':
            text = text[:text.index('\nvoid SnesCpu::Serialize(Serializer &s)')]
        elif name == 'SnesCpu.cpp':
            text = '#include "platform.h"\n#include "SnesCpu.h"\n#include "SnesCpu.Instructions.h"\n#include "SnesCpu.Shared.h"\n'+text
        (OUT/name).write_text(text)
        patch += ''.join(difflib.unified_diff(raw.decode('utf-8-sig').splitlines(True),text.splitlines(True),'upstream/'+name,'generated/'+name))
    (OUT/'integration.patch').write_text(patch)
    return manifest

def build_arm(command):
    from build_zz9000_debug import unpack_elf
    from amiga import PIN, container_command
    inc = ['-I'+str(p) for p in [OUT,SRC,COMMON,ROOT/'amiga/arm_debug',ROOT/'amiga/arm_debug/zz9000']]
    flags = ['--target=arm-none-eabi','-mcpu=cortex-a9','-marm','-mfloat-abi=soft','-mno-unaligned-access',
        '-ffreestanding','-fno-builtin','-fPIC','-fvisibility=hidden','-fno-stack-protector','-O2','-g',
        '-DZZ_BLOCK_SIZE=0x100000','-DZZ_CONTROL=0x80000','-DMB_MMU','-DMB_DCACHE',*inc]
    sources = [COMMON/'mmu_asm.S',COMMON/'mmu.c',COMMON/'worker.cpp',COMMON/'runtime.c',OUT/'SnesCpu.cpp',SRC/'bench.cpp']
    objects=[]
    for src in sources:
        obj=OUT/(src.stem+'.arm.o');objects.append(obj)
        extra=['-std=c++17','-fno-exceptions','-fno-rtti','-nostdinc++'] if src.suffix=='.cpp' else []
        subprocess.run(['clang',*flags,*extra,'-c',src,'-o',obj],check=True)
    ld='/opt/homebrew/opt/lld/bin/ld.lld'
    elf=OUT/'snes.elf'
    subprocess.run([ld,'-shared','-Bsymbolic','--no-undefined','-T',COMMON/'mmu_payload.ld',*objects,'-o',elf],check=True)
    image,relocs,entry=unpack_elf(elf.read_bytes(),0x40000)
    subprocess.run(['clang',*flags,'-c',ROOT/'amiga/arm_debug/zz9000/mapping_probe.S','-o',OUT/'mapping.o'],check=True)
    subprocess.run([ld,'-shared','-Bsymbolic','--no-undefined','-T',ROOT/'amiga/arm_debug/zz9000/payload.ld',OUT/'mapping.o','-o',OUT/'mapping.elf'],check=True)
    mapping,rel,point=unpack_elf((OUT/'mapping.elf').read_bytes());assert not rel and not point and len(mapping)<=1024
    def array(name,data): return 'static const unsigned char '+name+'[]={'+','.join(map(str,data))+'};\n'
    (OUT/'zz_payload.h').write_text(f'#define ZZ_ENTRY {entry}u\n'+array('zz_image',image)+array('zz_mapping_probe',mapping)+
        'static const unsigned long zz_relocations[]={'+','.join(map(str,relocs))+'};\n')
    log=(OUT/'native.log').read_text()
    checked=int(re.search(r'checked=(\d+)',log)[1])
    cycles=int(re.search(r'cycles=(\d+)',log)[1]);h=re.search(r'hash=([0-9a-f]+)',log)[1]
    (OUT/'expected.h').write_text(f'#define MESEN_EXPECT_CYCLES {cycles}u\n#define MESEN_EXPECT_HASH 0x{h}u\n')
    host=(COMMON/'host.c').read_text().replace('8991',str(checked)).replace('MESEN CPU-ONLY','MESEN2 SNES CPU-ONLY').replace('MESEN_CPU PASS','MESEN_SNES_CPU PASS')
    (OUT/'host.c').write_text(host)
    relative=OUT.relative_to(ROOT)
    argv=command or container_command()
    if not isinstance(argv,list) or not argv or not all(isinstance(x,str) and x for x in argv): raise ValueError('Invalid container argv')
    subprocess.run([*argv,'run','--rm','--platform','linux/amd64','-v',f'{ROOT}:/work','-w','/work',PIN['image'],'m68k-amigaos-gcc',
        '-std=c99','-noixemul','-m68020','-O2','-g','-Wall','-Wextra','-Werror',
        '-DZZ_HEADLESS_BENCH','-DZZ_RELEASE','-DZZ_BLOCK_SIZE=0x100000','-DZZ_CONTROL=0x80000',
        '-DZZ_APP_NAME="ZZSnes-Mesen-CPU"','-DZZ_APP_VERSION="0.1.0-dev"','-DZZ_MIN_STACK=65536',
        '-DMB_MMU','-DMB_DCACHE','-DZZ_HEADLESS_MMU','-DZZ_HEADLESS_ICACHE',
        '-Iamiga/arm_debug','-Iamiga/arm_debug/zz9000','-Iamiga/mesen_bench','-I'+str(relative),
        'amiga/arm_debug/zz9000/launcher.c',str(relative/'host.c'),'-lamiga','-o',str(relative/'ZZSnes-Mesen-CPU')],cwd=ROOT,check=True)
    binary=OUT/'ZZSnes-Mesen-CPU';assert binary.read_bytes()[:4]==b'\0\0\x03\xf3'
    subprocess.run([*argv,'run','--rm','--platform','linux/amd64','-v',f'{ROOT}:/work','-w','/work',PIN['image'],
        'm68k-amigaos-gcc','-std=c99','-noixemul','-m68020','-O2','-g','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
        'amiga/mesen_snes/launcher.c','-lamiga','-o',str(relative/'ZZSnes-Mesen')],cwd=ROOT,check=True)
    ui=OUT/'ZZSnes-Mesen';assert ui.read_bytes()[:4]==b'\0\0\x03\xf3'
    sources += [ROOT/'scripts/build_mesen_snes.py',SRC/'platform.h',COMMON/'host.c',COMMON/'wire.h',COMMON/'bench.h',COMMON/'mmu.h',COMMON/'mmu_plan.h',COMMON/'mmu_payload.ld',ROOT/'amiga/arm_debug/zz9000/launcher.c',ROOT/'amiga/arm_debug/zz9000/layout.h',ROOT/'amiga/arm_debug/zz9000/mapping_probe.S',ROOT/'amiga/arm_debug/protocol.h']
    sources += [SRC/'launcher.c']
    record={'scope':'SNES 65C816 CPU smoke diagnostic only. No PPU/SPC/DSP/DMA/cartridge or game FPS.',
        'upstream':json.loads((SRC/'vendor/upstream.json').read_text()),'compiler':subprocess.check_output(['clang','--version'],text=True),
        'cache_policy':'Existing owned 1 MiB mapping; private inner WB/WA, outer/shared NC; MMU/I/D enabled.',
        'image_bytes':len(image),'relocations':len(relocs),'checked_instructions':checked,'expected_cycles':cycles,'expected_hash':h,
        'source_hashes':{str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sources if '.context' not in str(p.relative_to(ROOT))},
        'outputs':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in [elf,binary,ui,OUT/'integration.patch',OUT/'native.log']},
        'executable_md5':hashlib.md5(binary.read_bytes()).hexdigest()}
    (OUT/'build.json').write_text(json.dumps(record,indent=2)+'\n')
    print(f'Built {binary.name}: {binary.stat().st_size} bytes; ARM image {len(image)} bytes')

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--arm',action='store_true')
    p.add_argument('--container-command',help='JSON argv, does not change global container settings')
    a=p.parse_args();prepare()
    native=OUT/'snes-native'
    subprocess.run(['clang++','-std=c++17','-O2','-g','-fno-exceptions','-fno-rtti','-fsanitize=address,undefined',
        *['-I'+str(x) for x in [OUT,SRC,COMMON]],OUT/'SnesCpu.cpp',SRC/'bench.cpp',SRC/'native.cpp','-o',native],check=True)
    result=subprocess.run([native],text=True,capture_output=True)
    (OUT/'native.log').write_text(result.stdout+result.stderr)
    print(result.stdout+result.stderr,end='');result.check_returncode()
    if a.arm: build_arm(json.loads(a.container_command) if a.container_command else None)

if __name__=='__main__':main()
