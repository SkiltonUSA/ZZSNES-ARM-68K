"""Per-app quality output and intact SNES rows. Source overlays, not generated edits."""
from pathlib import Path
import shutil
def edit(path,before,after):
 text=path.read_text()
 if before not in text:raise ValueError(f"{path.name}: missing {before[:60]}")
 path.write_text(text.replace(before,after))
ROOT=Path(__file__).resolve().parents[1]
DATA=ROOT/'development/output-quality-20261030'

def prepare_audio(stage):
 from prepare_ax_audio import prepare
 edit(stage/'player_audio.h','#define AUDIO_SLOTS 8u','#define AUDIO_SLOTS 12u')
 prepare(stage,DATA)
 shutil.copy2(DATA/'audio_output.h',stage/'audio_output.h')
 p=stage/'player_audio.h';s=p.read_text().replace('AUDIO_AX mode=','AUDIO_OUTPUT mode=').replace('per-app AX mode=','per-app AHI mode=')
 s=s.replace('silence_queued=%u\\n','silence_queued=%u starvations=%u\\n').replace('audio_ax_callbacks,audio_ax_silence);','audio_ax_callbacks,audio_ax_silence,audio_ax_starvations);')
 s=s.replace('8 queue slots','12 queue slots')
 p.write_text(s)

def prepare_launcher(lib):
 shutil.copy2(DATA/'audio_output.h',lib/'audio_output.h')
 p=lib/'library_mui.h'
 edit(p,'static const char *display_choices[]=', '#include "audio_output.h"\nstatic Object *audio_output;\nstatic const char *audio_choices[]={"Auto (configured output)","Configured AHI unit 0","Paula 14-bit / 22.05 kHz","Paula 14-bit / 28 kHz","ZZ9000AX / 32 kHz",NULL};\nstatic const char *display_choices[]=')
 edit(p,'if(!preserve_legacy())return 0;collect();','if(!preserve_legacy())return 0;if(!em_audio_save(value(audio_output,MUIA_Cycle_Active)))return 0;collect();')
 edit(p,'Child,HGroup,Child,mute=CheckMark(FALSE)', 'Child,HGroup,Child,textline("Audio output (all games)"),Child,audio_output=cycle(audio_choices),End,\n    Child,HGroup,Child,mute=CheckMark(FALSE)')
 edit(p,'set(scaling_mode,MUIA_Cycle_Active,model.smooth);','set(scaling_mode,MUIA_Cycle_Active,model.smooth);\n set(audio_output,MUIA_Cycle_Active,em_audio_load());\n set(audio_output,MUIA_ShortHelp,(ULONG)"Auto improves Paula unit 0 to 14-bit / 22.05 kHz.\\nOther configured outputs are retained. No system preference changes.");')

def prepare_rows(stage):
 p=stage/'view.h'
 edit(p,'return opt&1?216:288;', 'return opt&1?224:298;')
 edit(p,'if(y<12||y>=228)return -1;unsigned h=opt&256?239:224,sy=(y-12)*h/216,m=((opt>>6)&3)*8;', 'unsigned h=opt&256?239:224,top=(240-h)/2;if(y<top||y>=top+h)return -1;unsigned sy=y-top,m=((opt>>6)&3)*8;')
 p=stage/'hires_view.h'
 edit(p,'if(y<24||y>=456)return -1;\n unsigned sy=(y-24)*h/432;', 'unsigned top=((240-h/2)/2)*2;if(y<top||y>=top+h)return -1;\n unsigned sy=y-top;')
