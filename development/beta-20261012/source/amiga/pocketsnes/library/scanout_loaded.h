/* GPL-3.0-or-later. Immutable display slots with the normal FRAME/audio loop.
 * Included after scanout_diagnostic.h. No ARM presentation requests or pixel
 * writes after lp_init. Existing C0 PAN/ACK and teardown remain unchanged. */
static ULONG lp_before,lp_frames,lp_present_requests,lp_flips;
static int lp_ready;
static int lp_init(void){
 struct RenderInfo ri;
 if(!sp_lock(&ri))return 0;
 sp_fill(ri.Memory,ri.BytesPerRow,direct_format==ND_ARGB32);
 CacheClearE(ri.Memory,960*(ULONG)ri.BytesPerRow,CACRF_ClearD);
 lp_before=sp_hash(ri.Memory,ri.BytesPerRow);
 if(!full_queue() || !full_drain())return 0;
 lp_ready=1;return 1;
}
static int lp_present(void){
 struct RenderInfo ri;
 if(!lp_ready || !full_drain())return 0;
 if(!full_front()){quit=1;return 0;}
 if(!sp_lock(&ri) || !full_queue())return 0;
 lp_flips++;
 /* Like the player, retain the bitmap lock until PAN ACK. The next FRAME
  * request can run while waiting; it never writes these display slots. */
 return 1;
}
static int lp_finish(void){
 struct RenderInfo ri;ULONG after;
 if(!lp_ready)return 1;
 if(!full_front()){puts("LOAD_PATTERN integrity skipped: foreground lost");return 1;}
 if(!sp_lock(&ri))return 0;
 CacheClearE(ri.Memory,960*(ULONG)ri.BytesPerRow,CACRF_ClearD);
 after=sp_hash(ri.Memory,ri.BytesPerRow);full_unlock();
 printf("LOAD_PATTERN_END frames=%u flips=%u present_requests=%u before=%08x after=%08x immutable=%u cancelled=%u\n",
  lp_frames,lp_flips,lp_present_requests,lp_before,after,(ULONG)(lp_before==after),(ULONG)quit);
 return lp_before==after && !lp_present_requests;
}
