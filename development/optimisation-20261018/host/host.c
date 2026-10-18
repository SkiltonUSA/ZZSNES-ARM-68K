/* GPL-3.0-or-later. 68k Workbench preview; optional ARM/P96 source bitmap. */
#include <exec/memory.h>
#include <devices/timer.h>
#include <devices/inputevent.h>
#include <graphics/gfxbase.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#ifdef NES_FULLSCREEN
#include <intuition/intuitionbase.h>
#endif
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/timer.h>
#include <proto/Picasso96.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "wire.h"
#include "zip/rom_archive.h"
static char cartridge_name[512];
#include "mmu.h"
#include "palette.h"
#include "pixels.h"
#include "region.h"
#ifdef NES_PLAYER
#include "player_palette.h"
#endif
#include "demo.h"
#ifdef NES_DIRECT
#include <proto/expansion.h>
#include <libraries/configvars.h>
#include "direct_plan.h"
static struct BitMap *direct_bitmap;
static LONG direct_lock;
static int direct_locked,direct_ready;
static ULONG direct_format,direct_native_format,direct_verified,direct_challenges;
static uint64_t direct_ticks;
static UBYTE *direct_readback;
#ifdef NES_FULLSCREEN
static void full_cleanup(void);
static void em_display_close(void);
#endif
/* A failed presentation must reach synchronous reset promptly while locked. */
unsigned nes_direct_stop_ticks(void){return direct_locked?2u:100u;}
void nes_direct_cleanup(void){
#ifdef NES_FULLSCREEN
 em_display_close();
#endif
 if(direct_locked){p96UnlockBitMap(direct_bitmap,direct_lock);direct_locked=0;}
 if(direct_bitmap){p96FreeBitMap(direct_bitmap);direct_bitmap=NULL;}
 if(direct_readback){FreeVec(direct_readback);direct_readback=NULL;}
 direct_ready=0;
 if(GfxBase){CloseLibrary((struct Library*)GfxBase);GfxBase=NULL;}
}
#endif
struct Device *TimerBase;
struct GfxBase *GfxBase;
static volatile uint8_t *shared;
static const struct ad_io *syncio;
static uint32_t sequence;
static uint64_t frame_reply_ticks,present_reply_ticks;
static const char *rom_path;
static ULONG frame_limit;
static struct Window *window;
static UBYTE *pixels,*indices;
static struct timerequest *waitio;
static ULONG waits;
static LONG pens[64];
#ifndef NES_FULLSCREEN
static int truecolor;
#endif
static int core_only;
static unsigned fc_windowed,fc_scanlines,fc_fps,fc_optimise=1;
static int em_switch_pending,em_ready;
static int em_scanline_pending=-1;
#include "scanline_key.h"
#include "scanline_backend.h"
static unsigned em_soft_mode(void){return em_scan_software(fc_windowed,fc_scanlines);}
static ULONG em_fps_start,em_fps_count,em_fps_value,em_neon_frames;
#include "display_math.h"
static ULONG next_frame_us;
static ULONG fc_frame_us=16689u,fc_picture_height=224u;
static int player_audit,fc_scripted_input;
static ULONG present_pre_us,present_request_us,present_post_us;
static int verify_frame(unsigned n){return n==1 || (player_audit && n%60==0);}
#ifndef NES_DIRECT
static struct RenderInfo render;
#endif
static int quit;
#ifdef NES_PLAYER
static int player_demo,player_paused,player_muted;
#endif
static UWORD buttons;
static ULONG us(void){struct timeval t;GetSysTime(&t);return t.tv_secs*1000000u+t.tv_micro;}
#include "session.h"
int nes_arguments(int argc,char **argv){
 if((argc==3||argc==5||argc==6)&&!strcmp(argv[1],"--session")){
 if(argc==6){if(strcmp(argv[5],"VERIFY"))return -1;player_audit=1;}
 if(argc>=5){char*end;unsigned long n;if(strcmp(argv[3],"--frames"))return -1;errno=0;n=strtoul(argv[4],&end,10);if(errno||*end||!n||n>36000)return -1;frame_limit=n;}
 return fc_session(argv[2])?1:-1;}
 if(argc<=1)return argc;
 if(argc==2){rom_path=argv[1];return 1;}
 if(argc<4||argc>6){puts("Usage: ZZSnes-Pocket [ROM.sfc] or <nonce> RUN <ROM.sfc> [frame_limit] [VERIFY|INPUT]");return -1;}
 rom_path=argv[3];
 if(argc>=5){char *end;errno=0;frame_limit=strtoul(argv[4],&end,10);if(errno||*end||!frame_limit||frame_limit>36000)return -1;}
 if(argc==6){if(!strcmp(argv[5],"VERIFY"))player_audit=1;else if(!strcmp(argv[5],"INPUT"))fc_scripted_input=player_audit=1;else if(!strcmp(argv[5],"INPUTFAST"))fc_scripted_input=1;else return -1;}
 return 3;
}
static void em_window_paint(void);
static void paint(void){
#ifdef NES_FULLSCREEN
 if(fc_windowed&&window&&direct_ready&&!direct_locked){
 em_window_paint();}
 return;
#elif defined(NES_DIRECT)
 if(!window||!direct_ready||direct_locked)return;
 BltBitMapRastPort(direct_bitmap,0,0,window->RPort,window->BorderLeft,window->BorderTop,256,240,0xc0);WaitBlit();
 return;
#else
 if(!window||!pixels)return;
 if(truecolor)p96WritePixelArray(&render,0,0,window->RPort,window->BorderLeft,window->BorderTop,256,240);
 else WriteChunkyPixels(window->RPort,window->BorderLeft,window->BorderTop,window->BorderLeft+255,window->BorderTop+239,pixels,256);
#endif
}
static void events(void){
 struct IntuiMessage *msg;
 if(SetSignal(0,SIGBREAKF_CTRL_C)&SIGBREAKF_CTRL_C)quit=1;
#ifdef NES_DIRECT
 if(direct_locked)return;
#endif
 if(!window)return;
 while((msg=(struct IntuiMessage *)GetMsg(window->UserPort))){ULONG cl=msg->Class;UWORD code=msg->Code,qualifier=msg->Qualifier;ReplyMsg((struct Message *)msg);
  if(cl==IDCMP_CLOSEWINDOW)quit=1;
  else if(cl==IDCMP_INACTIVEWINDOW){buttons=0;
#ifdef NES_FULLSCREEN
   if(!fc_windowed)quit=1;
#endif
  }
#ifdef NES_FULLSCREEN
  else if(cl==IDCMP_MOUSEBUTTONS&&!fc_windowed)quit=1;
#endif
  else if(cl==IDCMP_REFRESHWINDOW){BeginRefresh(window);paint();EndRefresh(window,TRUE);}
  else if(cl==IDCMP_RAWKEY){unsigned key=code&127,bit=0;
   if(key==0x44&&(qualifier&(IEQUALIFIER_LALT|IEQUALIFIER_RALT))){if(!(code&128))em_switch_pending=1;}
   else if(key==0x59||key==0x45||key==0x10){if(!(code&128))quit=1;}
#ifdef NES_PLAYER
   else if(key==0x19){if(!(code&128)){player_paused=!player_paused;buttons=0;}}
   else if(key==0x56){em_scanline_key(code,qualifier&IEQUALIFIER_REPEAT,fc_scanlines,fc_windowed,&em_scanline_pending);}
   else if(key==0x58){if(!(code&128))fc_snapshot_action=1;}
   else if(key==0x54||key==0x57){if(!(code&128))fc_state_action=key==0x54?1:2;}
   else if(key==0x37){if(!(code&128))player_muted=!player_muted;}
#endif
   else {switch(key){case 0x31:bit=1;break;case 0x32:bit=2;break;case 0x60:bit=4;break;case 0x44:bit=8;break;
    case 0x20:bit=256;break;case 0x21:bit=512;break;case 0x33:bit=1024;break;case 0x34:bit=2048;break;case 0x4c:bit=16;break;case 0x4d:bit=32;break;case 0x4f:bit=64;break;case 0x4e:bit=128;break;}
    if(code&128)buttons&=~bit;else buttons|=bit;}
  }
 }
}
static int pause_us(ULONG delay){
 if(!delay)return 1;
 waitio->tr_node.io_Command=TR_ADDREQUEST;
 waitio->tr_time.tv_secs=0;waitio->tr_time.tv_micro=delay;
 if(DoIO((struct IORequest *)waitio))return 0;
 waits++;return 1;
}
#ifdef NES_FULLSCREEN
#include "wait_profile.h"
#include "frame_profile.h"
#include "joystick.h"
#include "fullscreen.h"
#include "window_display.h"
static void em_window_paint(void){BltBitMapRastPort(direct_bitmap,0,full_slots.front*em_height(),window->RPort,window->BorderLeft,window->BorderTop,em_width(),em_height(),0xc0);WaitBlit();}
#include "cadence.h"
#include "pace_target.h"
static struct ps_cadence cadence;
#endif
#ifdef NES_PLAYER
static int player_audio_submit(void);
#endif
static int request(ULONG op,ULONG arg){
 ULONG start=us(),token=(sequence+1)^ad_get(shared,ZZ_CONTROL);volatile uint8_t *cmd=shared+MB_HOST,*reply=shared+MB_REPLY;
 ad_put(cmd,4,op);ad_put(cmd,8,token);ad_put(cmd,12,arg);syncio->push(cmd+4,60,NULL);syncio->barrier(NULL);
 ad_put(cmd,0,++sequence);syncio->push(cmd,4,NULL);
#ifdef NES_PLAYER
 /* The worker only reads host-owned PCM. Propagate device errors without
  * making AHI calls on the emulation/display task. */
 if(op==NES_FRAME && !player_audio_submit())return 0;
#endif
 do{
#ifdef NES_FULLSCREEN
  if(!full_poll())return 0;
#endif
  syncio->pull(reply,96,NULL);
  if(ad_get(reply,0)==sequence){if(ad_get(reply,68)!=(token^ZZ_XOR)){puts("MAILBOX FAIL");return 0;}
   if(ad_get(reply,4)){printf("NES error=%u op=%u\n",(ULONG)ad_get(reply,4),op);return 0;}
   if(op==NES_INIT||op==NES_FRAME||op==10){ULONG h=ad_get(reply,56),period=ad_get(reply,60);
    if(!ps_height_valid(h)||period!=fc_frame_us){puts("SNES region/geometry mismatch");return 0;}fc_picture_height=h;}
   if(op==NES_FRAME)frame_reply_ticks+=ad_get(reply,48);
   if(op==NES_PRESENT)present_reply_ticks+=ad_get(reply,48);
   return 1;}
#ifdef NES_DIRECT
  /* Do not enter Intuition/graphics while a bitmap is locked. */
  if(!direct_locked)events();
#else
  events();
#endif
  #ifndef NES_PLAYER
  if(quit)return 0;
  #endif
#ifdef NES_LEGACY_HOST
  Delay(1);
#else
  /* PRESENT holds a bitmap lock: bounded polling avoids yielding here. */
  if(op!=NES_PRESENT && op!=NES_FRAME && !profile_pause(1,1000))return 0;
#endif
 }while((ULONG)(us()-start)<(op==NES_PRESENT?50000u:10000000u));
 puts("NES request timeout");return 0;
}
#ifdef NES_PLAYER
#include "player_frontend.h"
static int present(ULONG,ULONG);
#include "state_host.h"
#include "view.h"
#define SNAPSHOT_SNES 1
#include "snapshot_host.h"
#endif
#ifdef NES_DIRECT
static int present(ULONG raw_hash,ULONG frame){
 ULONG profile_start=us(),profile_request,profile_after;
 struct RenderInfo ri;struct ConfigDev *card=FindConfigDev(NULL,0x6d6e,4);
 ULONG span,addr,nonce,i,x,y,pitch,bpp=nd_bpp(direct_format),bad=0;
 ULONG view_opt=fc_view|(fc_picture_height==239?256u:0u);
 int verify=verify_frame(frame),capture=fc_snapshot_due(frame);
 if(!card||!direct_bitmap)return 0;
#ifdef NES_FULLSCREEN
 if(!full_drain())return 0;
 if(!full_front()){quit=1;return 0;}
#endif
 if(!p96GetBitMapAttr(direct_bitmap,P96BMA_ISONBOARD))return 0;
 memset(&ri,0,sizeof(ri));
#ifdef NES_FULLSCREEN
 full_origin_known=0;
#endif
 direct_lock=p96LockBitMap(direct_bitmap,(UBYTE *)&ri,sizeof(ri));direct_locked=1;
 if(!ri.Memory||((ULONG)ri.Memory&3u)||ri.BytesPerRow<(LONG)(256*bpp)||ri.BytesPerRow>4096||
    (ULONG)ri.RGBFormat!=direct_native_format)return 0;
 pitch=ri.BytesPerRow;
#ifdef NES_FULLSCREEN
 /* Check the complete 4-slot scanout allocation, then target the free slot.
  * 32-pixel black borders centre the unscaled 256-pixel NES in 320 pixels. */
 if(pitch<em_width()*4 || (ULONG)ri.Memory<(ULONG)card->cd_BoardAddr+0x10000u ||
    (ULONG)ri.Memory>(ULONG)card->cd_BoardAddr+0x4000000u-4*em_height()*pitch)return 0;
 full_origin=(ULONG)ri.Memory-(ULONG)card->cd_BoardAddr-0x10000u;full_pitch=pitch;
 full_origin_known=1;
 ri.Memory=(UBYTE *)ri.Memory+full_slots.render*em_height()*pitch;
#endif
 span=(em_height()-1)*ri.BytesPerRow+em_width()*bpp;
 if((ULONG)ri.Memory<(ULONG)card->cd_BoardAddr+0x10000u||
    (ULONG)ri.Memory>(ULONG)card->cd_BoardAddr+0x4000000u-span)return 0;
 addr=(ULONG)ri.Memory-(ULONG)card->cd_BoardAddr+0x1f0000u;
 if(nd_span_scaled(addr,ri.BytesPerRow,direct_format,fc_scanlines==4?2:1)!=span)return 0;
 nonce=ad_get(shared,ZZ_CONTROL)^(sequence+1)^0x97ab301du;
 ad_put(ri.Memory,0,nonce);ad_put(ri.Memory,span-4,nonce^0x31415926u);
 syncio->push(ri.Memory,4,NULL);syncio->push((uint8_t *)ri.Memory+span-4,4,NULL);
 ad_put(shared+MB_HOST,32,view_opt);
 ad_put(shared+MB_HOST,16,ri.BytesPerRow);ad_put(shared+MB_HOST,20,direct_format);ad_put(shared+MB_HOST,24,nonce);
 ad_put(shared+MB_HOST,36,em_soft_mode()|((fc_fps?em_fps_value+1:0)<<8));ad_put(shared+MB_HOST,40,fc_optimise);
 profile_request=us();present_pre_us+=profile_request-profile_start;
 if(!request(NES_PRESENT,addr))return 0; /* Keep locked on error until Core1 reset. */
 profile_after=us();present_request_us+=profile_after-profile_request;
 if(ad_get(shared+MB_REPLY,24)!=raw_hash)return 0;
 if(ad_get(shared+MB_REPLY,72)==0x4e454f4eu)em_neon_frames++;
 direct_ticks+=ad_get(shared+MB_REPLY,28);direct_challenges++;
 /* Explicit diagnostic readback on frame 1 and each 60th. Never read live
  * Workbench storage: only our locked off-screen source after ARM ACK. */
 if(verify||capture){
  syncio->pull(ri.Memory,span,NULL);
  CopyMem(ri.Memory,direct_readback,span);
 }
#ifdef NES_FULLSCREEN
 if(!full_queue())return 0;
 /* Usual frames stay locked through ACK; the next CPU/PPU request overlaps
  * the vblank wait. Diagnostic frames drain before comparing the snapshot. */
 if((verify||capture) && !full_drain())return 0;
#else
 p96UnlockBitMap(direct_bitmap,direct_lock);direct_locked=0;
#endif
 /* Validate the snapshot after unlocking; no slow per-byte Zorro reads
  * or pixel comparisons while holding the relocation lock. */
 if(verify){
  syncio->pull(shared+NES_OUTPUT,NES_OUTPUT_BYTES,NULL);CopyMem((APTR)(shared+NES_OUTPUT),indices,NES_OUTPUT_BYTES);
  for(y=0;y<em_height();y++)for(x=0;x<em_width();x++){
   unsigned vx=fc_scanlines==4?x/2:x,vy=fc_scanlines==4?y/2:y;
   UBYTE *pixel=direct_readback+y*pitch+x*bpp;
   ULONG c;UBYTE want[4];
#ifdef NES_PLAYER
   {int sx=fv_source(vx,view_opt),sy=fv_y(vy,view_opt);if(sx<0||sy<0)c=fv_border(vx,vy,view_opt);else if(view_opt&512){unsigned a,b,w;fv_sample(vx,view_opt,&a,&b,&w);
    unsigned ia=sy*256+a,ib=sy*256+b;
    c=fv_blend(pocket_rgb(((unsigned)indices[2*ia]<<8)|indices[2*ia+1]),pocket_rgb(((unsigned)indices[2*ib]<<8)|indices[2*ib+1]),w);
   }else {unsigned index=((unsigned)indices[2*(sy*256+sx)]<<8)|indices[2*(sy*256+sx)+1];c=pocket_rgb(index);}}
#else
   if(indices[y*256+x]>=64){bad++;continue;}
   c=nes_palette[indices[y*256+x]];
#endif
   c=em_effect(c,x,y,em_soft_mode(),fc_scanlines==4?0:(fc_fps?em_fps_value+1:0));
   if(fc_scanlines==4&&fc_fps){int ink=em_fps_ink(vx,vy,em_fps_value);if(ink>=0)c=ink?0xffffffffu:0xff000000u;}
   if(direct_format==ND_RGB24){want[0]=c>>16;want[1]=c>>8;want[2]=c;}
   else if(direct_format==ND_BGR24){want[0]=c;want[1]=c>>8;want[2]=c>>16;}
   else if(direct_format==ND_BGRA32){want[0]=c;want[1]=c>>8;want[2]=c>>16;want[3]=c>>24;}
   else {want[0]=c>>24;want[1]=c>>16;want[2]=c>>8;want[3]=c;}
   for(i=0;i<bpp;i++)if(pixel[i]!=want[i])bad++;
  }
  if(bad){printf("DIRECT READBACK FAIL frame=%u bytes=%u\n",frame,bad);return 0;}
  direct_verified++;
 }
 if(capture){
  /* Save the visible image rows, without the added black separators. */
  if(fc_scanlines==4){
   for(y=0;y<240;y++)for(x=0;x<320;x++)
    for(i=0;i<4;i++)direct_readback[(y*320+x)*4+i]=direct_readback[y*2*pitch+x*8+i];
   fc_snapshot_store(direct_readback,320*240*4,320*4,direct_format);
  }else fc_snapshot_store(direct_readback,span,pitch,direct_format);
 }
 present_post_us+=us()-profile_after;
 direct_ready=1;
 {ULONG now=us();if(!em_fps_start)em_fps_start=now;em_fps_count++;
 if((ULONG)(now-em_fps_start)>=1000000u){em_fps_value=(ULONG)((uint64_t)em_fps_count*1000000u/(now-em_fps_start));if(em_fps_value>999)em_fps_value=999;em_fps_start=now;em_fps_count=0;}}
 if(!em_ready){if(!full_drain())return 0;FILE*f=fopen("PROGDIR:Player.ready","w");if(f){fputs("ready\n",f);fclose(f);}em_ready=1;}
 return 1;
}
#endif
static uint64_t armclock(void){return (uint64_t)ad_get(shared+MB_REPLY,12)<<32|ad_get(shared+MB_REPLY,8);}
int ff_window(volatile uint8_t *base,const struct ad_io *io,int connected){
 struct MsgPort *port=NULL;struct timerequest *timer=NULL;struct Screen *screen=NULL;
 ULONG size=sizeof(demo_rom),i,rate=0,t0,t1,frames=0,start=0,total_core_us=0,output_hash=0;
 ULONG request_us=0,copy_us=0,colour_us=0,draw_us=0,before,frame_start,frame_core_us;
#ifdef NES_PLAYER
 ULONG audio_us=0,flip_drain_us=0;
#endif
 uint64_t c0,c1;int opened=0,rc=20;UBYTE *rom=NULL;
 (void)connected;shared=base;syncio=io;for(i=0;i<64;i++)pens[i]=-1;
 port=CreateMsgPort();if(!port)goto done;timer=(struct timerequest *)CreateIORequest(port,sizeof(*timer));if(!timer)goto done;
 if(OpenDevice(TIMERNAME,UNIT_MICROHZ,(struct IORequest *)timer,0))goto done;
 opened=1;TimerBase=timer->tr_node.io_Device;waitio=timer;
#ifdef NES_PLAYER
 if(!player_choose()){if(quit)rc=0;goto done;}
#endif
 rom=AllocVec(NES_ROM_LIMIT+513,MEMF_PUBLIC);if(!rom)goto done;
 if(rom_path){unsigned loaded;int error=em_rom_load(rom_path,rom,NES_ROM_LIMIT+512,&loaded,1,cartridge_name,sizeof(cartridge_name));if(error){puts(em_rom_message(error));goto done;}size=loaded;}
 else memcpy(rom,demo_rom,size);
 {unsigned skip=snes_skip(size);if(skip){memmove(rom,rom+skip,size-skip);size-=skip;}}
 if(size>NES_ROM_LIMIT){puts("ROM exceeds player cartridge limit");goto done;}
#ifdef NES_PLAYER
 if(nes_cartridge(rom,size)<0){puts("Supported: NTSC/PAL LoROM/HiROM up to 4 MiB, no enhancement chips");goto done;}
 fc_frame_us=ps_period_us(snes_pal(rom,size));
 printf("CARTRIDGE SNES region=%s period_us=%u bytes=%u\n",snes_pal(rom,size)?"PAL":"NTSC",fc_frame_us,size);
#endif
 fc_snapshot_init(rom,size);
 memcpy((void *)(base+NES_ROM),rom,size);io->push(base+NES_ROM,size,NULL);io->barrier(NULL);
 puts("STAGE ROM published; initializing core");fflush(stdout);
 if(!request(NES_INIT,size)){puts("SNES core rejected cartridge");goto done;}
#ifdef NES_PLAYER
 if(!player_load_save())goto done;
 if(fc_resume&&fc_state_transaction(1)!=1)goto done;
#endif
 puts("STAGE core initialized; calibrating clock");fflush(stdout);
 FreeVec(rom);rom=NULL;
 if(!request(NES_CLOCK,0))goto done;
 c0=armclock();t0=us();Delay(100);
 if(!request(NES_CLOCK,0))goto done;
 c1=armclock();t1=us();rate=(ULONG)((c1-c0)*1000000u/(t1-t0));
 if(rate<320000000u||rate>345000000u)goto done;
 if(!core_only){
 puts("STAGE opening display");fflush(stdout);
 GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",40);if(!GfxBase)goto done;
#ifdef NES_FULLSCREEN
 if(!full_open() || (!fc_windowed&&!full_measure_refresh()))goto done;
#else
 screen=LockPubScreen(NULL);if(!screen)goto done;
 truecolor=GetBitMapAttr(screen->RastPort.BitMap,BMA_DEPTH)>8;
#ifdef NES_DIRECT
 { ULONG format=p96GetBitMapAttr(screen->RastPort.BitMap,P96BMA_RGBFORMAT),depth=GetBitMapAttr(screen->RastPort.BitMap,BMA_DEPTH);
   if(format==RGBFB_R8G8B8)direct_format=ND_RGB24;
   else if(format==RGBFB_B8G8R8)direct_format=ND_BGR24;
   else if(format==RGBFB_B8G8R8A8)direct_format=ND_BGRA32;
   else if(format==RGBFB_A8R8G8B8)direct_format=ND_ARGB32;
   else {puts("DIRECT: unsupported Workbench format");goto done;}
   direct_native_format=format;
   direct_bitmap=p96AllocBitMap(256,240,depth,BMF_DISPLAYABLE,screen->RastPort.BitMap,format);
   if(!direct_bitmap)goto done;
   direct_readback=AllocVec(240*4096,MEMF_PUBLIC);if(!direct_readback)goto done;
   printf("DIRECT format=%u native_format=%u depth=%u\n",direct_format,format,depth);
 }
#else
 pixels=AllocVec(NES_PIXELS*(truecolor?4:1),MEMF_PUBLIC|MEMF_CLEAR);if(!pixels)goto done;
 if(truecolor){render.Memory=pixels;render.BytesPerRow=256*4;render.RGBFormat=RGBFB_A8R8G8B8;}
 else {
  for(i=0;i<64;i++){ULONG c=nes_palette[i];pens[i]=ObtainBestPen(screen->ViewPort.ColorMap,((c>>16)&255)*0x01010101u,((c>>8)&255)*0x01010101u,(c&255)*0x01010101u,TAG_DONE);if(pens[i]<0||pens[i]>255)goto done;}
  memset(pixels,pens[15],NES_PIXELS);
 }
#endif
 printf("DISPLAY bitmap_depth=%u path=%s\n",GetBitMapAttr(screen->RastPort.BitMap,BMA_DEPTH),
#ifdef NES_DIRECT
 "ARM to owned P96 bitmap; window blit"
#else
 truecolor?"Picasso96 ARGB":"indexed pens"
#endif
 );
 window=OpenWindowTags(NULL,WA_Title,(ULONG)"ZZSnes-Pocket ARM preview - arrows, Z/X, Enter; Q quits",WA_PubScreen,(ULONG)screen,
  WA_Left,80,WA_Top,70,WA_InnerWidth,256,WA_InnerHeight,240,
  WA_Flags,WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_ACTIVATE|WFLG_SIMPLE_REFRESH,
  WA_IDCMP,IDCMP_CLOSEWINDOW|IDCMP_RAWKEY|IDCMP_REFRESHWINDOW|IDCMP_INACTIVEWINDOW,TAG_DONE);
 if(!window)goto done;
#endif /* window/fullscreen */
 }
 indices=AllocVec(NES_OUTPUT_BYTES,MEMF_PUBLIC);if(!indices)goto done;
 printf("BUFFERS pixels=%08x indices=%08x legacy_host=%u\n",(ULONG)pixels,(ULONG)indices,
#ifdef NES_LEGACY_HOST
  1u
#else
  0u
#endif
 );
#ifdef NES_PLAYER
 fc_joy_open();
 if(!core_only && !player_audio_open())goto done;
 puts("PLAYER SCHEDULING paced + ARM LUT PocketSNES CPU/PPU/SPC/DSP, auto-region 32040Hz stereo; P pause, M mute, Q/Esc quit; bounded picture omission; every core/APU frame retained");
#endif
 printf("ZZSNES_POCKET START ROM=%s bytes=%u timer_hz=%u core_only=%u\n",rom_path?rom_path:"built-in original demo",size,rate,(ULONG)core_only);fflush(stdout);start=us();
 while(!quit && (!frame_limit||frames<frame_limit)){
#ifdef NES_PLAYER
  /* The previous flip can finish while Core1 emulates the next frame.
   * Poll ACK before touching Intuition; never process events under bitmap lock. */
  if(!full_poll())goto done;
  if(!direct_locked)events();
  if(em_switch_pending){if(!em_switch())goto done;player_audio_flush();if(player_paused){if(!present(ad_get(shared+MB_REPLY,24),frames)||!full_drain())goto done;paint();}}
  if(em_scanline_pending>=0){
   unsigned mode=(unsigned)em_scanline_pending;em_scanline_pending=-1;
   /* PRESENT has released the bitmap before changing the FPGA effect. */
   if(!full_drain())goto done;
   fc_scanlines=em_scan_normalize(mode);em_scanline_apply();
   printf("SCANLINES mode=%u window=%u\n",fc_scanlines,fc_windowed);fflush(stdout);
   if(player_paused){if(!present(ad_get(shared+MB_REPLY,24),frames)||!full_drain())goto done;paint();}
  }
  if(fc_state_action){int action=fc_state_action;fc_state_action=0;
   if(!full_drain()||fc_state_transaction(action==2)<0)goto done;}
  if(player_paused){
   next_frame_us=0;fc_last_flip=0;fc_joy_buttons=0;
   player_audio_flush();
   before=us();if(!full_drain())goto done;flip_drain_us+=us()-before;
   events();if(quit)break;
   if(fc_snapshot_action&&direct_ready){if(!present(ad_get(shared+MB_REPLY,24),frames))goto done;}
   if(player_paused){if(!pause_us(10000))goto done;continue;}
  }
#else
  events();
#endif
  if(quit)break;
  fc_profile_frame=frames;
  frame_start=us();if(!next_frame_us)cadence.valid=0;next_frame_us=ps_cadence_next(&cadence,frame_start,fc_frame_us);before=frame_start;{ UWORD input=buttons|fc_joy_buttons;if(fc_scripted_input)input=((frames>=360&&frames<368)||(frames>=480&&frames<488))?8:0;static UWORD last;
  if(input!=last){fc_input_changes++;}last=input;
  if(!request(NES_FRAME,input | (verify_frame(frames+1)?65536u:0u))){goto done;}request_us+=us()-before;fc_sample(0,us()-before);}
#ifdef NES_PLAYER
  if(quit){rc=0;goto done;}
  /* AHI may process input while waiting: release any outstanding bitmap lock first. */
  before=us();if(!full_drain())goto done;flip_drain_us+=us()-before;fc_sample(1,us()-before);fc_read_buttons();
  before=us();if(!core_only&&!player_audio_frame())goto done;audio_us+=us()-before;fc_sample(2,us()-before);
  if(quit){rc=0;goto done;}
#endif
  frame_core_us=(ULONG)((uint64_t)ad_get(base+MB_REPLY,28)*1000000u/rate);
#ifdef NES_DIRECT
  output_hash=ad_get(base+MB_REPLY,32);
  if(!core_only && ps_cadence_show(&cadence,us(),fc_frame_us)){
   before=us();if(!present(ad_get(base+MB_REPLY,24),frames+1))goto done;draw_us+=us()-before;fc_sample(3,us()-before);
  }
#else
  before=us();io->pull(base+NES_OUTPUT,NES_PIXELS,NULL);
#ifndef NES_LEGACY_HOST
  /* One bulk bus read, then validation/conversion on the host copy. */
  CopyMem((APTR)(base+NES_OUTPUT),indices,NES_PIXELS);
#endif
  copy_us+=us()-before;before=us();output_hash=2166136261u;
  for(i=0;i<NES_PIXELS;i++){
#ifdef NES_LEGACY_HOST
   UBYTE v=base[NES_OUTPUT+i];
#else
   UBYTE v=indices[i];
#endif
   if(v>=64){puts("Invalid frame index");goto done;}
   output_hash=(output_hash^v)*16777619u;
   if(!core_only){if(truecolor)((ULONG *)pixels)[i]=nes_palette[v];else pixels[i]=pens[v];}
  }
  if(output_hash!=ad_get(base+MB_REPLY,32)){puts("FRAME PUBLICATION FAIL");goto done;}
  colour_us+=us()-before;
#endif
  before=us();paint();draw_us+=us()-before;
  frames++;total_core_us+=frame_core_us;
  if((frame_limit && frame_limit<=10)||frames==1||frames%600==0){printf("FRAME n=%u cycles=%u core_hash=%08x output_hash=%08x cpu_ppu_us=%u\n",frames,
   (ULONG)ad_get(base+MB_REPLY,20),(ULONG)ad_get(base+MB_REPLY,24),output_hash,frame_core_us);fflush(stdout);}
  /* Bound the emulation rate without busy spinning. Reset a late deadline
   * to avoid a burst of catch-up frames after a window drag or task switch. */
  { ULONG elapsed=us()-frame_start,target=ps_pace_target(frame_start,cadence.deadline,fc_frame_us);
    while(elapsed<target){
     if(!full_poll())goto done;
     if(!direct_locked){events();if(quit)break;}
     if(!direct_locked && target-elapsed>4000u && !profile_pause(4,1000))goto done;
     elapsed=us()-frame_start;
    }
  }
  fc_sample(4,us()-frame_start);
  #ifndef NES_PLAYER
  if((ULONG)(us()-start)>300000000u)quit=1; // bounded preview session
  #endif
 }
 printf("REGION_PACING frames=%u play_us=%u period_us=%u late_rebases=%u\n",frames,(ULONG)(us()-start),fc_frame_us,cadence.rebases);
 rc=0;
done:
#ifdef NES_FULLSCREEN
 if(!full_drain())rc=20;
 printf("FULLSCREEN_END queued=%u flips=%u pending=%d ack_wait_us=%u fault=%u\n",
  direct_challenges,full_slots.flips,full_slots.pending,full_wait_us,(ULONG)full_fault);
#endif
#ifdef NES_PLAYER
 if(rc==0&&save_data){int old=quit;quit=0;if(!player_store_save()){puts("SAVE failed; previous file retained; check .sav/.tmp/.bak");rc=20;}quit=old;}
 if(rc==0&&!player_audio_submit())rc=20; /* Check worker status before draining audio. */
 player_audio_close();if(audio_error)rc=20;if(save_data){FreeVec(save_data);save_data=NULL;}
 printf("AUDIO_END samples=%u gaps=%u\n",audio_samples,audio_gaps);
#endif
 printf("PRESENT_OPTIONS window=%u scanlines=%u fps=%u optimised=%u neon_frames=%u\n",fc_windowed,fc_scanlines,fc_fps,fc_optimise,em_neon_frames);
 puts("STAGE closing frontend; requesting ARM return next");fflush(stdout);
 #ifndef NES_PLAYER
 if(quit)rc=0;
 #endif
#ifdef NES_FULLSCREEN
 if(full_fault)rc=20;
#endif
 if(start)printf("ZZSNES_POCKET END exit=%d frames=%u elapsed_us=%u core_total_us=%u output_hash=%08x\n",rc,frames,(ULONG)(us()-start),total_core_us,output_hash);
#ifdef NES_DIRECT
 printf("DIRECT_END challenges=%u verified_frames=%u copy_us=%u\n",direct_challenges,direct_verified,rate?(ULONG)((uint64_t)direct_ticks*1000000u/rate):0);
#endif
#ifdef NES_PLAYER
 printf("PLAYER_PROFILE audio_prepare_us=%u flip_drain_us=%u audio_wait_us=%u audio_copy_us=%u audio_send_us=%u\n",audio_us,flip_drain_us,audio_wait_us,audio_copy_us,audio_send_us);
#endif
 printf("ARM_REQUEST_PROFILE frame_us=%u present_us=%u\n",rate?(ULONG)(frame_reply_ticks*1000000u/rate):0,rate?(ULONG)(present_reply_ticks*1000000u/rate):0);
 printf("PRESENT_PROFILE pre_us=%u request_us=%u post_us=%u\n",present_pre_us,present_request_us,present_post_us);
 printf("TEARING_TRIAL variant=C0 native=0 slots=4 ack_blank=%u ack_active=%u\n",trial_ack_blank,trial_ack_active);
 printf("SCANOUT_GATE variant=C0 mode=0 delayed=%u total_us=%u max_us=%u timeouts=%u phase=%u\n",gate_count,gate_total_us,gate_max_us,gate_timeouts,full_gate.phase);
 fc_profile_print();fc_joy_close();
 print_wait_profile();
 printf("PROFILE frames=%u request_us=%u copy_us=%u colour_us=%u draw_us=%u waits=%u\n",frames,request_us,copy_us,colour_us,draw_us,waits);
 if(indices){FreeVec(indices);indices=NULL;}
#ifndef NES_FULLSCREEN
 if(window){CloseWindow(window);window=NULL;}
#endif
 if(screen){for(i=0;i<64;i++)if(pens[i]>=0)ReleasePen(screen->ViewPort.ColorMap,pens[i]);UnlockPubScreen(NULL,screen);}
 if(pixels){FreeVec(pixels);pixels=NULL;}
#ifndef NES_FULLSCREEN
 if(GfxBase){CloseLibrary((struct Library *)GfxBase);GfxBase=NULL;}
#endif
 if(rom)FreeVec(rom);if(opened)CloseDevice((struct IORequest *)timer);if(timer)DeleteIORequest((struct IORequest *)timer);if(port)DeleteMsgPort(port);
 return rc;
}

