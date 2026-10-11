#!/usr/bin/env python3
"""Prepare pinned FCEUX core; no implicit hardware launch."""
from pathlib import Path
import subprocess,re,json,hashlib,difflib,sys,argparse
ROOT=Path(__file__).resolve().parents[1]
SRC=ROOT/'amiga/fceux_bench';OUT=ROOT/'.context/amiga/fceux-bench'
UP=SRC/'vendor';COMMIT='4d01530b6689b06a6d7cf0679b63625e3270ae9a'
FILES=['pputile.inc','ops.inc','x6502.cpp','x6502.h','x6502struct.h','x6502abbrev.h','ppu.cpp','ppu.h','sound.cpp','sound.h','filter.cpp','filter.h','fcoeffs.h','cart.cpp','cart.h','fceu.h','git.h','boards/90.cpp']
FILES += ['fir/'+p.name for p in sorted((UP/'fir').glob('*.h'))]
def remove_function(s,name):
 m=re.search(r'^[^\n{};]*\b'+name+r'\([^\n]*\)\s*\{',s,re.M);assert m,name
 i=m.end();n=1
 while n:
  n+=(s[i]=='{')-(s[i]=='}');i+=1
 return s[:m.start()]+s[i:]
def prepare():
 pinned=json.loads((UP/'upstream.json').read_text());assert pinned['commit']==COMMIT
 OUT.mkdir(parents=True,exist_ok=True);manifest={};patch=''
 for name in FILES:
  p=UP/name;raw=p.read_bytes();assert hashlib.sha256(raw).hexdigest()==pinned['files'][name];s=raw.decode().replace('\r\n','\n');orig=s
  manifest[name]=hashlib.sha256(raw).hexdigest()
  if name.endswith('.cpp'):
   s=re.sub(r'^#include .*\n','',s,flags=re.M)
   s='#include "platform.h"\n'+s
   if name=='x6502.cpp':s='#include "platform.h"\n#include "x6502abbrev.h"\n'+s.removeprefix('#include "platform.h"\n')
   if name=='filter.cpp':s=s.replace('#include "platform.h"','#include "platform.h"\n#include "fcoeffs.h"')
   if name=='cart.cpp':
    for f in ['FCEU_OpenGenie','FCEU_SaveGameSave','FCEU_LoadGameSave']:
     s=remove_function(s,f)
  if name in ('ppu.cpp','pputile.inc'):
   s=re.sub(r'uint32\s*\*', 'FC_Unaligned32 *', s)
  if name=='ppu.cpp':
   s=s.replace('T* lut;', 'T lut[1 << BITS];').replace('BITREVLUT() {','void init() {').replace('lut = new T[n];','// Storage belongs to the launcher allocation.')
   s=s.replace('void FCEUPPU_Init(void) {','void FCEUPPU_Init(void) {\n bitrevlut.init();')
  if name=='sound.cpp':
   l1=[0]+[int(16384*95.52/(8128/x+100)) for x in range(1,32)]
   l2=[0]+[int(16384*163.67/(24329/x+100)) for x in range(1,203)]
   s='#ifdef FC_ARM\nstatic const unsigned fc_l1[]={'+','.join(map(str,l1))+'};\nstatic const unsigned fc_l2[]={'+','.join(map(str,l2))+'};\n#endif\n'+s
   replacements={
    'wlookup1[x]=(double)16*16*16*4*95.52/((double)8128/(double)x+100);':'wlookup1[x]=fc_l1[x];',
    'wlookup2[x]=(double)16*16*16*4*163.67/((double)24329/(double)x+100);':'wlookup2[x]=fc_l2[x];',
    'nesincsize=(int64)(((int64)1<<17)*(double)(PAL?PAL_CPU:NTSC_CPU)/(FSettings.SndRate * 16));':f'nesincsize={int((1<<17)*1789772.7272727272727272/(32000*16))};',
    'soundtsinc=(uint32)((uint64)(PAL?(long double)PAL_CPU*65536:(long double)NTSC_CPU*65536)/(FSettings.SndRate * 16));':f'soundtsinc={int(1789772.7272727272727272*65536)//(32000*16)};'}
   for old,new in replacements.items():
    assert old in s;s=s.replace(old,'\n#ifdef FC_ARM\n '+new+'\n#else\n '+old+'\n#endif\n')
  if name=='filter.cpp':
   old='mrratio=(PAL?(int64)(PAL_CPU*65536):(int64)(NTSC_CPU*65536))/rate;'
   assert old in s;s=s.replace(old,'\n#ifdef FC_ARM\n mrratio='+str(int(1789772.7272727272727272*65536)//32000)+';\n#else\n '+old+'\n#endif\n')
  if name=='git.h':
   s=s.replace('~FCEUGI();','').replace('FCEUGI();','')
   s=s.replace('#include "utils/md5.h"','struct MD5DATA { uint8 bytes[16]; };')
  dest=OUT/(name if name.startswith('fir/') else Path(name).name)
  dest.parent.mkdir(parents=True,exist_ok=True);dest.write_text(s)
  patch+=''.join(difflib.unified_diff(orig.splitlines(True),s.splitlines(True),'upstream/'+name,'generated/'+name))
 (OUT/'types.h').write_text('''#pragma once
#include <stdint.h>
typedef uint8_t uint8;typedef uint16_t uint16;typedef uint32_t uint32;typedef uint64_t uint64;typedef uint64_t u64;
typedef int8_t int8;typedef int16_t int16;typedef int32_t int32;typedef int64_t int64;
#define INLINE inline
#define GINLINE inline
#define FASTCALL
#define DEBUG(X)
#define LSB_FIRST 1
#define PSS_STYLE 1
typedef uint8 (*readfunc)(uint32);typedef void (*writefunc)(uint32,uint8);
''')
 (OUT/'integration.patch').write_text(patch)
 (OUT/'upstream.json').write_text(json.dumps({'repo':'https://github.com/Rezzy-dev/fceux','commit':COMMIT,'files':manifest},indent=2)+'\n')
 return [OUT/x for x in ['x6502.cpp','ppu.cpp','sound.cpp','filter.cpp','cart.cpp','90.cpp']]

def build_native(sources):
 common=['-std=c++14','-DDINGUX','-O3','-g','-fwrapv','-fno-exceptions','-fno-rtti','-Wno-deprecated-register','-Wno-writable-strings','-Wno-non-c-typedef-for-linkage','-I'+str(SRC),'-I'+str(OUT)]
 for name,extra in [('native',[]),('native-fixed',['-DFC_ARM']),('native-sanitize',['-fsanitize=address,undefined','-fno-sanitize=array-bounds,pointer-overflow','-fno-sanitize-recover=all'])]:
  subprocess.run(['clang++',*common,*extra,*sources,SRC/'core.cpp',SRC/'native.cpp','-o',OUT/name],check=True,cwd=ROOT)

def arm_objects(sources,worker=None,extras=(),defines=()):
 bench=ROOT/'amiga/mesen_bench'
 flags=['--target=arm-none-eabi','-mcpu=cortex-a9','-marm','-mfloat-abi=soft','-mno-unaligned-access','-ffreestanding','-fno-builtin','-fPIC','-fvisibility=hidden','-fno-stack-protector','-O3','-g','-fwrapv','-DFC_ARM','-DDINGUX','-DZZ_BLOCK_SIZE=0x400000','-DZZ_CONTROL=0x80000','-DMB_MMU','-DMB_DCACHE','-DMB_ALLOCATION=0x400000u','-Wno-deprecated-register','-Wno-writable-strings','-Wno-non-c-typedef-for-linkage','-I'+str(SRC),'-I'+str(OUT),'-I'+str(bench),'-Iamiga/arm_debug','-Iamiga/arm_debug/zz9000']
 flags+=list(defines)
 objects=[]
 for source in [bench/'mmu_asm.S',bench/'mmu.c',bench/'runtime.c',SRC/'runtime.c',worker or SRC/'worker.cpp',SRC/'core.cpp',*sources,*extras]:
  obj=OUT/((source.parent.name+'-'+source.stem)+'.arm.o');objects.append(obj)
  extra=['-std=c++14','-fno-exceptions','-fno-rtti','-nostdinc++'] if source.suffix=='.cpp' else []
  subprocess.run(['clang',*flags,*extra,'-c',source,'-o',obj],check=True,cwd=ROOT)
 ld='/opt/homebrew/opt/lld/bin/ld.lld'
 subprocess.run([ld,'-shared','-Bsymbolic','--no-undefined','-T',SRC/'payload.ld',*objects,'-o',OUT/'fceux.elf'],check=True,cwd=ROOT)
 return flags


def build_host(flags,command=None):
 from build_zz9000_debug import unpack_elf
 from amiga import PIN,container_command
 sys.path.insert(0,str(SRC))
 from elf import unpack
 image,rel,entry=unpack((OUT/'fceux.elf').read_bytes())
 ld='/opt/homebrew/opt/lld/bin/ld.lld'
 subprocess.run(['clang',*flags,'-c','amiga/arm_debug/zz9000/mapping_probe.S','-o',OUT/'mapping.o'],check=True,cwd=ROOT)
 subprocess.run([ld,'-shared','-Bsymbolic','--no-undefined','-T','amiga/arm_debug/zz9000/payload.ld',OUT/'mapping.o','-o',OUT/'mapping.elf'],check=True,cwd=ROOT)
 mapping,mrel,point=unpack_elf((OUT/'mapping.elf').read_bytes());assert not mrel and not point and len(mapping)<=1024
 def array(name,data):return 'static const unsigned char '+name+'[]={'+','.join(map(str,data))+'};\n'
 (OUT/'zz_payload.h').write_text(f'#define ZZ_ENTRY {entry}u\n'+array('zz_image',image)+array('zz_mapping_probe',mapping)+'static const unsigned long zz_relocations[]={'+','.join(map(str,rel))+'};\n')
 relative=OUT.relative_to(ROOT)
 subprocess.run([*(command or container_command()),'run','--rm','--platform','linux/amd64','-v',f'{ROOT}:/work','-w','/work',PIN['image'],
 'm68k-amigaos-gcc','-std=c99','-noixemul','-m68020','-O2','-g','-Wall','-Wextra','-Werror',
 '-DZZ_HEADLESS_BENCH','-DZZ_RELEASE','-DZZ_NES','-DZZ_BLOCK_SIZE=0x400000','-DZZ_CONTROL=0x80000',
 '-DZZ_APP_NAME="ZZFceuxBench"','-DZZ_APP_VERSION="0.1-dev"','-DZZ_MIN_STACK=65536','-DMB_ALLOCATION=0x400000u','-DMB_MMU','-DMB_DCACHE','-DZZ_HEADLESS_MMU','-DZZ_HEADLESS_ICACHE',
 '-Iamiga/fceux_bench','-Iamiga/mesen_bench','-Iamiga/arm_debug','-Iamiga/arm_debug/zz9000','-I'+str(relative),
 'amiga/arm_debug/zz9000/launcher.c','amiga/fceux_bench/host.c','-lamiga','-o',str(relative/'ZZFceuxBench')],cwd=ROOT,check=True)
 binary=OUT/'ZZFceuxBench';assert binary.read_bytes()[:4]==b'\0\0\x03\xf3'
 recorded=[*SRC.rglob('*'),ROOT/'scripts/build_fceux_bench.py',*ROOT.joinpath('amiga/mesen_bench').glob('mmu*'),ROOT/'amiga/mesen_bench/runtime.c',ROOT/'amiga/arm_debug/zz9000/launcher.c',ROOT/'amiga/arm_debug/zz9000/layout.h',ROOT/'amiga/arm_debug/zz9000/mapping_probe.S',ROOT/'amiga/arm_debug/protocol.h']
 record={'upstream':COMMIT,'scope':'NTSC mapper 0/90 headless CPU/PPU/APU; 32kHz quality0; eight sprites; no frameskip; zero input',
 'cache_policy':'Exec-owned 4MiB; runtime translation probe; private L1 outer NC; no display/AHI',
 'image_bytes':len(image),'relocations':len(rel),'flags':[x for x in flags if not x.startswith('-I')],'compiler':subprocess.check_output(['clang','--version'],text=True),
 'sources':{str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in recorded if p.is_file() and "__pycache__" not in p.parts},
 'outputs':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in [binary,OUT/'fceux.elf',OUT/'integration.patch']},'executable_md5':hashlib.md5(binary.read_bytes()).hexdigest()}
 (OUT/'build.json').write_text(json.dumps(record,indent=2)+'\n');print(f'Built {binary}: {binary.stat().st_size} bytes; ARM image {len(image)} bytes')

if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--host',action='store_true');p.add_argument('--container-command');a=p.parse_args()
 sources=prepare();build_native(sources);flags=arm_objects(sources)
 if a.host:build_host(flags,json.loads(a.container_command) if a.container_command else None)
