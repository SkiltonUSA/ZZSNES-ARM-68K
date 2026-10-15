/* GPL-3.0-or-later. Independent implementation of the XX19c deferred-PAN
 * handoff demonstrated by Xanai's ZZPicoDriveMD. See zzpicodrive-review.md.
 * Included by host.c after its timer helpers. No PicoDrive source reused. */
#include "flip_plan.h"
static struct Screen *full_screen;
static struct BitMap *full_allocation;
static volatile UBYTE *full_board;
static struct nf_slots full_slots={0,1,-1,0,0}; /* Core init precedes screen open. */
static ULONG full_pitch,full_origin,full_pending_us,full_wait_us;
static UWORD *full_pointer;
static int full_fault,full_attached,full_usedpan,full_origin_known;
static void full_write(unsigned reg,UWORD v){*(volatile UWORD *)(full_board+reg)=v;}
static UWORD full_ack_count(void){return *(volatile UWORD *)(full_board+0x46);}
static int full_front(void){return full_screen && IntuitionBase->FirstScreen==full_screen;}
static void full_pan(ULONG offset,int deferred){
 /* Keep unrelated graphics tasks out of this short register transaction.
  * Bitmap lock prevents screen switches while a deferred flip is pending. */
 Forbid();
 full_write(0x44,deferred?1:2);
 full_write(0x10,0);full_write(0x12,0);
 full_write(0x14,(UWORD)(full_pitch/4));full_write(0x30,2);
 full_write(0x0a,(UWORD)(offset>>16));full_write(0x0c,(UWORD)offset);
 Permit();
}
static void full_unlock(void){
 if(direct_locked){p96UnlockBitMap(direct_bitmap,direct_lock);direct_locked=0;}
}
static int full_poll(void){
 int status;
 if(full_slots.pending<0)return !full_fault;
 status=nf_ack(&full_slots,full_ack_count());
 if(status==1){full_wait_us+=us()-full_pending_us;full_unlock();return 1;}
 if(status<0 || (ULONG)(us()-full_pending_us)>100000u){
  /* Cancellation clears pending before restoring slot zero. Lock is still
   * held: the screen and origin cannot change underneath this transaction. */
  full_pan(full_origin,0);full_slots.pending=-1;full_fault=1;full_unlock();
  puts("FULLSCREEN flip ACK failed; pending cancelled");return 0;
 }
 return 1;
}
static int full_drain(void){
 while(full_slots.pending>=0){if(!full_poll())return 0;if(!pause_us(1000)){full_fault=1;return 0;}}
 return !full_fault;
}
static int full_queue(void){
 if(!direct_locked || !full_attached || !nf_queue(&full_slots,full_ack_count()))return 0;
 full_pending_us=us();
 full_usedpan=1;
 full_pan(full_origin+(ULONG)full_slots.pending*240*full_pitch,1);
 return 1;
}
static int full_open(void){
 struct ConfigDev *card=FindConfigDev(NULL,0x6d6e,4);
 ULONG mode,format,depth;
 if(!card)return 0;
 full_board=card->cd_BoardAddr;nf_init(&full_slots);
 mode=p96BestModeIDTags(P96BIDTAG_NominalWidth,320,P96BIDTAG_NominalHeight,240,
   P96BIDTAG_Depth,32,TAG_DONE);
 if(mode==0xffffffffu || p96GetModeIDAttr(mode,P96IDA_WIDTH)!=320 ||
    p96GetModeIDAttr(mode,P96IDA_HEIGHT)!=240){puts("FULLSCREEN requires an existing 320x240 true-colour RTG mode");return 0;}
 format=p96GetModeIDAttr(mode,P96IDA_RGBFORMAT);depth=p96GetModeIDAttr(mode,P96IDA_DEPTH);
 if(format==RGBFB_B8G8R8A8)direct_format=ND_BGRA32;
 else if(format==RGBFB_A8R8G8B8)direct_format=ND_ARGB32;
 else {printf("FULLSCREEN unsupported mode pixel format=%u\n",format);return 0;}
 direct_native_format=format;
 /* All slots belong to the actual screen bitmap. P96 sees the entire
  * scanout allocation, unlike independently allocated, untracked backbuffers. */
 direct_bitmap=p96AllocBitMap(320,720,depth,BMF_DISPLAYABLE|BMF_CLEAR,NULL,format);
 if(!direct_bitmap)return 0;
 full_allocation=direct_bitmap;
 full_screen=OpenScreenTags(NULL,SA_DisplayID,mode,SA_Width,320,SA_Height,240,
  SA_Depth,depth,SA_BitMap,(ULONG)direct_bitmap,SA_AutoScroll,FALSE,SA_Quiet,TRUE,
  SA_ShowTitle,FALSE,SA_Type,CUSTOMSCREEN,SA_Title,(ULONG)"ZZNes fullscreen",TAG_DONE);
 if(!full_screen){puts("FULLSCREEN custom screen open failed");return 0;}
 printf("FULLSCREEN_BITMAP allocated=%08x screen=%08x width=%u height=%u allocated_height=%u screen_height=%u on_board=%u/%u memory=%08x/%08x pitch=%u/%u\n",
  (ULONG)direct_bitmap,(ULONG)full_screen->RastPort.BitMap,(ULONG)full_screen->Width,(ULONG)full_screen->Height,
  p96GetBitMapAttr(direct_bitmap,P96BMA_HEIGHT),p96GetBitMapAttr(full_screen->RastPort.BitMap,P96BMA_HEIGHT),
  p96GetBitMapAttr(direct_bitmap,P96BMA_ISONBOARD),p96GetBitMapAttr(full_screen->RastPort.BitMap,P96BMA_ISONBOARD),
  p96GetBitMapAttr(direct_bitmap,P96BMA_MEMORY),p96GetBitMapAttr(full_screen->RastPort.BitMap,P96BMA_MEMORY),
  p96GetBitMapAttr(direct_bitmap,P96BMA_BYTESPERROW),p96GetBitMapAttr(full_screen->RastPort.BitMap,P96BMA_BYTESPERROW));
 /* Intuition/P96 may clone the supplied BitMap and place the screen copy in
  * VRAM. Use the real screen bitmap; retain the caller allocation for cleanup. */
 direct_bitmap=full_screen->RastPort.BitMap;
 if(full_screen->Width!=320 || full_screen->Height!=240 ||
    !p96GetBitMapAttr(direct_bitmap,P96BMA_ISONBOARD) ||
    p96GetBitMapAttr(direct_bitmap,P96BMA_WIDTH)<320 ||
    p96GetBitMapAttr(direct_bitmap,P96BMA_HEIGHT)<720 ||
    p96GetBitMapAttr(direct_bitmap,P96BMA_RGBFORMAT)!=format){puts("FULLSCREEN screen bitmap validation failed");return 0;}
 {
  struct RenderInfo ri;ULONG memory,span;
  memset(&ri,0,sizeof(ri));direct_lock=p96LockBitMap(direct_bitmap,(UBYTE *)&ri,sizeof(ri));direct_locked=1;
  memory=(ULONG)ri.Memory;span=720*(ULONG)ri.BytesPerRow;
  if(ri.BytesPerRow<1280 || ri.BytesPerRow>4096 || (ri.BytesPerRow&3) ||
     memory<(ULONG)full_board+0x10000u || memory>(ULONG)full_board+0x4000000u-span)return 0;
  memset(ri.Memory,0,span);CacheClearE(ri.Memory,span,CACRF_ClearD);
  full_unlock();
 }
 window=OpenWindowTags(NULL,WA_CustomScreen,(ULONG)full_screen,WA_Left,0,WA_Top,0,
  WA_Width,320,WA_Height,240,WA_Title,(ULONG)"ZZNes fullscreen - Q/Esc quits",
  WA_Flags,WFLG_BACKDROP|WFLG_BORDERLESS|WFLG_RMBTRAP|WFLG_ACTIVATE,
  WA_IDCMP,IDCMP_RAWKEY|IDCMP_INACTIVEWINDOW|IDCMP_MOUSEBUTTONS,TAG_DONE);
 if(!window)return 0;
 full_pointer=AllocMem(12,MEMF_CHIP|MEMF_CLEAR);
 if(full_pointer)SetPointer(window,full_pointer,1,16,0,0);
 direct_readback=AllocVec(240*4096,MEMF_PUBLIC);if(!direct_readback)return 0;
 full_attached=1;
 printf("FULLSCREEN mode=%08x native_format=%u depth=%u slots=3 geometry=320x240 NES=256x240\n",mode,format,depth);
 return 1;
}
/* Called only after synchronous Core1 reset, before bitmap/library release. */
static void full_cleanup(void){
 if(full_usedpan && full_screen){
  struct RenderInfo ri;
  /* Cancel even if another screen is now foremost. Never pan over Workbench. */
  full_write(0x44,2);
  if(!direct_locked){memset(&ri,0,sizeof(ri));full_origin_known=0;
   direct_lock=p96LockBitMap(direct_bitmap,(UBYTE *)&ri,sizeof(ri));direct_locked=1;
   if(ri.BytesPerRow>=320*4 && ri.BytesPerRow<=4096 &&
      (ULONG)ri.Memory>=(ULONG)full_board+0x10000u &&
      (ULONG)ri.Memory<=(ULONG)full_board+0x4000000u-720*(ULONG)ri.BytesPerRow){
    full_origin=(ULONG)ri.Memory-(ULONG)full_board-0x10000u;full_pitch=ri.BytesPerRow;full_origin_known=1;}}
  if(full_front() && full_origin_known)full_pan(full_origin,0);
 }
 full_slots.pending=-1;full_unlock();
 if(window){if(full_pointer)ClearPointer(window);CloseWindow(window);window=NULL;}
 if(full_pointer){FreeMem(full_pointer,12);full_pointer=NULL;}
 if(full_screen){CloseScreen(full_screen);full_screen=NULL;}
 /* The screen copy belongs to Intuition. Free only our supplied bitmap. */
 direct_bitmap=full_allocation;full_allocation=NULL;
 if(GfxBase){CloseLibrary((struct Library *)GfxBase);GfxBase=NULL;}
 full_attached=0;
}
