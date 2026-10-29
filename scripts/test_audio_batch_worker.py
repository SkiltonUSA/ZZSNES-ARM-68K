from pathlib import Path
import subprocess,sys
stage=Path(sys.argv[1]);batch=int(sys.argv[2]);out=stage/'test';out.mkdir(exist_ok=True)
audio=(stage/'player_audio.h').read_text().split('static void player_audio_close')[0].replace('#include <dos/dostags.h>','').replace('profile_send_io','SendIO').replace('profile_wait_io','WaitIO')
stereo='AHIST_S16S' in audio;sample_bytes=4 if stereo else 2
(out/'audio-worker-under-test.h').write_text(audio)
s=(Path(__file__).resolve().parents[1]/'tests/amiga/fceux_player/audio_worker.c').read_text().replace('r->io_Length<=6144',f'r->io_Length<={1024*sample_bytes*batch}')
if stereo:
 s=s.replace('#define AHIST_M16S 1','#define AHIST_M16S 1\n#define AHIST_S16S 2\nstatic ULONG fc_frame_us=16639;').replace('2*(1+(consumed_blocks*131)%1024)','4*(1+(consumed_blocks*131)%1024)').replace('b->count*2','b->count*4').replace('ahir_Frequency==32000','ahir_Frequency==32040').replace('pending[i]->io_Length/=2','pending[i]->io_Length/=4')
s=s.replace(' run(1,1,0,0,0);',f' run(600,8,0,0,0);assert(sends=={600//batch});\n run(1,1,0,0,0);')
s=s.replace('1838 ordered blocks and forced partial flush','ordered PCM and forced partial flush')
(out/'test.c').write_text(s)
subprocess.run(['clang','-std=c99','-O2','-g','-fsanitize=address,undefined','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-Wno-unused-function','-DFC_STATE_AUDIO','-I'+str(out),str(out/'test.c'),'-o',str(out/'test')],check=True)
subprocess.run([str(out/'test')],check=True)
