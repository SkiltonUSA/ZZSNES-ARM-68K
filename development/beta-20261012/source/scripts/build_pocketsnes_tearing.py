#!/usr/bin/env python3
"""Isolated 2x2 vertical geometry / buffer retirement comparison.

Requires the verified PAL-aware region build. Does not deploy or launch.
A=scaled/3 slots, B=native/3, C=scaled/4, D=native/4.
No overlap, clock, audio, core, firmware or memory-ownership changes.
"""
import hashlib, importlib.util, json, shutil, subprocess
from pathlib import Path
from amiga import PIN
from build_zz9000_debug import unpack_elf

ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'.context/amiga/pocketsnes-regions/build'
OUT=ROOT/'.context/amiga/pocketsnes-tearing'
ARM=['podman','--connection','nuflix-converter-root','exec','zzffmpeg-toolchain']
def run(args):subprocess.run(list(map(str,args)),cwd=ROOT,check=True)
def inside(path):return '/work/'+str(path.relative_to(ROOT))
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def replace(text,old,new):
 assert text.count(old)==1,(old,text.count(old))
 return text.replace(old,new)
def main():
 meta=json.loads((BASE/'build.json').read_text())
 assert sha(BASE/'ZZSnes-ui')=='682db690a702d9770057e7f56b85bd4fdec332a5b5dfbea76cab0be082d4fccc'
 assert sha(BASE/'pocket.elf')==meta['outputs']['pocket.elf']['sha256']
 objects=sorted(BASE.glob('*.arm.o'),key=lambda p:int(p.name.split('-')[0]));assert len(objects)==38
 spec=importlib.util.spec_from_file_location('pack',BASE/'elf.py');pack=importlib.util.module_from_spec(spec);spec.loader.exec_module(pack)
 original=pack.unpack((BASE/'pocket.elf').read_bytes())
 records={}
 for name,native,slots in [('A',False,3),('B',True,3),('C',False,4),('D',True,4)]:
  stage=OUT/name;stage.mkdir(parents=True,exist_ok=True)
  for p in BASE.iterdir():
   if p.is_file() and p.suffix in ('.h','.c','.cpp','.elf','.py','.ld'):shutil.copy2(p,stage/p.name)
  shutil.copy2(ROOT/'amiga/pocketsnes/library/native_height.h',stage/'native_height.h')
  shutil.copy2(ROOT/('amiga/mesen_nes/flip_plan.h' if slots==3 else 'amiga/pocketsnes/library/flip_retire.h'),stage/'flip_plan.h')
  if native:
   p=stage/'view.h';s=p.read_text();start=s.index('static inline int fv_y(');end=s.index('\n',start)
   s=s[:start]+'#include "native_height.h"\nstatic inline int fv_y(unsigned y,unsigned opt){return ps_native_y(y,opt);}'+s[end:];p.write_text(s)
  # All variants use the same black, unmasked CLI default for comparison.
  p=stage/'session.h';p.write_text(replace(p.read_text(),'fc_view=4,','fc_view=0,'))
  full=(stage/'fullscreen.h').read_text();host=(stage/'host.c').read_text()
  if slots==4:
   full=replace(full,'full_slots={0,1,-1,0,0}','full_slots={0,1,-1,0,0,3}')
   full=full.replace('720','960').replace('slots=3','slots=4')
   host=host.replace('720*pitch','960*pitch').replace('complete 3-slot','complete 4-slot')
  # Observation only: these timestamps are CPU acknowledgement observations,
  # not proof of the hardware DMA latch or monitor refresh frequency.
  full=replace(full,'static void full_write(','''static ULONG trial_ack_blank,trial_ack_active;
static void full_write(''')
  full=replace(full,'if(status==1){fc_flip_sample();','if(status==1){if((*(volatile UWORD *)(full_board+0x4c)&0x20u)!=0)trial_ack_blank++;else trial_ack_active++;fc_flip_sample();')
  host=replace(host,'fc_profile_print();fc_joy_close();',f'printf("TEARING_TRIAL variant={name} native={int(native)} slots={slots} ack_blank=%u ack_active=%u\\n",trial_ack_blank,trial_ack_active);\n fc_profile_print();fc_joy_close();')
  (stage/'fullscreen.h').write_text(full);(stage/'host.c').write_text(host)
  candidate=objects[:]
  # Rebuild presenter for both geometries; A must reproduce the original
  # loaded image exactly before permitting reuse of cached core objects.
  if name in ('A','B'):
   flags=[v.replace(inside(BASE),inside(stage)) for v in meta['arm_flags']]
   obj=stage/'31-direct.arm.o';candidate[31]=obj
   run([*ARM,'arm-none-eabi-g++',*flags,'-std=c++17','-fno-exceptions','-fno-rtti','-c',inside(stage/'direct.cpp'),'-o',inside(obj)])
   run([*ARM,'arm-none-eabi-ld','-shared','-Bsymbolic','--gc-sections','--exclude-libs=ALL','--no-undefined','-T',inside(stage/'payload.ld'),'-Map='+inside(stage/'link.map'),*map(inside,candidate),'--start-group',inside(BASE/'libc.a'),'/usr/lib/gcc/arm-none-eabi/15/thumb/v7-a/nofp/libgcc.a','--end-group','-o',inside(stage/'pocket.elf')])
   if name=='A':assert pack.unpack((stage/'pocket.elf').read_bytes())==original,'Control does not reproduce baseline payload'
  else:shutil.copy2(OUT/('B' if native else 'A')/'pocket.elf',stage/'pocket.elf')
  image,rel,entry=pack.unpack((stage/'pocket.elf').read_bytes())
  mapping,mrel,point=unpack_elf((stage/'mapping.elf').read_bytes());assert not mrel and not point and len(mapping)<=1024
  array=lambda n,d:'static const unsigned char '+n+'[]={'+','.join(map(str,d))+'};\n'
  (stage/'zz_payload.h').write_text(f'#define ZZ_ENTRY {entry}u\n'+array('zz_image',image)+array('zz_mapping_probe',mapping)+'static const unsigned long zz_relocations[]={'+','.join(map(str,rel))+'};\n')
  relative=stage.relative_to(ROOT)
  defines=['ZZ_HEADLESS_BENCH','ZZ_RELEASE','ZZ_NES','ZZ_BLOCK_SIZE=0x2000000','ZZ_CONTROL=0x180000',f'ZZ_APP_NAME="ZZSnes-{name}"','ZZ_APP_VERSION="tearing-trial"','ZZ_MIN_STACK=65536','MB_ALLOCATION=0x2000000u','MB_MMU','MB_DCACHE','ZZ_HEADLESS_MMU','ZZ_HEADLESS_ICACHE','NES_PLAYER','NES_DIRECT','NES_FULLSCREEN']
  run(['podman','--connection','nuflix-converter-root','run','--rm','--platform','linux/amd64','-v',f'{ROOT}:/work','-w','/work',PIN['image'],'m68k-amigaos-gcc','-std=c99','-noixemul','-m68020','-O2','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-misleading-indentation',*['-D'+v for v in defines],'-I'+str(relative),'-Iamiga/mesen_nes','-Iamiga/mesen_bench','-Iamiga/arm_debug','-Iamiga/arm_debug/zz9000','amiga/arm_debug/zz9000/launcher.c',relative/'host.c','-lamiga','-o',relative/f'ZZSnes-{name}'])
  run(['clang','-O2','-I'+str(stage),ROOT/'tests/amiga/pocketsnes/view_stream.c','-o',stage/'view-native'])
  files=['host.c','fullscreen.h','flip_plan.h','view.h','native_height.h','session.h','zz_payload.h','pocket.elf',f'ZZSnes-{name}']
  record={'variant':name,'native_height':native,'slots':slots,'hardware_tested':False,'sha256':{n:sha(stage/n) for n in files},'md5':hashlib.md5((stage/f'ZZSnes-{name}').read_bytes()).hexdigest()}
  (stage/'trial-build.json').write_text(json.dumps(record,indent=2)+'\n');records[name]=record
  print('Built variant',name,flush=True)
 (OUT/'builds.json').write_text(json.dumps(records,indent=2)+'\n')
if __name__=='__main__':main()
