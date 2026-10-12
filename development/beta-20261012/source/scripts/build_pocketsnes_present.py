#!/usr/bin/env python3
"""Isolated ARM presenter candidate; reuse verified core objects and paced host.
Never installs or launches. The original core, allocation and save ABI stay put.
"""
from pathlib import Path
import hashlib,json,shutil,subprocess,importlib.util,argparse
from amiga import PIN
from build_zz9000_debug import unpack_elf
ROOT=Path(__file__).resolve().parents[1]
OLD=ROOT/'.context/amiga/pocketsnes-library'
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--cadence',choices=['baseline','paced'],default='paced');parser.add_argument('--stores',choices=['scalar','burst','stmia'],default='scalar');args=parser.parse_args()
BASE=ROOT/('.context/amiga/pocketsnes-schedule/paced' if args.cadence=='paced' else '.context/amiga/game-snapshots/snes')
OUT=ROOT/'.context/amiga/pocketsnes-schedule'/(('lut' if args.cadence=='paced' else 'lut-baseline')+('-'+args.stores if args.stores!='scalar' else ''));OUT.mkdir(parents=True,exist_ok=True)
expected=json.loads((OLD/'library-build.json').read_text())['files']['pocket.elf']['sha256']
assert hashlib.sha256((OLD/'pocket.elf').read_bytes()).hexdigest()==expected
for p in BASE.iterdir():
 if p.is_file() and p.suffix in ('.h','.c','.elf'):shutil.copy2(p,OUT/p.name)
for name in ['elf.py','payload.ld','libc.a','view-native']:
 shutil.copy2(OLD/name,OUT/name)
shutil.copy2(ROOT/'amiga/pocketsnes/library/view_fast.h',OUT/'view_fast.h')
(OUT/'direct.cpp').write_text((OLD/'direct.cpp').read_text().replace('#include "view.h"','#include "view_fast.h"'))
if args.stores=='burst':
 p=OUT/'direct.cpp';s=p.read_text();s=s.replace('  for(unsigned x=0;x<320;x++)row[x]=row_words[x];','  /* Private completed bitmap: grouped copies, DSB before revocation/ACK below. */\n  for(unsigned x=0;x<320;x+=8)__builtin_memcpy((void*)(row+x),row_words+x,32);');p.write_text(s)
if args.stores=='stmia':
 shutil.copy2(ROOT/'amiga/pocketsnes/library/row_copy_arm.h',OUT/'row_copy_arm.h')
 p=OUT/'direct.cpp';s=p.read_text().replace('#include \"nes_api.h\"','#include \"nes_api.h\"\n#include \"row_copy_arm.h\"').replace('  for(unsigned x=0;x<320;x++)row[x]=row_words[x];','  ps_copy_row(row,row_words);');p.write_text(s)
inside=lambda p:'/work/'+str(p.relative_to(ROOT))
cmd=['podman','--connection','nuflix-converter-root','exec','zzffmpeg-toolchain']
flags=['-Os','-g','-fno-strict-aliasing','-mcpu=cortex-a9','-marm','-mfloat-abi=soft','-mno-unaligned-access','-ffreestanding','-fno-builtin','-fPIC','-fvisibility=hidden','-ffunction-sections','-fdata-sections','-fno-stack-protector','-std=c++17','-fno-exceptions','-fno-rtti','-DNES_PLAYER','-DNES_DIRECT','-DPOCKET_ARM','-DZZ_BLOCK_SIZE=0x2000000','-DZZ_CONTROL=0x180000','-DMB_MMU','-DMB_DCACHE','-DMB_ALLOCATION=0x2000000u',
 '-I'+inside(OUT),'-I/work/amiga/mesen_nes','-I/work/amiga/mesen_bench','-I/work/amiga/arm_debug','-I/work/amiga/arm_debug/zz9000','-include','/work/amiga/pocketsnes/libc_config.h']
