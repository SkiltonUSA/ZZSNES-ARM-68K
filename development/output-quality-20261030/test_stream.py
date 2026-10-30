from pathlib import Path
import subprocess
w=Path(__file__).parent;o=w.parents[1]/'.context/quality-audio-native';o.mkdir(parents=True,exist_ok=True)
s=(w/'ax_stream.h').read_text()
s='\n'.join(l for l in s.splitlines() if not l.startswith('#include <'))
for reg in ('a0','a1','a2'):s=s.replace(f' __asm("{reg}")','')
(o/'stream-under-test.h').write_text(s)
c=r'''
#include <assert.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef uintptr_t ULONG;typedef unsigned char UBYTE;
struct Library {int unused;};struct Task {int unused;};struct MsgPort {int unused;};
struct IORequest {void *io_Device;};struct AHIRequest {struct IORequest ahir_Std;unsigned ahir_Version;};
struct AHIAudioCtrl {int unused;};struct AHISoundMessage {unsigned ahism_Channel;};
struct Hook {ULONG (*h_Entry)();void*h_Data;};struct AHISampleInfo {ULONG ahisi_Type,ahisi_Length;void*ahisi_Address;};
#define EM_AUDIO_FILE "test-audio.cfg"
#define EM_AUDIO_PREFS_FILE "test-ahi.prefs"
extern struct Library *AHIBase;
static unsigned output_choice,source_rate=32000,hifi_order;
#define AHINAME "ahi.device"
#define AHI_NO_UNIT 255
#define AHI_INVALID_ID (~(ULONG)0)
#define MEMF_PUBLIC 1
#define MEMF_CLEAR 2
#define TAG_DONE 0UL
#define AHIDB_BufferLen 1UL
#define AHIDB_Name 2UL
#define AHIDB_Stereo 3UL
#define AHIDB_Bits 4UL
#define AHIDB_Realtime 5UL
#define AHIDB_MaxPlaySamples 6UL
#define AHIA_AudioID 1UL
#define AHIA_MixFreq 2UL
#define AHIA_Channels 3UL
#define AHIA_Sounds 4UL
#define AHIA_SoundFunc 5UL
#define AHIC_Play 6UL
#define AHIC_MixFreq_Query 7UL
#define AHIST_DYNAMICSAMPLE 1
#define AHISF_IMM 1
#define TRUE 1
#define FALSE 0
static unsigned allocs,calls,fail_at,device,control,loaded,open_error,missing,bad_caps,query_error,start_error;
static void *allocate(size_t n){if(++calls==fail_at)return NULL;void*p=calloc(1,n);assert(p);allocs++;return p;}
static void release(void*p){assert(p&&allocs);free(p);allocs--;}
static struct Task*FindTask(void*p){(void)p;return (struct Task*)1;}
static struct MsgPort*CreateMsgPort(void){return allocate(sizeof(struct MsgPort));}
static void*CreateIORequest(struct MsgPort*p,size_t n){assert(p);return allocate(n);}
static void DeleteIORequest(struct IORequest*p){assert(!device&&!control);release(p);}
static void DeleteMsgPort(struct MsgPort*p){assert(!device&&!control);release(p);}
static void*AllocVec(size_t n,int flags){assert(flags==(MEMF_PUBLIC|MEMF_CLEAR));return allocate(n);}
static void FreeVec(void*p){assert(!control);release(p);}
static int OpenDevice(const char*name,int unit,struct IORequest*r,int flags){assert(!strcmp(name,AHINAME)&&unit==255&&!flags);if(open_error)return 1;r->io_Device=r;device=1;return 0;}
static void CloseDevice(struct IORequest*r){assert(r&&device&&!control);device=0;}
static ULONG AHI_NextAudioID(ULONG id){if(missing)return AHI_INVALID_ID;
 if(id==AHI_INVALID_ID)return hifi_order==2?124:123;
 if(hifi_order&&id==(hifi_order==2?124u:123u))return hifi_order==2?123:124;
 return AHI_INVALID_ID;}
static int AHI_GetAudioAttrs(ULONG id,struct AHIAudioCtrl*c,...){
 assert(AHIBase);(void)id;va_list a;va_start(a,c);ULONG tag;
 while((tag=va_arg(a,ULONG))!=TAG_DONE){ULONG v=va_arg(a,ULONG);
  if(tag==AHIDB_Name)strcpy((char*)v,output_choice==4?"ZZ9000AX:16 bit Stereo":id==124?"Paula:HiFi 14 bit stereo calibrated":"Paula:14 bit stereo");
  else if(tag==AHIDB_Stereo||tag==AHIDB_Realtime)*(ULONG*)v=bad_caps?0:1;
  else if(tag==AHIDB_Bits)*(ULONG*)v=output_choice==4?16:14;
  else if(tag==AHIDB_MaxPlaySamples)*(ULONG*)v=3840;
 }
 va_end(a);return 1;
}
static struct AHIAudioCtrl*AHI_AllocAudio(ULONG first,...){assert(first==AHIA_AudioID);va_list a;va_start(a,first);ULONG selected=va_arg(a,ULONG);assert(selected==(output_choice==4?(hifi_order==2?124:123):hifi_order?124:123));assert(va_arg(a,ULONG)==AHIA_MixFreq);ULONG mix=va_arg(a,ULONG);va_end(a);assert(mix==(output_choice==4?32000:output_choice==3?28000:22050));struct AHIAudioCtrl*c=allocate(sizeof(*c));if(c)control=1;return c;}
static void AHI_FreeAudio(struct AHIAudioCtrl*c){assert(control);control=loaded=0;release(c);}
static ULONG AHI_ControlAudio(struct AHIAudioCtrl*c,ULONG tag,...){assert(c&&control);
 va_list a;va_start(a,tag);if(tag==AHIC_MixFreq_Query){ULONG ptr=va_arg(a,ULONG);*(ULONG*)ptr=output_choice==4?32000:output_choice==3?28000:22050;}va_end(a);
 return (tag==AHIC_MixFreq_Query&&query_error)||(tag==AHIC_Play&&start_error);
}
static ULONG AHI_LoadSound(ULONG i,ULONG type,struct AHISampleInfo*s,struct AHIAudioCtrl*c){
 assert(i==loaded&&type==AHIST_DYNAMICSAMPLE&&s->ahisi_Address&&s->ahisi_Length>=3840&&c);
 if(++calls==fail_at)return 1;loaded++;return 0;
}
static void AHI_SetSound(unsigned ch,unsigned sound,unsigned off,unsigned len,struct AHIAudioCtrl*c,unsigned flags){assert(!ch&&sound==3&&!off&&len==(source_rate+49)/50&&c&&flags==1);}
static void AHI_SetVol(unsigned ch,unsigned vol,unsigned pan,struct AHIAudioCtrl*c,unsigned flags){assert(!ch&&vol==65536&&pan==32768&&c&&flags==1);}
static void AHI_SetFreq(unsigned ch,unsigned rate,struct AHIAudioCtrl*c,unsigned flags){assert(!ch&&rate==source_rate&&c&&flags==1);}
static unsigned delay_calls;static void Delay(unsigned ticks){assert(ticks==1);delay_calls++;}
static void Signal(struct Task*t,ULONG mask){assert(t&&mask);}
#include "stream-under-test.h"
static void run(unsigned failure,unsigned openfail,unsigned nomode,unsigned caps,unsigned query,unsigned start){
 struct ax_stream s;calls=0;fail_at=failure;open_error=openfail;missing=nomode;bad_caps=caps;query_error=query;start_error=start;
 int ok=ax_open(&s,source_rate,1,2,4);
 if(ok){assert(!failure&&!openfail&&!nomode&&!caps&&!query);assert(s.capacity>=3840&&s.silent_frames==(source_rate+49)/50&&loaded==4);assert(ax_start(&s)==!start);
 if(!start){delay_calls=0;assert(!ax_ready(&s)&&delay_calls==25);s.q.callbacks=1;delay_calls=0;assert(ax_ready(&s)&&!delay_calls);}
 ax_close(&s);}
 else assert(failure||openfail||nomode||caps||query);
 assert(!allocs&&!device&&!control&&!AHIBase);
}
int main(void){for(hifi_order=0;hifi_order<3;hifi_order++)for(source_rate=32000;source_rate<=32040;source_rate+=40)for(output_choice=2;output_choice<=4;output_choice++){assert(em_audio_save(output_choice));assert(em_audio_load()==output_choice);run(0,0,0,0,0,0);unsigned count=calls;for(unsigned i=1;i<=count;i++)run(i,0,0,0,0,0);
 run(0,1,0,0,0,0);run(0,0,1,0,0,0);run(0,0,0,1,0,0);run(0,0,0,0,1,0);run(0,0,0,0,0,1);
 }
 /* Auto must open AHI before asking for the registered unit-0 name. */
 unsigned char pref[52]={0};memcpy(pref,"FORM",4);pref[7]=44;memcpy(pref+8,"PREFAHIU",8);pref[19]=32;pref[24]=0;pref[25]=2;pref[27]=12;
 FILE*f=fopen(EM_AUDIO_PREFS_FILE,"wb");assert(f);assert(fwrite(pref,1,52,f)==52);fclose(f);output_choice=2;assert(em_audio_save(0));run(0,0,0,0,0,0);
 remove(EM_AUDIO_FILE);remove(EM_AUDIO_PREFS_FILE);
 puts("PASS actual per-app API adapter: allocation/load failures, mode capabilities, HiFi preference in either order and legacy fallback, 32k device clock for both sources, callback liveness timeout, query/start failure, free-before-storage cleanup");}
'''
(o/'stream.c').write_text(c)
subprocess.run(['clang','-std=c99','-O2','-g','-fsanitize=address,undefined','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-Wno-unused-function','-I'+str(w),str(o/'stream.c'),'-o',str(o/'stream')],check=True)
subprocess.run([str(o/'stream')],cwd=o,check=True)
