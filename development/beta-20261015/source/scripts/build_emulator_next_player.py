#!/usr/bin/env python3
"""Compile the existing cores plus the bounded shared presentation adapter."""
import argparse,hashlib,importlib.util,json,shutil,subprocess
from pathlib import Path
from amiga import PIN
from build_zz9000_debug import unpack_elf
from emulator_next_host import prepare
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'.context/amiga/emulator-next'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def run(a):subprocess.run(list(map(str,a)),cwd=ROOT,check=True)
def inside(p):return '/work/'+str(p.relative_to(ROOT))
def build(name):
 snes=name=='ZZSNES';base=ROOT/'.context/amiga/emulator-beta/build'/name
 original=ROOT/('.context/amiga/pocketsnes-regions/build' if snes else '.context/amiga/fceux-compat')
 stage=OUT/name/'player';stage.mkdir(parents=True,exist_ok=True)
 # Preserve source seeds independently from modified adapters.
 for p in base.iterdir():
  if p.suffix in ['.h','.c','.elf']:shutil.copy2(p,stage/p.name)
 prepare(stage,name)
 for n in ['direct.cpp','copy_neon.S','neon_state.S']:shutil.copy2(ROOT/'amiga/emulator'/n,stage/n)
 shutil.copy2(ROOT/'amiga/pocketsnes/library/row_copy_arm.h',stage/'row_copy_arm.h')
 meta=json.loads((original/'build.json').read_text());sources=[]
 for rel,digest in meta['sources'].items():
  p=ROOT/rel
  assert sha(p)==digest,(p,'baseline source changed')
  sources.append(stage/'direct.cpp' if p.name=='direct.cpp' else p)
 sources += [stage/'copy_neon.S',stage/'neon_state.S']
 block,control=('0x2000000','0x180000') if snes else ('0x400000','0x80000')
 if snes:
  flags=meta['arm_flags'];flags=['-I'+inside(stage)]+flags+['-DEM_SNES']
  command=['podman','--connection','nuflix-converter-root','exec','zzffmpeg-toolchain']
 else:
  flags=['--target=arm-none-eabi','-mcpu=cortex-a9','-marm','-mfloat-abi=soft','-mno-unaligned-access','-ffreestanding','-fno-builtin','-fPIC','-fvisibility=hidden','-fno-stack-protector','-O3','-g','-fwrapv','-DFC_ARM','-DDINGUX','-DFC_STATES','-DNES_PLAYER','-DNES_DIRECT','-DZZ_BLOCK_SIZE='+block,'-DZZ_CONTROL='+control,'-DMB_MMU','-DMB_DCACHE','-DMB_ALLOCATION='+block+'u','-Wno-deprecated-register','-Wno-writable-strings','-Wno-non-c-typedef-for-linkage']
  flags+=['-I'+str(p) for p in [stage,original,ROOT/'amiga/fceux_bench',ROOT/'amiga/mesen_nes',ROOT/'amiga/mesen_bench',ROOT/'amiga/arm_debug',ROOT/'amiga/arm_debug/zz9000']];command=[]
 objects=[];headers=b''.join(p.read_bytes() for p in sorted(stage.glob('*.h')) if p.name!='zz_payload.h')
 for i,p in enumerate(sources):
  obj=stage/f'{i}-{p.stem}.arm.o';objects.append(obj);cpp=p.suffix=='.cpp';stamp=obj.with_suffix('.sha256');digest=hashlib.sha256(p.read_bytes()+headers+str(flags).encode()).hexdigest()
  if not obj.exists()or not stamp.exists()or stamp.read_text()!=digest:
   run(command+(['arm-none-eabi-g++' if cpp else 'arm-none-eabi-gcc'] if snes else ['clang'])+flags+(['-std=c++14','-fno-exceptions','-fno-rtti']+([]if snes else ['-nostdinc++']) if cpp else [])+['-c',inside(p) if snes else p,'-o',inside(obj) if snes else obj]);stamp.write_text(digest)
 elf=stage/('pocket.elf' if snes else 'fceux.elf')
 if snes:run(command+['arm-none-eabi-ld','-shared','-Bsymbolic','--gc-sections','--exclude-libs=ALL','--no-undefined','-T',inside(original/'payload.ld'),*map(inside,objects),'--start-group',inside(original/'libc.a'),'/usr/lib/gcc/arm-none-eabi/15/thumb/v7-a/nofp/libgcc.a','--end-group','-o',inside(elf)])
 else:run(['/opt/homebrew/opt/lld/bin/ld.lld','-shared','-Bsymbolic','--no-undefined','-T',ROOT/'amiga/fceux_bench/payload.ld',*objects,'-o',elf])
 spec=importlib.util.spec_from_file_location('pack',original/'elf.py' if snes else ROOT/'amiga/fceux_bench/elf.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
 image,rel,entry=m.unpack(elf.read_bytes());mapping,mrel,point=unpack_elf((stage/'mapping.elf').read_bytes());assert not mrel and not point
 array=lambda n,d:'static const unsigned char '+n+'[]={'+','.join(map(str,d))+'};\n'
 (stage/'zz_payload.h').write_text(f'#define ZZ_ENTRY {entry}u\n'+array('zz_image',image)+array('zz_mapping_probe',mapping)+'static const unsigned long zz_relocations[]={'+','.join(map(str,rel))+'};\n')
 version='0.5.0 Beta'if snes else '0.6.0 Beta';exe='ZZSnes-ui'if snes else 'ZZFceux-ui';relative=stage.relative_to(ROOT)
 defs=['ZZ_HEADLESS_BENCH','ZZ_RELEASE','ZZ_NES','ZZ_BLOCK_SIZE='+block,'ZZ_CONTROL='+control,'ZZ_APP_NAME="'+name+'"','ZZ_APP_VERSION="'+version+'"','ZZ_MIN_STACK=65536','MB_ALLOCATION='+block+'u','MB_MMU','MB_DCACHE','ZZ_HEADLESS_MMU','ZZ_HEADLESS_ICACHE','NES_PLAYER','NES_DIRECT','NES_FULLSCREEN']
 run(['podman','--connection','nuflix-converter-root','run','--rm','--platform','linux/amd64','-v',str(ROOT)+':/work','-w','/work',PIN['image'],'m68k-amigaos-gcc','-std=c99','-noixemul','-m68020','-O2','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-misleading-indentation',*['-D'+d for d in defs],'-I'+str(relative),'-Iamiga/mesen_nes','-Iamiga/mesen_bench','-Iamiga/arm_debug','-Iamiga/arm_debug/zz9000','amiga/arm_debug/zz9000/launcher.c',relative/'host.c','-lamiga','-o',relative/exe])
 (stage/'next-build.json').write_text(json.dumps(dict(console=name,version=version,image_bytes=len(image),relocations=len(rel),sources={str(p.relative_to(ROOT)):sha(p) for p in sources},files={exe:sha(stage/exe),elf.name:sha(elf)}),indent=2)+'\n')
 print('BUILT',name,version,len(image),flush=True)
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--console',choices=['ZZNES','ZZSNES'],action='append');a=p.parse_args()
 for n in a.console or ['ZZNES','ZZSNES']:build(n)
