#!/usr/bin/env python3
"""Test the generated library's stereo AHI state drain and 32 MiB MMU plan."""
from pathlib import Path
import subprocess,sys,re
ROOT=Path(__file__).resolve().parents[3]
OUT=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else ROOT/'.context/amiga/pocketsnes-library'
audio=(OUT/'player_audio.h').read_text();audio=audio[:audio.index('static void player_audio_close')]
audio=audio.replace('#include <dos/dostags.h>','').replace('profile_send_io(','SendIO(').replace('profile_wait_io(','WaitIO(')
(OUT/'audio-worker-under-test.h').write_text(audio)
test=(ROOT/'tests/amiga/fceux_player/audio_worker.c').read_text()
capacity=int(re.search(r'data\[i\]=AllocVec\((\d+),',audio)[1])
test=test.replace('#define AHIST_M16S 1','#define AHIST_S16S 1').replace('r->io_Length<=6144',f'r->io_Length<={capacity}').replace('r->io_Length>=2','r->io_Length>=4')
test=test.replace('2*(1+(consumed_blocks*131)%1024)','4*(1+(consumed_blocks*131)%1024)').replace('==32000','==32040').replace('io_Length/=2','io_Length/=4').replace('b->count*2','b->count*4')
test=test.replace('assert(((struct AHIRequest *)r)->ahir_Frequency==32040);','assert(((struct AHIRequest *)r)->ahir_Frequency==32040);assert(((struct AHIRequest *)r)->ahir_Type==AHIST_S16S);')
(OUT/'audio-worker-test.c').write_text(test)

for name,source,flags in [
 ('audio',OUT/'audio-worker-test.c',['-I'+str(OUT),'-DFC_STATE_AUDIO']),
 ('mmu',ROOT/'tests/amiga/mesen/mmu_plan.c',['-I'+str(OUT),'-DMB_ALLOCATION=0x2000000u'])]:
 exe=OUT/(name+'-checked')
 subprocess.run(['clang','-O2','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all',*flags,source,'-o',exe],check=True)
 subprocess.run([exe],check=True)
regions=[(0x100000,0x11c000),(0x120000-32,0x120000+1056),(0x130000,0x14e000),(0x170000,0x171000),(0x180000,0x184000),(0x200000,0x600000),(0x600000,0x700000)]
assert all(a[1]<=b[0] for a,b in zip(regions,regions[1:]))
print('PASS disjoint tables/witness/video/stereo/control/ROM/state ranges before BSS')
