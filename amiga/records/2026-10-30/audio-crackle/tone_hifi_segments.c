#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "ax_stream_segments.h"
/* HiFi captures are signed 32-bit; normalize their high word for comparison. */
static UBYTE *capture;
static volatile ULONG capture_bytes,capture_type,capture_calls;
#define CAPACITY 1600000u
static ULONG output_hook(struct Hook*h __asm("a0"),struct AHIAudioCtrl*c __asm("a2"),struct AHIEffOutputBuffer*m __asm("a1")){
 (void)h;(void)c;ULONG bytes=AHI_SampleFrameSize(m->ahieob_Type)*m->ahieob_Length;
 if(bytes<=CAPACITY-capture_bytes){CopyMem(m->ahieob_Buffer,capture+capture_bytes,bytes);capture_bytes+=bytes;}
 capture_type=m->ahieob_Type;capture_calls++;return 0;
}
/* A complete waveform is resident before playback: no producer starvation.
 * Compare one sound against 80 contiguous segments through SoundFunc. */
static volatile ULONG next_frame, total_frames, block_frames, boundaries, ending, ended;
static ULONG separate;
static ULONG segment_hook(struct Hook*h __asm("a0"),struct AHIAudioCtrl*c __asm("a2"),struct AHISoundMessage*m __asm("a1")){
 struct ax_stream*s=h->h_Data;if(m->ahism_Channel)return 0;boundaries++;
 if(next_frame<total_frames){ULONG n=total_frames-next_frame;if(n>block_frames)n=block_frames;
  AHI_SetSound(0,separate?4+next_frame/block_frames:0,separate?0:next_frame,n,c,0);next_frame+=n;
 }else{AHI_SetSound(0,3,0,s->silent_frames,c,0);if(++ending>=2)ended=1;}
 return 0;
}
int main(int argc,char**argv){
 struct ax_stream s;LONG sig=AllocSignal(-1);ULONG ticks=0;WORD *waveform=NULL;int rc=20;
 ULONG stereo=argc>1&&!strcmp(argv[1],"stereo"),rate=stereo?32040:32000;
 struct Hook hook;struct AHIEffOutputBuffer effect;
 setbuf(stdout,NULL);if(sig<0)return 20;
 capture=AllocVec(CAPACITY,MEMF_PUBLIC);if(!capture)goto done;
 if(!ax_open(&s,rate,stereo?AHIST_S16S:AHIST_M16S,stereo?4:2,1UL<<sig))goto done;
 printf("OPEN mode=%08x source=%u mix=%u\n",s.mode,rate,s.mixrate);
 memset(&hook,0,sizeof(hook));memset(&effect,0,sizeof(effect));
 hook.h_Entry=(ULONG(*)())output_hook;effect.ahie_Effect=AHIET_OUTPUTBUFFER;effect.ahieob_Func=&hook;
 if(AHI_SetEffect(&effect,s.control)){puts("EFFECT failed");goto close;}
 total_frames=rate*4;block_frames=argc>2&&!strcmp(argv[2],"whole")?total_frames:(stereo?1602:1600);separate=argc>2&&!strcmp(argv[2],"split");next_frame=block_frames;
 waveform=AllocVec((total_frames+s.capacity)*(stereo?4:2),MEMF_PUBLIC);if(!waveform)goto close;
 for(ULONG i=0;i<total_frames+s.capacity;i++){
  LONG wave=(i%320)<160?(LONG)(i%320):320-(LONG)(i%320);
  WORD sample=(wave-80)*64;waveform[i*(stereo?2:1)]=sample;if(stereo)waveform[i*2+1]=sample;
 }
 AHI_UnloadSound(0,s.control);
 {struct AHISampleInfo sample;sample.ahisi_Type=stereo?AHIST_S16S:AHIST_M16S;
  sample.ahisi_Address=waveform;sample.ahisi_Length=total_frames;
  if(AHI_LoadSound(0,AHIST_SAMPLE,&sample,s.control))goto close;
  if(separate)for(ULONG i=0;i<80;i++){
   sample.ahisi_Address=waveform+i*block_frames*(stereo?2:1);sample.ahisi_Length=s.capacity;
   if(AHI_LoadSound(4+i,AHIST_DYNAMICSAMPLE,&sample,s.control))goto close;
  }}
 s.hook.h_Entry=(ULONG(*)())segment_hook;
 if(AHI_ControlAudio(s.control,AHIC_Play,TRUE,TAG_DONE))goto close;s.playing=1;
 AHI_SetVol(0,0x10000,0x8000,s.control,AHISF_IMM);AHI_SetFreq(0,rate,s.control,AHISF_IMM);
 AHI_SetSound(0,separate?4:0,0,block_frames,s.control,AHISF_IMM);
 while(!ended){if(++ticks>400){puts("TIMEOUT");goto close;}Delay(1);}
 rc=0;
close:
 ax_close(&s);printf("RESULT rc=%d frames=%u block=%u boundaries=%u captured=%u type=%u calls=%u\n",(LONG)rc,total_frames,block_frames,boundaries,capture_bytes,capture_type,capture_calls);
 if(capture_type==AHIST_S16S||capture_type==AHIST_M16S||capture_type==10||capture_type==8){
  ULONG wide=capture_type==10||capture_type==8,channels=(capture_type==AHIST_S16S||capture_type==10)?2:1,frames=capture_bytes/(channels*(wide?4:2)),max=0,jumps=0;
  WORD*p=(WORD*)capture;
  for(ULONG i=s.mixrate/2+1;i+s.mixrate/2<frames;i++){
   LONG current=wide?((LONG*)capture)[i*channels]>>16:p[i*channels];
   LONG previous=wide?((LONG*)capture)[(i-1)*channels]>>16:p[(i-1)*channels];
   LONG delta=current-previous;if(delta<0)delta=-delta;
   if((ULONG)delta>max)max=delta;if(delta>512)jumps++;
  }
  printf("CONTINUITY frames=%u max_step=%u jumps_over_512=%u\n",frames,max,jumps);
 }
 {FILE*f=fopen("PROGDIR:mixed.raw","wb");if(f){fwrite(capture,1,capture_bytes,f);fclose(f);}}
done:
 if(waveform)FreeVec(waveform);if(capture)FreeVec(capture);FreeSignal(sig);return rc;
}
