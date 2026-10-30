"""Current beta audio scheduling and integer Workbench window sizes."""
from pathlib import Path
import shutil

def edit(path, before, after):
 s=path.read_text()
 if before not in s:raise ValueError(f'{path.name}: missing {before[:70]}')
 path.write_text(s.replace(before,after))

def prepare_launcher(lib):
 from prepare_output_quality import prepare_launcher as audio_launcher
 audio_launcher(lib)
 edit(lib/'library_mui.h','"Fullscreen","Workbench window",NULL','"Fullscreen","Window 1x (320x240)","Window 2x (640x480)","Window 3x (960x720)",NULL')
 edit(lib/'ui_model.h','m->windowed>1','m->windowed>3')
 p=lib/'preferences.h'
 if p.exists():edit(p,'p->windowed<2','p->windowed<4')

def prepare(stage,lib,snes):
 source=Path(__file__).resolve().parents[1]/'development/audio-window-20261029'
 for f in ('window_scale.h','direct_plan.h'):shutil.copy2(source/f,stage/f)
 prepare_launcher(lib)
 edit(stage/'session.h','fc_windowed>1','fc_windowed>3')
 audio=stage/'player_audio.h'
 if not snes:
  edit(audio,'#define AUDIO_PREBUFFER 4u','#define AUDIO_PREBUFFER 6u')
  edit(audio,'8 queue slots, 4-frame prebuffer','8 queue slots, 6-frame prebuffer')
  edit(audio,'One frame of margin beyond a three-frame AHI batch.','Two complete three-frame AHI batches.')
 else:
  edit(stage/'cadence.h','(int32_t)period){c->deadline=now','(int32_t)(period*4u)){c->deadline=now')
  edit(stage/'cadence.h','A stall over one\n  * frame rebases it','A stall over four\n  * frames rebases it')
 # Publish a completed batch immediately, before another presentation can stall.
 edit(audio,'#include <dos/dostags.h>','#include <dos/dostags.h>\n#ifndef EM_AUDIO_EARLY\n#define EM_AUDIO_EARLY\n#endif')
 # Large Workbench blits need a larger presentation budget, not fewer core frames.
 edit(stage/'cadence.h','ps_cadence_show(struct ps_cadence *c,uint32_t now,uint32_t period)', 'ps_cadence_show_budget(struct ps_cadence *c,uint32_t now,uint32_t budget,unsigned max_skip)')
 edit(stage/'cadence.h','now+period/4-c->deadline','now+budget-c->deadline')
 edit(stage/'cadence.h','c->streak<2','c->streak<max_skip')
 edit(stage/'cadence.h','Budget a quarter frame for conversion/queueing. Always show the first\n  * picture and never omit more than two consecutive completed pictures.','Reserve the selected presentation budget. Always show the first\n  * picture and bound consecutive omissions; core/APU frames are retained.')
 edit(stage/'cadence.h','#endif','static inline int ps_cadence_show(struct ps_cadence*c,uint32_t now,uint32_t period){return ps_cadence_show_budget(c,now,period/4,2); }\n#endif')
 h=stage/'host.c';f=stage/'fullscreen.h'
 edit(h,'return n==1 || (player_audit && n%60==0);','return (n==1&&fc_windowed<2) || (player_audit&&(n==1||n%60==0));')
 edit(h,'if(!core_only && ps_cadence_show(&cadence,us(),fc_frame_us))','if(!core_only && ((player_audit&&verify_frame(frames+1))||ps_cadence_show_budget(&cadence,us(),fc_windowed>1?fc_frame_us*(fc_windowed-1)/2:fc_frame_us/4,fc_windowed>1?2*fc_windowed-1:2)))')
 if snes:
  edit(f,'return (fc_scanlines==4||(fc_core_options&32))?640:320;','return fc_windowed?320*fc_windowed:(fc_scanlines==4||(fc_core_options&32))?640:320;')
  edit(f,'return (fc_scanlines==4||(fc_core_options&32))?480:240;','return fc_windowed?240*fc_windowed:(fc_scanlines==4||(fc_core_options&32))?480:240;')
 else:
  edit(f,'#include "flip_plan.h"','#include "flip_plan.h"\nstatic unsigned em_width(void){return 320*(fc_windowed?fc_windowed:1);}\nstatic unsigned em_height(void){return 240*(fc_windowed?fc_windowed:1);}')
  # Geometry in the window path and host slot accounting; fullscreen stays 320x240.
  for a,b in [('full_slots.render*240*pitch','full_slots.render*em_height()*pitch'),('pitch<320*4','pitch<em_width()*4'),('0x4000000u-720*pitch','0x4000000u-3*em_height()*pitch'),('span=239*ri.BytesPerRow+320*bpp','span=(em_height()-1)*ri.BytesPerRow+em_width()*bpp'),('nd_span(addr,ri.BytesPerRow,direct_format)','nd_span_scaled(addr,ri.BytesPerRow,direct_format,fc_windowed?fc_windowed:1)'),('full_slots.front*240','full_slots.front*em_height()'),('window->BorderTop,320,240,0xc0','window->BorderTop,em_width(),em_height(),0xc0')]:edit(h,a,b)
 if snes:edit(h,'(fc_scanlines==4||(fc_core_options&32))?2:1','fc_windowed?fc_windowed:(fc_scanlines==4||(fc_core_options&32))?2:1')
 # A skipped picture has no new window pixels. Refresh events still repaint.
 edit(h,'static void em_window_paint(void);','static ULONG em_painted_flip;\nstatic void em_window_paint(void);')
 edit(h,'em_height(),0xc0);WaitBlit();}', 'em_height(),0xc0);WaitBlit();em_painted_flip=full_slots.flips;}')
 edit(h,'before=us();paint();draw_us+=us()-before;','before=us();if(!fc_windowed||em_painted_flip!=full_slots.flips)paint();draw_us+=us()-before;')
 edit(h,'ad_put(shared+MB_HOST,40,fc_optimise);','ad_put(shared+MB_HOST,40,fc_optimise);ad_put(shared+MB_HOST,44,fc_windowed);')
 # Verify a normalized 320x240 readback for enlarged ordinary frames. High-res
 # SNES retains the full-size verifier and its native source coordinates.
 if snes:
  edit(h,'unsigned vx=(fc_scanlines==4||(fc_core_options&32))?x/2:x,vy=(fc_scanlines==4||(fc_core_options&32))?y/2:y;','unsigned vx=x/(em_width()/320),vy=y/(em_height()/240);')
  edit(h,'ps_hires_x(x,view_opt),sy=ps_hires_y(y,view_opt)','ps_hires_x(x*640/em_width(),view_opt),sy=ps_hires_y(y*480/em_height(),view_opt)')
  edit(h,'fv_border(x/2,y/2,view_opt)','fv_border(vx,vy,view_opt)')
  edit(h,'c=em_effect(c,x,y,em_soft_mode(),','c=em_effect(c,fc_windowed?vx:x,fc_windowed?vy:y,em_soft_mode(),')
 else:
  edit(h,'UBYTE *pixel=direct_readback+y*pitch+x*bpp;','unsigned scale=fc_windowed?fc_windowed:1;\n   UBYTE *pixel=direct_readback+y*scale*pitch+x*scale*bpp;')
 w=stage/'window_display.h'
 edit(w,'static struct Screen*em_public_screen;','#include "window_scale.h"\nstatic struct Screen*em_public_screen;\nstatic unsigned em_last_window=1;')
 edit(w,'ULONG format=p96GetBitMapAttr', 'fc_windowed=em_window_fit(fc_windowed,em_public_screen->Width,em_public_screen->Height,em_public_screen->BarHeight+20);\n ULONG format=p96GetBitMapAttr')
 if not snes:
  for a,b in [('p96AllocBitMap(320,720,32','p96AllocBitMap(em_width(),3*em_height(),32'),('WA_InnerWidth,320,WA_InnerHeight,240','WA_InnerWidth,em_width(),WA_InnerHeight,em_height()'),('AllocVec(240*4096','AllocVec(em_height()*4096')]:edit(w,a,b)
 edit(w,'WA_Left,100,WA_Top,70,','WA_Left,0,WA_Top,em_public_screen->BarHeight+1,')
 edit(w,'full_attached=1;return 1;','printf("WINDOW scale=%u geometry=%ux%u\\n",fc_windowed,em_width(),em_height());\n em_painted_flip=~0u;full_attached=1;return 1;')
 edit(w,'em_display_close();fc_windowed=!fc_windowed;','em_display_close();if(fc_windowed){em_last_window=fc_windowed;fc_windowed=0;}else fc_windowed=em_last_window;')
 # Screenshot encoder expects the logical 320x240 image, independently of window size.
 if snes:
  edit(h,'  /* Save the visible image rows, without the added black separators. */\n  if(fc_scanlines==4||(fc_core_options&32)){\n   for(y=0;y<240;y++)for(x=0;x<320;x++)\n    for(i=0;i<4;i++)direct_readback[(y*320+x)*4+i]=direct_readback[y*2*pitch+x*8+i];\n   fc_snapshot_store(direct_readback,320*240*4,320*4,direct_format);\n  }else fc_snapshot_store(direct_readback,span,pitch,direct_format);','  /* Normalize once, in the snapshot encoder, for both hires and windows. */\n  fc_snapshot_store(direct_readback,span,pitch,direct_format);')
 p=stage/'snapshot_host.h'
 edit(p,'UBYTE*p=NULL,*check=NULL;','UBYTE*p=NULL,*check=NULL,*logical=NULL;')
 edit(p,'if(!ss_encode(p,source,span,pitch,format,','if(em_width()>320){unsigned scale=em_width()/320;\n  logical=AllocVec(320*240*4,MEMF_PUBLIC);if(!logical)goto end;\n  em_window_reduce(logical,source,pitch,scale);source=logical;span=320*240*4;pitch=320*4;\n }\n if(!ss_encode(p,source,span,pitch,format,')
 edit(p,'end:\n','end:\n if(logical)FreeVec(logical);\n')
 # ARM converts one logical row, then expands directly into the owned bitmap.
 p=stage/'direct.cpp';edit(p,'#include "direct_plan.h"','#include "direct_plan.h"\n#include "window_scale.h"')
 if snes:
  edit(p,'uint32_t span=nd_span_scaled','unsigned window_scale=ad_get(base+MB_HOST,44);if(window_scale>3)return 6;if(window_scale)scale=window_scale;\n uint32_t span=nd_span_scaled')
  edit(p,'table+3072','table+4096')
  edit(p,'int xs[640];for(unsigned x=0;x<640;x++)xs[x]=ps_hires_x(x,opt);','int xs[960];for(unsigned x=0;x<320*scale;x++)xs[x]=ps_hires_x(x*2/scale,opt);')
  edit(p,'for(unsigned y=0;y<480;y++)','for(unsigned y=0;y<240*scale;y++)')
  edit(p,'ps_hires_y(y,opt)','ps_hires_y(y*2/scale,opt)')
  edit(p,'for(unsigned x=0;x<640;x++)','for(unsigned x=0;x<320*scale;x++)')
  edit(p,'fv_border(x/2,y/2,opt)','fv_border(x/scale,y/scale,opt)')
  edit(p,'c=em_effect(c,x,y,scan,0)','c=em_effect(c,window_scale?x/scale:x,window_scale?y/scale:y,scan,0)')
  edit(p,'em_fps_ink(x/2,y/2,fps-1)','em_fps_ink(x/scale,y/scale,fps-1)')
 else:
  edit(p,'uint32_t span=nd_span(addr,pitch,format)','unsigned window_scale=ad_get(base+MB_HOST,44),scale=window_scale?window_scale:1;if(scale>3)return 6;\n uint32_t span=nd_span_scaled(addr,pitch,format,scale)')
  edit(p,'table+2048','table+4096')
  edit(p,'nd_map(l1,(uint32_t*)table,table,addr,pitch,format)','nd_map_scaled(l1,(uint32_t*)table,table,addr,pitch,format,scale)')
 edit(p,'dst+y*pitch);\n  unsigned level','dst+y*scale*pitch);\n  unsigned level')
 edit(p,'bool vector_dim=neon&&','bool vector_dim=scale==1&&neon&&')
 edit(p,'if(vector_dim)em_dim_neon(row,row_words,level);','if(scale>1)em_window_expand(row,pitch/4,row_words,scale);\n  else if(vector_dim)em_dim_neon(row,row_words,level);')

 if snes:
  # Cortex-A9 has no integer divide: cache coordinates outside the pixel loop.
  edit(p,'  uint16_t *full=ps_full_pixels();int xs[960];for(unsigned x=0;x<320*scale;x++)xs[x]=ps_hires_x(x*2/scale,opt);\n  for(unsigned y=0;y<240*scale;y++){\n   if(ad_get(base,ZZ_STOP)){error=2;break;}\n   int sy=ps_hires_y(y*2/scale,opt);volatile uint32_t *row=(volatile uint32_t*)(dst+y*pitch);\n   for(unsigned x=0;x<320*scale;x++){\n    uint32_t c=xs[x]<0||sy<0?fv_border(x/scale,y/scale,opt):pocket_rgb(full[sy*512+xs[x]]);\n    c=em_effect(c,window_scale?x/scale:x,window_scale?y/scale:y,scan,0);\n    if(fps){int ink=em_fps_ink(x/scale,y/scale,fps-1);if(ink>=0)c=ink?0xffffffffu:0xff000000u;}\n    row[x]=format==3?c:__builtin_bswap32(c);\n   }\n  }','  uint16_t *full=ps_full_pixels(),logical_x[960];int xs[960];\n  for(unsigned x=0;x<320*scale;x++){xs[x]=ps_hires_x(x*2/scale,opt);logical_x[x]=x/scale;}\n  for(unsigned y=0;y<240*scale;y++){\n   if(ad_get(base,ZZ_STOP)){error=2;break;}\n   unsigned logical_y=y/scale,level=em_scan_level(scan,window_scale?logical_y:y);\n   int sy=ps_hires_y(y*2/scale,opt);volatile uint32_t *row=(volatile uint32_t*)(dst+y*pitch);\n   for(unsigned x=0;x<320*scale;x++){\n    uint32_t c=xs[x]<0||sy<0?fv_border(logical_x[x],logical_y,opt):pocket_rgb(full[sy*512+xs[x]]);\n    if(level!=8)c=em_scan_dim(c,level);\n    if(fps){int ink=em_fps_ink(logical_x[x],logical_y,fps-1);if(ink>=0)c=ink?0xffffffffu:0xff000000u;}\n    row[x]=format==3?c:__builtin_bswap32(c);\n   }\n  }')
