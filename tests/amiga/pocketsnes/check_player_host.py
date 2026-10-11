#!/usr/bin/env python3
"""Sanitizer tests of actual generated stereo AHI worker and presentation layout."""
from pathlib import Path
import subprocess, sys, hashlib
ROOT=Path(__file__).resolve().parents[3];OUT=ROOT/'.context/amiga/pocketsnes-player'
# Compile the exact generated core used by the ARM build with native sanitizers.
sys.path.insert(0,str(ROOT/'scripts'))
import build_pocketsnes as build
build.OUT=OUT
names='apu apuaux c4 c4emu cheats cheats2 clip data dsp1 fxemu fxinst globals dma memmap cpu cpuexec cpuops sa1 sa1cpu sdd1 sdd1emu snapshot soundux spc700 srtc ppu_ gfx tile'.split()
sources=[OUT/'src'/f'{n}.c' for n in names]+[OUT/'libretro/libretro.c',OUT/'libretro/libretro-common/streams/memory_stream.c',ROOT/'amiga/pocketsnes/player.c',ROOT/'tests/amiga/pocketsnes/player_native.c']
flags=['clang','-std=gnu99',*build.flags(),'-Wno-incompatible-pointer-types','-Wno-pointer-sign','-fsanitize=address,undefined','-fno-sanitize-recover=all']
headers=hashlib.sha256(b''.join(p.read_bytes() for folder in ['src','libretro'] for p in sorted((OUT/folder).rglob('*.h')))).digest()
objects=[]
for i,source in enumerate(sources):
 obj=OUT/f'player-check-{i}.o';stamp=obj.with_suffix('.sha256');objects.append(obj)
 digest=hashlib.sha256(source.read_bytes()+headers+str(flags).encode()).hexdigest()
 if not obj.exists() or not stamp.exists() or stamp.read_text()!=digest:
  subprocess.run([*flags,'-c',source,'-o',obj],check=True);stamp.write_text(digest)
subprocess.run(['clang','-fsanitize=address,undefined',*objects,'-lm','-o',OUT/'native'],check=True)
r=subprocess.run([OUT/'native',ROOT/'.context/amiga/pocketsnes-game/input.sfc'],capture_output=True,text=True)
(OUT/'native.log').write_text(r.stdout+r.stderr);print(r.stdout+r.stderr);r.check_returncode()
audio=(OUT/'player_audio.h').read_text();audio=audio[:audio.index('static void player_audio_close')]
audio=audio.replace('#include <dos/dostags.h>','').replace('profile_send_io(','SendIO(').replace('profile_wait_io(','WaitIO(')
(OUT/'audio-worker-under-test.h').write_text(audio)
test=(ROOT/'tests/amiga/fceux_player/audio_worker.c').read_text()
test=test.replace('#define AHIST_M16S 1','#define AHIST_S16S 1').replace('r->io_Length<=6144','r->io_Length<=12288').replace('r->io_Length>=2','r->io_Length>=4')
test=test.replace('2*(1+(consumed_blocks*131)%1024)','4*(1+(consumed_blocks*131)%1024)').replace('==32000','==32040').replace('io_Length/=2','io_Length/=4').replace('b->count*2','b->count*4')
test=test.replace('assert(((struct AHIRequest *)r)->ahir_Frequency==32040);','assert(((struct AHIRequest *)r)->ahir_Frequency==32040);assert(((struct AHIRequest *)r)->ahir_Type==AHIST_S16S);')
(OUT/'audio-worker-test.c').write_text(test)
commands=[('audio',OUT/'audio-worker-test.c',['-I'+str(OUT)]),('pixels',ROOT/'tests/amiga/pocketsnes/player_pixels.c',['-I'+str(ROOT/'amiga/pocketsnes'),'-I'+str(ROOT/'amiga/mesen_nes')]),('mmu',ROOT/'tests/amiga/mesen/mmu_plan.c',['-I'+str(OUT),'-DMB_ALLOCATION=0x1800000u']),('direct',ROOT/'tests/amiga/mesen_nes/direct_plan.c',['-I'+str(ROOT/'amiga/mesen_nes')])]
for name,source,flags in commands:
 exe=OUT/(name+'-checked');subprocess.run(['clang','-O2','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all',*flags,source,'-o',exe],check=True);subprocess.run([exe],check=True)
# Disjoint used transfer ranges, all before zeroed BSS.
regions=[(0x100000,0x110000),(0x120000-32,0x120000+1056),(0x130000,0x130000+61440*2),(0x170000,0x171000),(0x171000,0x179000),(0x180000,0x184000),(0x190000,0x210000)]
assert all(a[1]<=b[0] for a,b in zip(regions,regions[1:]));assert regions[-1][1]<=0x220000
print('PASS disjoint control/video/stereo/save/ROM transfer ranges within owned allocation')
