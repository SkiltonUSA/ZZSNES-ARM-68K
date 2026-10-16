"""SNES-only PAL cadence and completed-picture metadata integration."""
from pathlib import Path
import shutil
ROOT=Path(__file__).resolve().parents[1]
LIB=ROOT/'amiga/pocketsnes/library'
def replace(s,old,new):
 assert old in s,old
 return s.replace(old,new)
def frontend(out):
 shutil.copy2(LIB/'region.h',out/'region.h')
 shutil.copy2(LIB/'pace_target.h',out/'pace_target.h')
 f=out/'worker.cpp';s=f.read_text()
 s=replace(s,'extern "C" int fc_state_load(const uint8_t *,unsigned);','extern "C" int fc_state_load(const uint8_t *,unsigned);\nextern "C" unsigned ps_video_height(void),ps_frame_us(void);')
 s=replace(s,'ad_put(reply,16,initialized?nes_frames():0);','ad_put(reply,56,initialized?ps_video_height():224);ad_put(reply,60,initialized?ps_frame_us():0);\n  ad_put(reply,16,initialized?nes_frames():0);');f.write_text(s)
 f=out/'host.c';s=f.read_text();s=replace(s,'#include "pixels.h"','#include "pixels.h"\n#include "region.h"')
 s=replace(s,'static ULONG next_frame_us;','static ULONG next_frame_us;\nstatic ULONG fc_frame_us=16689u,fc_picture_height=224u;')
 s=replace(s,'   if(op==NES_FRAME)frame_reply_ticks', '''   if(op==NES_INIT||op==NES_FRAME||op==10){ULONG h=ad_get(reply,56),period=ad_get(reply,60);
    if(!ps_height_valid(h)||period!=fc_frame_us){puts("SNES region/geometry mismatch");return 0;}fc_picture_height=h;}
   if(op==NES_FRAME)frame_reply_ticks''')
 s=replace(s,'printf("CARTRIDGE SNES ordinary LoROM bytes=%u\\n",size);','fc_frame_us=ps_period_us(snes_pal(rom,size));\n printf("CARTRIDGE SNES region=%s period_us=%u bytes=%u\\n",snes_pal(rom,size)?"PAL":"NTSC",fc_frame_us,size);')
 s=s.replace('Supported: NTSC LoROM/HiROM','Supported: NTSC/PAL LoROM/HiROM').replace('SPC/DSP, NTSC 32040Hz','SPC/DSP, auto-region 32040Hz')
 s=replace(s,'fc_next_deadline(frame_start,next_frame_us)','fc_next_deadline(frame_start,next_frame_us,fc_frame_us)')
 s=s.replace('target=target>33378u?target-33378u:0;','target=target>2*fc_frame_us?target-2*fc_frame_us:0;')
 s=replace(s,'#include "region.h"','#include "region.h"\n#include "pace_target.h"')
 s=replace(s,'(LONG)(next_frame_us-frame_start)>0?next_frame_us-frame_start:0;target=target>2*fc_frame_us?target-2*fc_frame_us:0;', 'ps_pace_target(frame_start,next_frame_us,fc_frame_us);')
 s=replace(s,'int verify=verify_frame(frame),capture=fc_snapshot_due(frame);','ULONG view_opt=fc_view|(fc_picture_height==239?256u:0u);\n int verify=verify_frame(frame),capture=fc_snapshot_due(frame);') if 'capture=fc_snapshot_due(frame)' in s else replace(s,'int verify=verify_frame(frame);','ULONG view_opt=fc_view|(fc_picture_height==239?256u:0u);\n int verify=verify_frame(frame);')
 s=replace(s,'ad_put(shared+MB_HOST,32,fc_view);','ad_put(shared+MB_HOST,32,view_opt);')
 s=replace(s,'{int sx=fv_source(x,fc_view),sy=fv_y(y,fc_view);if(sx<0||sy<0)c=fv_border(x,y,fc_view);','{int sx=fv_source(x,view_opt),sy=fv_y(y,view_opt);if(sx<0||sy<0)c=fv_border(x,y,view_opt);')
 f.write_text(s)
 (out/'pace.h').write_text('''/* SNES-only deadlines matching the core-reported region. */
static inline ULONG fc_next_deadline(ULONG now,ULONG previous,ULONG period){
 if(!previous || (LONG)(now-previous)>(LONG)(period*2u))previous=now;
 return previous+period;
}
''')
