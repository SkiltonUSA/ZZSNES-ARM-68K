#!/usr/bin/env python3
"""Build separate PocketSNES A9 diagnostic. Never deploys or launches."""
import subprocess,sys,json,hashlib,importlib.util
from pathlib import Path
import build_pocketsnes as b
ROOT=b.ROOT;OUT=b.OUT;SRC=b.SRC
import os
if 'POCKET_TOOLCHAIN_COMMAND' not in os.environ:
 raise SystemExit('Set POCKET_TOOLCHAIN_COMMAND to a JSON toolchain prefix; mount this checkout at /work (docs/build.md).')
cmd=json.loads(os.environ['POCKET_TOOLCHAIN_COMMAND'])
assert isinstance(cmd,list) and cmd and all(isinstance(x,str) and x for x in cmd)

def inside(p):return '/work/'+str(Path(p).relative_to(ROOT))
def run(args):subprocess.run([*cmd,*map(str,args)],check=True)
sources=b.prepare()
if not (OUT/'libc.a').exists():
 import shutil
 shutil.copyfile(ROOT/'artifacts/pocketsnes/build-support/libc.a',OUT/'libc.a')
common=ROOT/'amiga/mesen_bench'
for name in ['worker.cpp','mmu.c','mmu.h','mmu_asm.S','mmu_plan.h']:
 text=(common/name).read_text()
 if name=='mmu_plan.h':
  import re
  moves={'0x40000u':'0x100000u','0x50000u':'0x110000u','0x60000u':'0x120000u','0x80000u':'0x180000u','0x84000u':'0x184000u','0x82000u':'0x182000u','0x82400u':'0x182400u'}
  text=re.sub('|'.join(moves),lambda m:moves[m[0]],text)
 (OUT/name).write_text(text)
flags=[x.replace(str(ROOT),'/work') for x in b.flags()]+['-Os','-mcpu=cortex-a9','-marm','-mfloat-abi=soft','-mno-unaligned-access','-ffreestanding','-fno-builtin','-fPIC','-fvisibility=hidden','-ffunction-sections','-fdata-sections','-fno-stack-protector','-DPOCKET_ARM','-DRETRO_API=','-DZZ_BLOCK_SIZE=0x1800000','-DZZ_CONTROL=0x180000','-DMB_MMU','-DMB_DCACHE','-DMB_ALLOCATION=0x1800000u','-I/work/amiga/arm_debug','-I/work/amiga/arm_debug/zz9000','-include','/work/amiga/pocketsnes/libc_config.h']
sources += [SRC/'runtime.c',ROOT/'amiga/ffmpeg_bench/heap.c',ROOT/'amiga/mesen_snes/system/memory.c',OUT/'worker.cpp',OUT/'mmu.c',OUT/'mmu_asm.S']
header_inputs=sorted((OUT/'src').rglob('*.h'))+sorted((OUT/'libretro').rglob('*.h'))+sorted(common.glob('*.h'))+sorted((ROOT/'amiga/arm_debug/zz9000').glob('*.h'))+[OUT/'fixture.h',OUT/'mmu_plan.h',SRC/'libc_config.h',ROOT/'amiga/arm_debug/protocol.h']
header_digest=hashlib.sha256(b''.join(p.read_bytes() for p in header_inputs)).digest()
objects=[]
for i,src in enumerate(sources):
 obj=OUT/(str(i)+'-'+src.stem+'.arm.o');objects.append(obj)
 iscpp=src.suffix=='.cpp'
 extra=['-std=c++17','-fno-exceptions','-fno-rtti'] if iscpp else []
 if src.suffix=='.S':extra+=['-marm']
 digest=hashlib.sha256(src.read_bytes()+header_digest+str(flags+extra).encode()).hexdigest();stamp=obj.with_suffix('.sha256')
 if not stamp.exists() or stamp.read_text()!=digest:
   run(['arm-none-eabi-g++' if iscpp else 'arm-none-eabi-gcc',*flags,*extra,'-c',inside(src),'-o',inside(obj)])
   stamp.write_text(digest)
run(['arm-none-eabi-ld','-shared','-Bsymbolic','--gc-sections','--exclude-libs=ALL','--no-undefined','-T',inside(SRC/'payload.ld'),'-Map='+inside(OUT/'link.map'),*map(inside,objects),'--start-group',inside(OUT/'libc.a'),'/usr/lib/gcc/arm-none-eabi/15/thumb/v7-a/nofp/libgcc.a','--end-group','-o',inside(OUT/'pocket.elf')])
print('ARM linked')
