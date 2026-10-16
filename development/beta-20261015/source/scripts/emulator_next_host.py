"""Apply display frontend changes to a checksum-verified accepted host snapshot."""
from pathlib import Path
import shutil
ROOT=Path(__file__).resolve().parents[1]
def once(s,a,b):
 assert s.count(a)==1,(a[:100],s.count(a));return s.replace(a,b)
def prepare(stage,name):
 snes=name=='ZZSNES';s=(stage/'host.c').read_text()
 s=once(s,'static int core_only;','static int core_only;\nstatic unsigned fc_windowed,fc_scanlines,fc_fps,fc_optimise=1;\nstatic int em_switch_pending,em_ready;\nstatic ULONG em_fps_start,em_fps_count,em_fps_value,em_neon_frames;\n#include "display_math.h"')
 s=s.replace('static void full_cleanup(void);','static void full_cleanup(void);\nstatic void em_display_close(void);')
 s=once(s,' full_cleanup();',' em_display_close();')
 s=once(s,' direct_ready=0;\n}', ' direct_ready=0;\n if(GfxBase){CloseLibrary((struct Library*)GfxBase);GfxBase=NULL;}\n}') # GfxBase extern from proto
 s=once(s,' return; /* Firmware scanout consumes the completed slot; no pixel blit. */', ''' if(fc_windowed&&window&&direct_ready&&!direct_locked){
 BltBitMapRastPort(direct_bitmap,0,full_slots.front*240,window->RPort,window->BorderLeft,window->BorderTop,320,240,0xc0);WaitBlit();}
 return;''')
 # paint is before full_slots declaration; route through a forward helper.
 s=s.replace('BltBitMapRastPort(direct_bitmap,0,full_slots.front*240,window->RPort,window->BorderLeft,window->BorderTop,320,240,0xc0);WaitBlit();','em_window_paint();')
 s=once(s,'static void paint(void){','static void em_window_paint(void);\nstatic void paint(void){')
 s=once(s,'UWORD code=msg->Code;ReplyMsg','UWORD code=msg->Code,qualifier=msg->Qualifier;ReplyMsg')
 s=s.replace('   quit=1;\n#endif','   if(!fc_windowed)quit=1;\n#endif')
 s=s.replace('else if(cl==IDCMP_MOUSEBUTTONS)quit=1;','else if(cl==IDCMP_MOUSEBUTTONS&&!fc_windowed)quit=1;')
 s=once(s,'if(key==0x45||key==0x10)', 'if(key==0x44&&(qualifier&(IEQUALIFIER_LALT|IEQUALIFIER_RALT))){if(!(code&128))em_switch_pending=1;}\n   else if(key==0x59||key==0x45||key==0x10)')
 s=once(s,'#include "fullscreen.h"','#include "fullscreen.h"\n#include "window_display.h"\nstatic void em_window_paint(void){BltBitMapRastPort(direct_bitmap,0,full_slots.front*240,window->RPort,window->BorderLeft,window->BorderTop,320,240,0xc0);WaitBlit();}')
 s=once(s,' profile_request=us();present_pre_us+=profile_request-profile_start;', ' ad_put(shared+MB_HOST,36,fc_scanlines|((fc_fps?em_fps_value+1:0)<<8));ad_put(shared+MB_HOST,40,fc_optimise);\n profile_request=us();present_pre_us+=profile_request-profile_start;')
 s=once(s,' direct_ticks+=ad_get(shared+MB_REPLY,28);', ' if(ad_get(shared+MB_REPLY,72)==0x4e454f4eu)em_neon_frames++;\n direct_ticks+=ad_get(shared+MB_REPLY,28);')
 s=once(s,'   if(direct_format==ND_RGB24)', '   c=em_effect(c,x,y,fc_scanlines,fc_fps?em_fps_value+1:0);\n   if(direct_format==ND_RGB24)')
 s=s.replace('want[3]=255;', 'want[3]=c>>24;').replace('want[0]=255;', 'want[0]=c>>24;')
 s=once(s,' direct_ready=1;\n return 1;', ''' direct_ready=1;
 {ULONG now=us();if(!em_fps_start)em_fps_start=now;em_fps_count++;
 if((ULONG)(now-em_fps_start)>=1000000u){em_fps_value=(ULONG)((uint64_t)em_fps_count*1000000u/(now-em_fps_start));if(em_fps_value>999)em_fps_value=999;em_fps_start=now;em_fps_count=0;}}
 if(!em_ready){if(!full_drain())return 0;FILE*f=fopen("PROGDIR:Player.ready","w");if(f){fputs("ready\\n",f);fclose(f);}em_ready=1;}
 return 1;''')
 s=s.replace('if(!full_open() || !full_measure_refresh())goto done;','if(!full_open() || (!fc_windowed&&!full_measure_refresh()))goto done;')
 s=once(s,'  if(!direct_locked)events();\n  if(fc_state_action)', '  if(!direct_locked)events();\n  if(em_switch_pending){if(!em_switch())goto done;player_audio_flush();if(player_paused){if(!present(ad_get(shared+MB_REPLY,24),frames)||!full_drain())goto done;paint();}}\n  if(fc_state_action)')
 s=s.replace('frames%60==0','frames%600==0') # avoid file writes during steady gameplay
 s=once(s,' puts("STAGE closing frontend; requesting ARM return next");',' printf("PRESENT_OPTIONS window=%u scanlines=%u fps=%u optimised=%u neon_frames=%u\\n",fc_windowed,fc_scanlines,fc_fps,fc_optimise,em_neon_frames);\n puts("STAGE closing frontend; requesting ARM return next");')
 s=s.replace('#include <devices/timer.h>','#include <devices/timer.h>\n#include <devices/inputevent.h>')
 (stage/'host.c').write_text(s)
 p=stage/'session.h';s=p.read_text();s=once(s,' if(ferror(f))goto end;', ''' if(fgets(line,sizeof(line),f)){
 if(sscanf(line,"DISPLAY %u %u %u %u",&fc_windowed,&fc_scanlines,&fc_fps,&fc_optimise)!=4||fc_windowed>1||fc_scanlines>3||fc_fps>1||fc_optimise>1)goto end;}
 if(ferror(f))goto end;''');p.write_text(s)
 p=stage/'fullscreen.h';s=p.read_text().replace('XANI','Xanxi')
 s=once(s,'static int full_front(void){return ', 'static int full_front(void){if(fc_windowed)return window!=NULL;return ')
 s=once(s,'static int full_poll(void){','static int full_poll(void){\n if(fc_windowed)return !full_fault;')
 s=once(s,'static int full_queue(void){', 'static int full_queue(void){\n if(fc_windowed){full_slots.front=full_slots.render;full_slots.flips++;full_unlock();return 1;}')
 s=once(s,'static int full_open(void){','static int em_fullscreen_open(void){')
 s=s.replace(' if(GfxBase){CloseLibrary((struct Library *)GfxBase);GfxBase=NULL;}','')
 p.write_text(s)
 for n in ['display_math.h','window_display.h']:shutil.copy2(ROOT/'amiga/emulator'/n,stage/n)
 p=stage/'window_display.h';s=p.read_text().replace('EM_SLOT_HEIGHT','960' if snes else '720').replace('EM_TITLE','"'+name+'"');p.write_text(s)
