#!/usr/bin/env python3
"""Pack bounded PocketSNES ELF and build its independent 68k diagnostic."""
import sys,subprocess,importlib.util,json,hashlib
from pathlib import Path
import build_pocketsnes as b
from build_zz9000_debug import unpack_elf
from amiga import PIN, container_command
root=b.ROOT;out=b.OUT;src=b.SRC;common=root/'amiga/mesen_bench'
for required in ['mmu.h','mmu_plan.h','libc.a']:
 if not (out/required).is_file():raise SystemExit('Build the ARM component first: missing '+required)
spec=importlib.util.spec_from_file_location('pack',src/'elf.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
elf=out/'pocket.elf';image,rel,entry=m.unpack(elf.read_bytes());print('Image',len(image),'relocations',len(rel))
subprocess.run(['clang','--target=arm-none-eabi','-mcpu=cortex-a9','-marm','-fPIC','-Iamiga/arm_debug/zz9000','-DZZ_CONTROL=0x180000','-c','amiga/arm_debug/zz9000/mapping_probe.S','-o',out/'mapping.o'],cwd=root,check=True)
subprocess.run(['/opt/homebrew/opt/lld/bin/ld.lld','-shared','-Bsymbolic','--no-undefined','-T',root/'amiga/arm_debug/zz9000/payload.ld',out/'mapping.o','-o',out/'mapping.elf'],check=True)
mapping,mrel,point=unpack_elf((out/'mapping.elf').read_bytes());assert not mrel and not point and len(mapping)<=1024
def array(n,data):return 'static const unsigned char '+n+'[]={'+','.join(map(str,data))+'};\n'
(out/'zz_payload.h').write_text(f'#define ZZ_ENTRY {entry}u\n'+array('zz_image',image)+array('zz_mapping_probe',mapping)+'static const unsigned long zz_relocations[]={'+','.join(map(str,rel))+'};\n')
(out/'expected.h').write_text('#define MESEN_EXPECT_CYCLES 6u\n#define MESEN_EXPECT_HASH 0x67711dc5u\n')
host=(common/'host.c').read_text().replace('8991','6').replace('instructions=6','frames=6').replace('cpu_us=','core_us=')
host=host.replace('MESEN CPU-ONLY','POCKETSNES CPU/PPU/DMA/SPC/DSP; portable C; no frameskip/speed hacks')
host=host.replace('MESEN_CPU PASS; CPU-only throughput, NOT emulated video FPS','POCKETSNES PASS; original headless fixture, NOT game/display FPS')
pos=host.index('    for(i=0;i<11;i++)')
host=host[:pos]+'    printf("SYSTEM width=%u height=%u pcm=%08x samples=%u\\n",value(7),value(8),value(9),value(10));\n    if(value(7)!=256 || value(8)!=224 || value(9)!=0x77d82d05 || value(10)!=3208)goto done;\n'+host[pos:]
host=host.replace('if(value(0) || value(1)', 'if(value(7)!=256 || value(8)!=224 || value(9)!=0x77d82d05 || value(10)!=3208 || value(0) || value(1)')
(out/'host.c').write_text(host)
appname='ZZSnes-Pocket-Bench';relative=out.relative_to(root)
import os
container=container_command() if 'AMIGA_CONTAINER_COMMAND' in os.environ else ['podman','--connection','nuflix-converter-root']
subprocess.run([*container,'run','--rm','--platform','linux/amd64','-v',f'{root}:/work','-w','/work',PIN['image'],'m68k-amigaos-gcc','-std=c99','-noixemul','-m68020','-O2','-g','-Wall','-Wextra','-Werror','-DZZ_HEADLESS_BENCH','-DZZ_RELEASE','-DZZ_BLOCK_SIZE=0x1800000','-DZZ_CONTROL=0x180000','-DMB_ALLOCATION=0x1800000u','-DZZ_APP_NAME="'+appname+'"','-DZZ_APP_VERSION="0.1.0-dev"','-DZZ_MIN_STACK=65536','-DMB_MMU','-DMB_DCACHE','-DZZ_HEADLESS_MMU','-DZZ_HEADLESS_ICACHE','-I'+str(relative),'-Iamiga/arm_debug','-Iamiga/arm_debug/zz9000','-Iamiga/mesen_bench','amiga/arm_debug/zz9000/launcher.c',str(relative/'host.c'),'-lamiga','-o',str(relative/appname)],cwd=root,check=True)
binary=out/appname;assert binary.read_bytes()[:4]==b'\0\0\x03\xf3'
record={'upstream':b.PIN,'url':'https://github.com/libretro/snes9x2002','scope':'Original six-frame headless fixture; portable C CPU/PPU/SPC/DSP; no frameskip, idle shutdown or game speed patches. No display/audio-device FPS.','allocation':0x1800000,'image_bytes':len(image),'relocations':len(rel),'source_hashes':{str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [*src.glob('*.*'),*root.glob('scripts/build_pocketsnes*.py')] if p.is_file()},'outputs':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in [elf,binary,out/'System-Test.sfc',out/'integration.patch',out/'libc.a']},'executable_md5':hashlib.md5(binary.read_bytes()).hexdigest()}
(out/'build.json').write_text(json.dumps(record,indent=2)+'\n');print(appname,binary.stat().st_size,record['executable_md5'])