/* Invoked only after RET1 and synchronous idle reset, before FreeMem. */
int mb_after_stop(volatile uint8_t *base,const struct ad_io *io) {
    ULONG i,bad=0,phase,registers,mask;
    io->pull(base+MB_MMU_REPORT,160,NULL);
    phase=ad_get(base,MB_MMU_REPORT);registers=ad_get(base,MB_MMU_REPORT+4);
    mask=ad_get(base,MB_MMU_REPORT+136);
    io->pull(base+MB_WITNESS-32,1088,NULL);
    for(i=0;i<256;i++)if(ad_get(base,MB_WITNESS+i*4)!=(ad_get(base,ZZ_CONTROL)^i*0x01010101u))bad++;
    for(i=0;i<32;i+=4)if(ad_get(base,MB_WITNESS-32+i) || ad_get(base,MB_WITNESS+1024+i))bad++;
    printf("MMU_RESTORE phase=%u register_mismatches=%08x cpsr_control=%03x expected=%03x ttbr=%08x ccsidr=%08x witness_bad=%u mailbox_rounds=%u\n",
        phase,registers,mask,ad_get(base,MB_MMU_REPORT+16+6*4)&511u,
        ad_get(base,MB_MMU_REPORT+8),ad_get(base,MB_MMU_REPORT+12),bad,sequence);
    for(i=0;i<14;i++)printf("REGISTER n=%u original=%08x restored=%08x\n",i,
        ad_get(base,MB_MMU_REPORT+16+i*4),ad_get(base,MB_MMU_REPORT+80+i*4));
    return phase==3 && !registers && !bad && mask==(ad_get(base,MB_MMU_REPORT+16+6*4)&511u);
}
