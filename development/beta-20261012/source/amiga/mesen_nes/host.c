/* GPL-3.0-or-later. 68k Workbench preview; optional ARM/P96 source bitmap. */
#include <exec/memory.h>
#include <devices/timer.h>
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
#include "mmu.h"
#include "palette.h"
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
#endif
/* A failed presentation must reach synchronous reset promptly while locked. */
unsigned nes_direct_stop_ticks(void){return direct_locked?2u:100u;}
void nes_direct_cleanup(void){
#ifdef NES_FULLSCREEN
 full_cleanup();
#endif
 if(direct_locked){p96UnlockBitMap(direct_bitmap,direct_lock);direct_locked=0;}
 if(direct_bitmap){p96FreeBitMap(direct_bitmap);direct_bitmap=NULL;}
 if(direct_readback){FreeVec(direct_readback);direct_readback=NULL;}
 direct_ready=0;
}
#endif
struct Device *TimerBase;
struct GfxBase *GfxBase;
static volatile uint8_t *shared;
static const struct ad_io *syncio;
static uint32_t sequence;
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
#ifndef NES_DIRECT
static struct RenderInfo render;
#endif
static int quit;
#ifdef NES_PLAYER
static int player_demo,player_paused,player_muted;
#endif
static UBYTE buttons;
static ULONG us(void){struct timeval t;GetSysTime(&t);return t.tv_secs*1000000u+t.tv_micro;}
int nes_arguments(int argc,char **argv){
#ifdef NES_PLAYER
 if(argc==2){rom_path=argv[1];return 1;}
 player_demo=argc>=4;
#endif
 if(argc>5){puts("Usage: ZZNes <nonce> RUN [ROM.nes|DEMO|CORE] [frame_limit]");return -1;}
 if(argc>=4 && !strcmp(argv[3],"CORE")){core_only=1;frame_limit=6;}
 else if(argc>=4 && strcmp(argv[3],"DEMO"))rom_path=argv[3];
 if(argc==5){char *end;errno=0;frame_limit=strtoul(argv[4],&end,10);if(errno||*end||!frame_limit||frame_limit>36000)return -1;}
 return argc>3?3:argc;
}
static void paint(void){
#ifdef NES_FULLSCREEN
 return; /* Firmware scanout consumes the completed slot; no pixel blit. */
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
 while((msg=(struct IntuiMessage *)GetMsg(window->UserPort))){ULONG cl=msg->Class;UWORD code=msg->Code;ReplyMsg((struct Message *)msg);
  if(cl==IDCMP_CLOSEWINDOW)quit=1;
  else if(cl==IDCMP_INACTIVEWINDOW){buttons=0;
#ifdef NES_FULLSCREEN
   quit=1;
#endif
  }
#ifdef NES_FULLSCREEN
  else if(cl==IDCMP_MOUSEBUTTONS)quit=1;
#endif
  else if(cl==IDCMP_REFRESHWINDOW){BeginRefresh(window);paint();EndRefresh(window,TRUE);}
  else if(cl==IDCMP_RAWKEY){unsigned key=code&127,bit=0;
   if(key==0x45||key==0x10){if(!(code&128))quit=1;}
#ifdef NES_PLAYER
   else if(key==0x19){if(!(code&128)){player_paused=!player_paused;buttons=0;}}
   else if(key==0x37){if(!(code&128))player_muted=!player_muted;}
#endif
   else {switch(key){case 0x31:bit=1;break;case 0x32:bit=2;break;case 0x60:bit=4;break;case 0x44:bit=8;break;
    case 0x4c:bit=16;break;case 0x4d:bit=32;break;case 0x4f:bit=64;break;case 0x4e:bit=128;break;}
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
#include "fullscreen.h"
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
  if(!pause_us(1000))return 0;
#endif
 }while((ULONG)(us()-start)<(op==NES_PRESENT?50000u:10000000u));
 puts("NES request timeout");return 0;
}
#ifdef NES_PLAYER
#include "player_frontend.h"
#endif
#ifdef NES_DIRECT
static int present(ULONG raw_hash,ULONG frame){
 struct RenderInfo ri;struct ConfigDev *card=FindConfigDev(NULL,0x6d6e,4);
 ULONG span,addr,nonce,i,x,y,pitch,bpp=nd_bpp(direct_format),bad=0;
 int verify=frame==1 || frame%60==0;
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
 /* Check the complete 3-slot scanout allocation, then target the free slot.
  * 32-pixel black borders centre the unscaled 256-pixel NES in 320 pixels. */
 if(pitch<320*4 || (ULONG)ri.Memory<(ULONG)card->cd_BoardAddr+0x10000u ||
    (ULONG)ri.Memory>(ULONG)card->cd_BoardAddr+0x4000000u-720*pitch)return 0;
 full_origin=(ULONG)ri.Memory-(ULONG)card->cd_BoardAddr-0x10000u;full_pitch=pitch;
 full_origin_known=1;
 ri.Memory=(UBYTE *)ri.Memory+full_slots.render*240*pitch+32*4;
#endif
 span=239*ri.BytesPerRow+256*bpp;
 if((ULONG)ri.Memory<(ULONG)card->cd_BoardAddr+0x10000u||
    (ULONG)ri.Memory>(ULONG)card->cd_BoardAddr+0x4000000u-span)return 0;
 addr=(ULONG)ri.Memory-(ULONG)card->cd_BoardAddr+0x1f0000u;
 if(nd_span(addr,ri.BytesPerRow,direct_format)!=span)return 0;
 nonce=ad_get(shared,ZZ_CONTROL)^(sequence+1)^0x97ab301du;
 ad_put(ri.Memory,0,nonce);ad_put(ri.Memory,span-4,nonce^0x31415926u);
 syncio->push(ri.Memory,4,NULL);syncio->push((uint8_t *)ri.Memory+span-4,4,NULL);
 ad_put(shared+MB_HOST,16,ri.BytesPerRow);ad_put(shared+MB_HOST,20,direct_format);ad_put(shared+MB_HOST,24,nonce);
 if(!request(NES_PRESENT,addr))return 0; /* Keep locked on error until Core1 reset. */
 if(ad_get(shared+MB_REPLY,24)!=raw_hash)return 0;
 direct_ticks+=ad_get(shared+MB_REPLY,28);direct_challenges++;
 /* Explicit diagnostic readback on frame 1 and each 60th. Never read live
  * Workbench storage: only our locked off-screen source after ARM ACK. */
 if(verify){
  syncio->pull(ri.Memory,span,NULL);
  CopyMem(ri.Memory,direct_readback,span);
 }
#ifdef NES_FULLSCREEN
 if(!full_queue())return 0;
 /* Usual frames stay locked through ACK; the next CPU/PPU request overlaps
  * the vblank wait. Diagnostic frames drain before comparing the snapshot. */
 if(verify && !full_drain())return 0;
#else
 p96UnlockBitMap(direct_bitmap,direct_lock);direct_locked=0;
#endif
 /* Validate the snapshot after unlocking; no slow per-byte Zorro reads
  * or pixel comparisons while holding the relocation lock. */
 if(verify){
  syncio->pull(shared+NES_OUTPUT,NES_OUTPUT_BYTES,NULL);CopyMem((APTR)(shared+NES_OUTPUT),indices,NES_OUTPUT_BYTES);
  for(y=0;y<240;y++)for(x=0;x<256;x++){
   UBYTE *pixel=direct_readback+y*pitch+x*bpp;
   ULONG c;UBYTE want[4];
#ifdef NES_PLAYER
   {unsigned index=((unsigned)indices[2*(y*256+x)]<<8)|indices[2*(y*256+x)+1];if(index>=512){bad++;continue;}c=nes_player_palette[index];}
#else
   if(indices[y*256+x]>=64){bad++;continue;}
   c=nes_palette[indices[y*256+x]];
#endif
   if(direct_format==ND_RGB24){want[0]=c>>16;want[1]=c>>8;want[2]=c;}
   else if(direct_format==ND_BGR24){want[0]=c;want[1]=c>>8;want[2]=c>>16;}
   else if(direct_format==ND_BGRA32){want[0]=c;want[1]=c>>8;want[2]=c>>16;want[3]=255;}
   else {want[0]=255;want[1]=c>>16;want[2]=c>>8;want[3]=c;}
   for(i=0;i<bpp;i++)if(pixel[i]!=want[i])bad++;
  }
  if(bad){printf("DIRECT READBACK FAIL frame=%u bytes=%u\n",frame,bad);return 0;}
  direct_verified++;
 }
 direct_ready=1;
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
 rom=AllocVec(NES_ROM_LIMIT+1,MEMF_PUBLIC);if(!rom)goto done;
 if(rom_path){FILE *f=fopen(rom_path,"rb");if(!f){puts("Cannot open ROM");goto done;}size=fread(rom,1,NES_ROM_LIMIT+1,f);fclose(f);}
 else memcpy(rom,demo_rom,size);
 if(size>NES_ROM_LIMIT){puts("ROM exceeds player cartridge limit");goto done;}
#ifdef NES_PLAYER
 {int id=nes_cartridge(rom,size);if(id<0){puts("Unsupported cartridge: require clean NTSC iNES, mapper 0/1/2/3/4/7/90; see ReadMe");goto done;}printf("CARTRIDGE mapper=%d PRG=%u CHR=%u battery=%u\n",id,rom[4]*16384u,rom[5]*8192u,(ULONG)!!(rom[6]&2));}
#endif
 memcpy((void *)(base+NES_ROM),rom,size);io->push(base+NES_ROM,size,NULL);io->barrier(NULL);
 puts("STAGE ROM published; initializing core");fflush(stdout);
 if(!request(NES_INIT,size)){puts("Only NTSC iNES NROM, 16/32 KiB PRG, 0/8 KiB CHR; no trainer/battery supported");goto done;}
#ifdef NES_PLAYER
 if(!player_load_save())goto done;
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
 if(!full_open())goto done;
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
 window=OpenWindowTags(NULL,WA_Title,(ULONG)"ZZNes ARM preview - arrows, Z/X, Enter; Q quits",WA_PubScreen,(ULONG)screen,
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
 if(!core_only)player_audio_open();
 puts("PLAYER Mesen CPU/PPU/APU, NTSC; P pause, M mute, Q/Esc quit; no timing hacks");
#endif
 printf("ZZNES START ROM=%s bytes=%u timer_hz=%u core_only=%u\n",rom_path?rom_path:"built-in original demo",size,rate,(ULONG)core_only);fflush(stdout);start=us();
 while(!quit && (!frame_limit||frames<frame_limit)){
#ifdef NES_PLAYER
  /* The previous flip can finish while Core1 emulates the next frame.
   * Poll ACK before touching Intuition; never process events under bitmap lock. */
  if(!full_poll())goto done;
  if(!direct_locked)events();
  if(player_paused){
   player_audio_flush();
   before=us();if(!full_drain())goto done;flip_drain_us+=us()-before;
   events();if(quit)break;
   if(player_paused){if(!pause_us(10000))goto done;continue;}
  }
#else
  events();
#endif
  if(quit)break;
  frame_start=us();before=frame_start;if(!request(NES_FRAME,buttons))goto done;request_us+=us()-before;
#ifdef NES_PLAYER
  if(quit){rc=0;goto done;}
  /* AHI may process input while waiting: release any outstanding bitmap lock first. */
  before=us();if(!full_drain())goto done;flip_drain_us+=us()-before;
  before=us();if(!core_only&&!player_audio_frame())goto done;audio_us+=us()-before;
  if(quit){rc=0;goto done;}
#endif
  frame_core_us=(ULONG)((uint64_t)ad_get(base+MB_REPLY,28)*1000000u/rate);
#ifdef NES_DIRECT
  output_hash=ad_get(base+MB_REPLY,32);
  if(!core_only){
   before=us();if(!present(ad_get(base+MB_REPLY,24),frames+1))goto done;draw_us+=us()-before;
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
  if((frame_limit && frame_limit<=10)||frames==1||frames%60==0){printf("FRAME n=%u cycles=%u core_hash=%08x output_hash=%08x cpu_ppu_us=%u\n",frames,
   (ULONG)ad_get(base+MB_REPLY,20),(ULONG)ad_get(base+MB_REPLY,24),output_hash,frame_core_us);fflush(stdout);}
  /* Bound the emulation rate without busy spinning. Reset a late deadline
   * to avoid a burst of catch-up frames after a window drag or task switch. */
  { ULONG elapsed=us()-frame_start,target=16639u;
    if(elapsed<target && !pause_us(target-elapsed))goto done;
  }
  #ifndef NES_PLAYER
  if((ULONG)(us()-start)>300000000u)quit=1; // bounded preview session
  #endif
 }
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
 puts("STAGE closing frontend; requesting ARM return next");fflush(stdout);
 #ifndef NES_PLAYER
 if(quit)rc=0;
 #endif
#ifdef NES_FULLSCREEN
 if(full_fault)rc=20;
#endif
 if(start)printf("ZZNES END exit=%d frames=%u elapsed_us=%u core_total_us=%u output_hash=%08x\n",rc,frames,(ULONG)(us()-start),total_core_us,output_hash);
#ifdef NES_DIRECT
 printf("DIRECT_END challenges=%u verified_frames=%u copy_us=%u\n",direct_challenges,direct_verified,rate?(ULONG)((uint64_t)direct_ticks*1000000u/rate):0);
#endif
#ifdef NES_PLAYER
 printf("PLAYER_PROFILE audio_prepare_us=%u flip_drain_us=%u audio_wait_us=%u audio_copy_us=%u audio_send_us=%u\n",audio_us,flip_drain_us,audio_wait_us,audio_copy_us,audio_send_us);
#endif
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
