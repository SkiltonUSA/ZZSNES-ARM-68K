/* GPL-3.0-or-later. Bounded headless benchmark, one mailbox command per frame. */
#include <exec/memory.h>
#include <devices/timer.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/timer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "wire.h"
#include "mmu.h"
struct Device *TimerBase;
static volatile uint8_t *shared;
static const struct ad_io *syncio;
static uint32_t sequence;
static const char *rom_path;
static ULONG frame_limit;
static struct timerequest *waitio;
static ULONG us(void){struct timeval t;GetSysTime(&t);return t.tv_secs*1000000u+t.tv_micro;}
int nes_arguments(int argc,char **argv){
 char *end;
 if(argc!=5){puts("Usage: ZZFceuxBench <nonce> RUN <ROM.nes> <frames 1..3600>");return -1;}
 rom_path=argv[3];errno=0;frame_limit=strtoul(argv[4],&end,10);
 if(errno||*end||!frame_limit||frame_limit>3600)return -1;
 return 3;
}
static int request(ULONG op,ULONG arg){
 ULONG start=us(),token=(sequence+1)^ad_get(shared,ZZ_CONTROL);volatile uint8_t *cmd=shared+MB_HOST,*reply=shared+MB_REPLY;
 ad_put(cmd,4,op);ad_put(cmd,8,token);ad_put(cmd,12,arg);syncio->push(cmd+4,60,NULL);syncio->barrier(NULL);
 ad_put(cmd,0,++sequence);syncio->push(cmd,4,NULL);
 do{
  syncio->pull(reply,96,NULL);
  if(ad_get(reply,0)==sequence){
   if(ad_get(reply,68)!=(token^ZZ_XOR)){puts("MAILBOX FAIL");return 0;}
   if(ad_get(reply,4)){printf("FCEUX error=%u op=%u\n",(ULONG)ad_get(reply,4),op);return 0;}
   return 1;
  }
  if(SetSignal(0,SIGBREAKF_CTRL_C)&SIGBREAKF_CTRL_C)return 0;
  waitio->tr_node.io_Command=TR_ADDREQUEST;waitio->tr_time.tv_secs=0;waitio->tr_time.tv_micro=1000;
  if(DoIO((struct IORequest *)waitio))return 0;
 }while((ULONG)(us()-start)<10000000u);
 puts("FCEUX request timeout");return 0;
}
static uint64_t armclock(void){return (uint64_t)ad_get(shared+MB_REPLY,12)<<32|ad_get(shared+MB_REPLY,8);}
int ff_window(volatile uint8_t *base,const struct ad_io *io,int connected){
 struct MsgPort *port=NULL;struct timerequest *timer=NULL;
 ULONG size=0,rate=0,t0,t1,frames=0,start=0,total_samples=0,total_nonzero=0;
 uint64_t c0,c1,core_ticks=0;int opened=0,rc=20;UBYTE *rom=NULL;FILE *f=NULL;
 (void)connected;shared=base;syncio=io;
 port=CreateMsgPort();if(!port)goto done;
 timer=(struct timerequest *)CreateIORequest(port,sizeof(*timer));if(!timer)goto done;
 if(OpenDevice(TIMERNAME,UNIT_MICROHZ,(struct IORequest *)timer,0))goto done;
 opened=1;TimerBase=timer->tr_node.io_Device;waitio=timer;
 rom=AllocVec(FC_ROM_LIMIT+1,MEMF_PUBLIC);if(!rom)goto done;
 f=fopen(rom_path,"rb");if(!f)goto done;size=fread(rom,1,FC_ROM_LIMIT+1,f);fclose(f);f=NULL;
 if(size<16||size>FC_ROM_LIMIT)goto done;
 memcpy((void *)(base+FC_ROM),rom,size);io->push(base+FC_ROM,size,NULL);io->barrier(NULL);
 printf("FCEUX INIT bytes=%u mapper=%u frames=%u NTSC rate=32000 quality=0 frameskip=0 sprites=8\n",size,(ULONG)((rom[6]>>4)|(rom[7]&240)),frame_limit);fflush(stdout);
 if(!request(FC_INIT,size))goto done;
 FreeVec(rom);rom=NULL;
 if(!request(FC_CLOCK,0))goto done;
 c0=armclock();t0=us();Delay(100);
 if(!request(FC_CLOCK,0))goto done;
 c1=armclock();t1=us();rate=(ULONG)((c1-c0)*1000000u/(t1-t0));
 printf("CLOCK timer_hz=%u\n",rate);fflush(stdout);
 if(rate<320000000u||rate>345000000u)goto done;
 start=us();
 for(frames=0;frames<frame_limit;frames++){
  ULONG ticks,samples,cycles;
  if(!request(FC_FRAME,0))goto done;
  ticks=ad_get(base+MB_REPLY,28);samples=ad_get(base+MB_REPLY,36);cycles=ad_get(base+MB_REPLY,20);
  if(ad_get(base+MB_REPLY,16)!=frames+1||!ticks||samples<400||samples>600||cycles<(frames+1)*29000u||cycles>(frames+1)*30500u){puts("FRAME INVARIANT FAIL");goto done;}
  core_ticks+=ticks;total_samples+=samples;total_nonzero+=ad_get(base+MB_REPLY,40);
  printf("FRAME n=%u cycles=%u video=%08x audio=%08x samples=%u nonzero=%u ticks=%u\n",frames+1,cycles,
   (ULONG)ad_get(base+MB_REPLY,24),(ULONG)ad_get(base+MB_REPLY,32),samples,(ULONG)ad_get(base+MB_REPLY,40),ticks);
 }
 rc=0;
done:
 printf("FCEUX END exit=%d frames=%u elapsed_us=%u core_total_us=%u samples=%u nonzero=%u\n",rc,frames,start?(ULONG)(us()-start):0,rate?(ULONG)(core_ticks*1000000u/rate):0,total_samples,total_nonzero);fflush(stdout);
 if(f)fclose(f);
 if(rom)FreeVec(rom);
 if(opened)CloseDevice((struct IORequest *)timer);
 if(timer)DeleteIORequest((struct IORequest *)timer);
 if(port)DeleteMsgPort(port);
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
