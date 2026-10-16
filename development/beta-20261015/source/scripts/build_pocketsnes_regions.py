#!/usr/bin/env python3
"""Build PAL/Contra support on the accepted border/LUT/STMIA frontend.
Requires the existing verified frontend cache. Never deploys or launches.
"""
from pathlib import Path
import hashlib,importlib.util,json,shutil,subprocess
import build_pocketsnes as core
from pocketsnes_library import core_patch,LIB
from pocketsnes_regions import frontend
from amiga import PIN
from build_game_libraries import IMAGE as UI_IMAGE
from build_zz9000_debug import unpack_elf
ROOT=core.ROOT
OUT=ROOT/'.context/amiga/pocketsnes-regions/build'
BASE=ROOT/'.context/amiga/game-borders/snes'
OLD=ROOT/'.context/amiga/pocketsnes-library'
def inside(p):return '/work/'+str(p.relative_to(ROOT))
def run(a):subprocess.run(list(map(str,a)),cwd=ROOT,check=True)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 OUT.mkdir(parents=True,exist_ok=True)
 for p in BASE.iterdir():
  if p.is_file() and p.suffix in ('.h','.c','.cpp','.elf'):shutil.copy2(p,OUT/p.name)
 for name in ['worker.cpp','payload.ld','elf.py','libc.a','mmu.c','mmu_asm.S']:shutil.copy2(OLD/name,OUT/name)
 for name in ['cartridge.h','view.h','view_fast.h','state_format.h','player.c','region.h']:shutil.copy2(LIB/name,OUT/name)
 for name in ['ui_model.h','library_mui.h','library_preview.h','library_saves.h','library_controller.h','snapshot_format.h']:
  shutil.copy2(ROOT/'amiga/fceux_player'/name,OUT/name)
 f=OUT/'ui_model.h';f.write_text(f.read_text().replace('"ZZNESUI"','"ZZSNESUI"').replace('m->mask=1;','m->theme=2;'))
 frontend(OUT)
 core.OUT=OUT;sources=core.prepare()
 f=OUT/'src/soundux.c';s=f.read_text().replace('sample1 << 4','sample1 * 16')
 for name in ['sample1','sample2']:
  s=s.replace(f'(int32) {name} << shift',f'(int32) {name} * (1 << shift)').replace(f'{name} << shift',f'{name} * (1 << shift)')
 f.write_text(s.replace('prev0 << 1','prev0 * 2'));core_patch(OUT)
 sources=[OUT/'player.c' if s.name=='bench.c' else s for s in sources]
 sources += [OUT/'direct.cpp',core.SRC/'runtime.c',ROOT/'amiga/ffmpeg_bench/heap.c',ROOT/'amiga/mesen_snes/system/memory.c',OUT/'worker.cpp',OUT/'mmu.c',OUT/'mmu_asm.S']
 meta=json.loads((ROOT/'.context/amiga/pocketsnes-schedule/lut-stmia/present-build.json').read_text())
 flags=[v.replace(inside(ROOT/'.context/amiga/pocketsnes-schedule/lut-stmia'),inside(OUT)) for v in meta['arm_flags'] if v not in ['-std=c++17','-fno-exceptions','-fno-rtti']]
 flags += [v.replace(str(ROOT),'/work') for v in core.flags() if v not in ['-O2','-g','-fno-strict-aliasing']]+['-DRETRO_API=']
 arm_flags=flags[:]
 command=['podman','--connection','nuflix-converter-root','exec','zzffmpeg-toolchain'];objects=[]
 headers=hashlib.sha256(b''.join(p.read_bytes() for p in sorted(OUT.rglob('*.h')) if p.name!='zz_payload.h')).digest()
 for i,s in enumerate(sources):
  obj=OUT/f'{i}-{s.stem}.arm.o';objects.append(obj);cpp=s.suffix=='.cpp'
  stamp=obj.with_suffix('.sha256');digest=hashlib.sha256(s.read_bytes()+headers+str(flags).encode()).hexdigest()
  if obj.exists() and stamp.exists() and stamp.read_text()==digest:continue
  run([*command,'arm-none-eabi-g++' if cpp else 'arm-none-eabi-gcc',*flags,*(['-std=c++17','-fno-exceptions','-fno-rtti'] if cpp else []),'-c',inside(s),'-o',inside(obj)])
  stamp.write_text(digest)
 run([*command,'arm-none-eabi-ld','-shared','-Bsymbolic','--gc-sections','--exclude-libs=ALL','--no-undefined','-T',inside(OUT/'payload.ld'),'-Map='+inside(OUT/'link.map'),*map(inside,objects),'--start-group',inside(OUT/'libc.a'),'/usr/lib/gcc/arm-none-eabi/15/thumb/v7-a/nofp/libgcc.a','--end-group','-o',inside(OUT/'pocket.elf')])
 spec=importlib.util.spec_from_file_location('pack',OUT/'elf.py');pack=importlib.util.module_from_spec(spec);spec.loader.exec_module(pack)
 image,rel,entry=pack.unpack((OUT/'pocket.elf').read_bytes())
 mapping,mrel,point=unpack_elf((OUT/'mapping.elf').read_bytes());assert not mrel and not point and len(mapping)<=1024
 array=lambda n,d:'static const unsigned char '+n+'[]={'+','.join(map(str,d))+'};\n'
 (OUT/'zz_payload.h').write_text(f'#define ZZ_ENTRY {entry}u\n'+array('zz_image',image)+array('zz_mapping_probe',mapping)+'static const unsigned long zz_relocations[]={'+','.join(map(str,rel))+'};\n')
 command=['podman','--connection','nuflix-converter-root','run','--rm','--platform','linux/amd64','-v',f'{ROOT}:/work','-w','/work',PIN['image'],'m68k-amigaos-gcc','-std=c99','-noixemul','-m68020','-O2','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-misleading-indentation']
 flags=['-DZZ_HEADLESS_BENCH','-DZZ_RELEASE','-DZZ_NES','-DZZ_BLOCK_SIZE=0x2000000','-DZZ_CONTROL=0x180000','-DZZ_APP_NAME="ZZSnes-ui"','-DZZ_APP_VERSION="regions-preview"','-DZZ_MIN_STACK=65536','-DMB_ALLOCATION=0x2000000u','-DMB_MMU','-DMB_DCACHE','-DZZ_HEADLESS_MMU','-DZZ_HEADLESS_ICACHE','-DNES_PLAYER','-DNES_DIRECT','-DNES_FULLSCREEN']
 relative=OUT.relative_to(ROOT)
 run([*command,*flags,'-I'+str(relative),'-Iamiga/mesen_nes','-Iamiga/mesen_bench','-Iamiga/arm_debug','-Iamiga/arm_debug/zz9000','amiga/arm_debug/zz9000/launcher.c',relative/'host.c','-lamiga','-o',relative/'ZZSnes-ui'])
 ui_command=[UI_IMAGE if v==PIN['image'] else v for v in command]
 run([*ui_command,'-DLIBRARY_PREVIEW',relative/'launcher.c','-lamiga','-o',relative/'ZZSnesLibrary'])
 record={'scope':'Automatic PAL/NTSC cadence, 224/239 geometry, bounded Contra III header compatibility; original 32 MiB allocation and LUT/STMIA presenter','hardware_tested':False,'upstream':core.PIN,'image_bytes':len(image),'relocations':len(rel),'arm_flags':arm_flags,'outputs':{n:{'bytes':(OUT/n).stat().st_size,'md5':hashlib.md5((OUT/n).read_bytes()).hexdigest(),'sha256':sha(OUT/n)} for n in ['ZZSnes-ui','ZZSnesLibrary','pocket.elf']},'sources':{str(s.relative_to(ROOT)):sha(s) for s in sources}}
 (OUT/'build.json').write_text(json.dumps(record,indent=2)+'\n');print(json.dumps(record['outputs'],indent=2))
if __name__=='__main__':main()
