from pathlib import Path
import shutil

def prepare(out, backend=Path(__file__).resolve().parents[1]/"development/ax-audio-20261029"):
 p=out/'player_audio.h';s=p.read_text();s=s.replace('static int audio_opened;', 'static int audio_opened;\nstatic ULONG audio_ax_mode;');stereo='AHIST_S16S' in s
 for file in ('ax_stream.h','ax_queue.h','ax_worker.h'):shutil.copy2(backend/file,out/file)
 marker='static void player_audio_close(void){';assert s.count(marker)==1
 defs=('#define AX_STEREO\n' if stereo else '')+f'#define AX_RATE {32040 if stereo else 32000}\n#define AX_TYPE AHIST_{"S" if stereo else "M"}16S\n#define AX_BYTES {4 if stereo else 2}\n'
 s=s.replace(marker,defs+'#include "ax_worker.h"\n'+marker)
 s=s.replace('NP_Entry,(ULONG)audio_worker,','NP_Entry,(ULONG)audio_dispatch,')
 needle=' printf("AUDIO_TASK queued=';assert s.count(needle)==1
 s=s.replace(needle,' printf("AUDIO_AX mode=%08x mix=%u callbacks=%u silence_queued=%u\\n",audio_ax_mode,audio_ax_mix,audio_ax_callbacks,audio_ax_silence);\n'+needle)
 needle=' puts("AUDIO task: AHI unit 0,';assert s.count(needle)==1
 s=s.replace(needle,' if(audio_ax_mode){printf("AUDIO task: per-app AX mode=%08x mix=%u; three immutable buffers\\n",audio_ax_mode,audio_ax_mix);return 1;}\n'+needle)
 if stereo:s=s.replace('AUDIO_SLOTS+6u','AUDIO_SLOTS+(audio_ax_mode?9u:6u)').replace('Queue plus two linked AHI requests.', 'Queue plus the selected backend: two linked requests or three AX buffers.')
 p.write_text(s)
