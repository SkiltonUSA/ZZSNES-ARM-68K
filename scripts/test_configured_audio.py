"""Check the built release worker against fake AHI: routing, PCM order and cleanup.
Not proof of physical playback or audible quality. Build --release-ax first.
"""
from pathlib import Path
import subprocess
root=Path(__file__).resolve().parents[1]
name, stereo = root.name, True
r=root;o=root/'.context/configured-audio-test';o.mkdir(parents=True,exist_ok=True)
audio=(r/'.context/ax-release-build/player/player_audio.h').read_text().split('static void player_audio_close')[0]
audio=audio.replace('#include <dos/dostags.h>','').replace('profile_send_io(','SendIO(').replace('profile_wait_io(','WaitIO(')
(o/'audio-worker-under-test.h').write_text('static unsigned fc_frame_us=16689;\n'+audio)
test=(r/'tests/amiga/fceux_player/audio_worker.c').read_text()
if stereo:
 for a,b in [('#define AHIST_M16S 1','#define AHIST_S16S 1'),('r->io_Length<=6144','r->io_Length<=12288'),('r->io_Length>=2','r->io_Length>=4'),('2*(1+(consumed_blocks*131)%1024)','4*(1+(consumed_blocks*131)%1024)'),('==32000','==32040'),('io_Length/=2','io_Length/=4'),('b->count*2','b->count*4')]:test=test.replace(a,b)
(o/'worker.c').write_text(test)
subprocess.run(['clang','-O2','-g','-DFC_STATE_AUDIO','-fsanitize=address,undefined','-fno-sanitize-recover=all','-I'+str(o),str(o/'worker.c'),'-o',str(o/'test')],check=True)
subprocess.run([str(o/'test')],check=True)
print(name,'configured AHI unit 0 verified')
