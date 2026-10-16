"""Build the isolated compatibility core for native sanitizer diagnostics."""
from pathlib import Path
import shutil,subprocess,hashlib

def build(source,O,*,experimental=False,baseline=False,fx3=False):
 r=Path(__file__).resolve().parents[1];snap=r/'development/beta-20261015';core=O/'native-core';O.mkdir(parents=True,exist_ok=True)
 shutil.copytree(snap/'source/core',core,dirs_exist_ok=True)
 if not baseline:
  shutil.copytree(r/'development/compatibility-20261016/core',core,dirs_exist_ok=True)
 inc=O/'include';shutil.copytree(snap/'build-inputs/player',inc,dirs_exist_ok=True)
 p=inc/'cartridge.h';s=p.read_text()
 if experimental:s=s.replace('(map==0x20+i||map==0x30+i||contra)&&type<=2','(((map==0x20+i||map==0x30+i||contra)&&type<=2)||(!i&&map==0x20&&(type==0x13||type==0x14||type==0x15||type==0x1a))||(!i&&map==0x23&&(type==0x34||type==0x35)))')
 if fx3:s=(r/'development/fx3-20261016/cartridge.h').read_text()
 p.write_text(s);(core/"cartridge.h").write_text(s)
 names='apu apuaux c4 c4emu cheats cheats2 clip data dsp1 fxemu fxinst globals dma memmap cpu cpuexec cpuops sa1 sa1cpu sdd1 sdd1emu snapshot soundux spc700 srtc ppu_ gfx tile'.split()
 sources=[core/'src'/f'{n}.c'for n in names]+[core/'libretro/libretro.c',core/'libretro/libretro-common/streams/memory_stream.c',core/'player.c',source]
 flags=['clang','-std=gnu99','-O2','-g','-fno-strict-aliasing','-fwrapv','-DHAVE_STRINGS_H','-DHAVE_STDINT_H','-DHAVE_INTTYPES_H','-D__LIBRETRO__','-DUSE_SA1','-D__OLD_RASTER_FX__','-DPOCKET_NATIVE','-Wno-incompatible-pointer-types','-Wno-pointer-sign','-fsanitize=address,undefined','-fno-sanitize-recover=all']
 flags+=['-I'+str(p)for p in [inc,core,core/'src',core/'libretro',core/'libretro/libretro-common/include',snap/'source/amiga/mesen_bench']]
 headers=hashlib.sha256(b''.join(p.read_bytes()for d in [inc,core]for p in sorted(d.rglob('*.h')))).digest();objects=[]
 for i,p in enumerate(sources):
  obj=O/f'native-{i}.o';stamp=obj.with_suffix('.sha256');h=hashlib.sha256(headers+p.read_bytes()+(core/'src/cpuops.c').read_bytes()+str(flags).encode()).hexdigest();objects.append(obj)
  if not obj.exists()or not stamp.exists()or stamp.read_text()!=h:
   subprocess.run(flags+['-c',str(p),'-o',str(obj)],check=True);stamp.write_text(h)
 subprocess.run(['clang','-fsanitize=address,undefined',*map(str,objects),'-lm','-o',str(O/'native')],check=True)
 return O/"native"
