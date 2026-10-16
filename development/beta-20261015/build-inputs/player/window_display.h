/* GPL-3.0-or-later. RTG-owned off-screen bitmap, normal Workbench blit.
 * ARM never maps the Workbench bitmap or writes overlapping windows. */
static struct Screen*em_public_screen;
static int em_window_open(void){
 struct ConfigDev*card=FindConfigDev(NULL,0x6d6e,4);if(!card)return 0;
 full_board=card->cd_BoardAddr;nf_init(&full_slots);full_fault=0;
 em_public_screen=LockPubScreen(NULL);if(!em_public_screen)return 0;
 ULONG format=p96GetBitMapAttr(em_public_screen->RastPort.BitMap,P96BMA_RGBFORMAT);
 /* A 32-bit private source is blitted by P96 to the public screen's format. */
 if(format!=RGBFB_B8G8R8A8&&format!=RGBFB_A8R8G8B8)format=RGBFB_B8G8R8A8;
 direct_format=format==RGBFB_B8G8R8A8?ND_BGRA32:ND_ARGB32;direct_native_format=format;
 direct_bitmap=p96AllocBitMap(320,960,32,BMF_DISPLAYABLE|BMF_CLEAR,em_public_screen->RastPort.BitMap,format);
 full_allocation=direct_bitmap;
 if(!direct_bitmap||!p96GetBitMapAttr(direct_bitmap,P96BMA_ISONBOARD))return 0;
 window=OpenWindowTags(NULL,WA_Title,(ULONG)"ZZSNES" " - Alt+Return fullscreen",WA_PubScreen,(ULONG)em_public_screen,
 WA_Left,100,WA_Top,70,WA_InnerWidth,320,WA_InnerHeight,240,
 WA_Flags,WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_ACTIVATE|WFLG_SIMPLE_REFRESH,
 WA_IDCMP,IDCMP_CLOSEWINDOW|IDCMP_RAWKEY|IDCMP_REFRESHWINDOW|IDCMP_INACTIVEWINDOW,TAG_DONE);
 if(!window)return 0;
 direct_readback=AllocVec(240*4096,MEMF_PUBLIC);if(!direct_readback)return 0;
 full_attached=1;return 1;
}
static int full_open(void){return fc_windowed?em_window_open():em_fullscreen_open();}
static void em_display_close(void){
 full_cleanup();
 if(direct_bitmap){p96FreeBitMap(direct_bitmap);direct_bitmap=NULL;}
 if(direct_readback){FreeVec(direct_readback);direct_readback=NULL;}
 if(em_public_screen){UnlockPubScreen(NULL,em_public_screen);em_public_screen=NULL;}
 direct_ready=0;full_usedpan=full_origin_known=full_fault=0;
}
static int em_switch(void){
 if(!full_drain())return 0;
 /* PRESENT has acknowledged and unmapped before this safe-boundary switch. */
 em_display_close();fc_windowed=!fc_windowed;em_switch_pending=0;
 next_frame_us=0;fc_last_flip=0;return full_open();
}