subprocess.run([*cmd,'arm-none-eabi-g++',*flags,'-c',inside(OUT/'direct.cpp'),'-o',inside(OUT/'31-direct.arm.o')],check=True)
objects=sorted(OLD.glob('*.arm.o'),key=lambda p:int(p.name.split('-')[0]));assert len(objects)==38
objects=[OUT/p.name if p.name=='31-direct.arm.o' else p for p in objects]
subprocess.run([*cmd,'arm-none-eabi-ld','-shared','-Bsymbolic','--gc-sections','--exclude-libs=ALL','--no-undefined','-T',inside(OUT/'payload.ld'),'-Map='+inside(OUT/'link.map'),*map(inside,objects),'--start-group',inside(OUT/'libc.a'),'/usr/lib/gcc/arm-none-eabi/15/thumb/v7-a/nofp/libgcc.a','--end-group','-o',inside(OUT/'pocket.elf')],check=True)
spec=importlib.util.spec_from_file_location('pack',OUT/'elf.py');pack=importlib.util.module_from_spec(spec);spec.loader.exec_module(pack)
image,rel,entry=pack.unpack((OUT/'pocket.elf').read_bytes())
mapping,mrel,point=unpack_elf((OUT/'mapping.elf').read_bytes());assert not mrel and not point and len(mapping)<=1024
array=lambda n,d:'static const unsigned char '+n+'[]={'+','.join(map(str,d))+'};\n'
(OUT/'zz_payload.h').write_text(f'#define ZZ_ENTRY {entry}u\n'+array('zz_image',image)+array('zz_mapping_probe',mapping)+'static const unsigned long zz_relocations[]={'+','.join(map(str,rel))+'};\n')
p=OUT/'host.c';p.write_text(p.read_text().replace('PLAYER SCHEDULING paced','PLAYER SCHEDULING paced + ARM LUT').replace('PLAYER PocketSNES','PLAYER SCHEDULING baseline + ARM LUT PocketSNES'))
relative=OUT.relative_to(ROOT)
defines=['-DZZ_HEADLESS_BENCH','-DZZ_RELEASE','-DZZ_NES','-DZZ_BLOCK_SIZE=0x2000000','-DZZ_CONTROL=0x180000','-DZZ_APP_NAME="ZZSnes-Pocket"','-DZZ_APP_VERSION="fps-lut-preview"','-DZZ_MIN_STACK=65536','-DMB_ALLOCATION=0x2000000u','-DMB_MMU','-DMB_DCACHE','-DZZ_HEADLESS_MMU','-DZZ_HEADLESS_ICACHE','-DNES_PLAYER','-DNES_DIRECT','-DNES_FULLSCREEN']
subprocess.run(['podman','--connection','nuflix-converter-root','run','--rm','--platform','linux/amd64','-v',f'{ROOT}:/work','-w','/work',PIN['image'],'m68k-amigaos-gcc','-std=c99','-noixemul','-m68020','-O2','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-misleading-indentation',*defines,'-I'+str(relative),'-Iamiga/mesen_nes','-Iamiga/mesen_bench','-Iamiga/arm_debug','-Iamiga/arm_debug/zz9000','amiga/arm_debug/zz9000/launcher.c',str(relative/'host.c'),'-lamiga','-o',str(relative/'ZZSnes-Pocket')],check=True)
record={'scope':'Only direct-presenter ARM object replaced; '+args.cadence+' host; no core/save/MMU policy change','base_elf_sha256':expected,'stores':args.stores,'arm_flags':flags,'objects':{str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in objects},'files':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in [OUT/n for n in ['ZZSnes-Pocket','pocket.elf','direct.cpp','view_fast.h','host.c','pace.h','player_audio.h','link.map']]}}
record['source_sha256']={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [ROOT/'amiga/pocketsnes/library/view_fast.h',ROOT/'amiga/pocketsnes/library/row_copy_arm.h',Path(__file__).resolve()]}
(OUT/'present-build.json').write_text(json.dumps(record,indent=2)+'\n');print('Built',OUT/'ZZSnes-Pocket',record['files']['ZZSnes-Pocket'])
