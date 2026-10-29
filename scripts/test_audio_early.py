from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
for overlay,channels in [('expanded-20261019',2)]:
 source=root/'development'/overlay/'host/player_audio.h';s=source.read_text();s=s[s.index('static void player_audio_flush'):]
 with tempfile.TemporaryDirectory() as tmp:
  p=Path(tmp);(p/'test.c').write_text(r'''
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
typedef uint32_t ULONG;typedef void*APTR;
#define EM_AUDIO_EARLY
#define AUDIO_SLOTS 8u
#define MB_REPLY 0
#define NES_AUDIO 128
static uint32_t samples,clock_us;
static unsigned char memory[8192],*shared=memory;
static unsigned ad_get(void*p,unsigned n){(void)p;(void)n;return samples;}
struct block{unsigned count;unsigned char data[4096];};
static struct block q[8],*audio_queue=q;
static unsigned audio_write,audio_read,audio_highwater,audio_samples,audio_force;
static unsigned audio_wait_us,audio_copy_us;
static int audio_opened=1,audio_done,audio_error,quit,player_muted,wakes;
static ULONG us(void){return clock_us+=1000;}
static void Forbid(void){}static void Permit(void){}static void events(void){}
static int profile_pause(int a,int b){(void)a;(void)b;audio_read++;return 1;}
static void CopyMem(const void*s,void*d,unsigned n){memcpy(d,s,n);}
static void pull(void*p,unsigned n,void*c){(void)p;(void)n;(void)c;}
static struct{void(*pull)(void*,unsigned,void*);}io={pull},*syncio=&io;
static void audio_wake(void){wakes++;assert(audio_force||audio_write-audio_read>=3);}
'''+s+r'''
int main(void){unsigned cases=0;
 for(unsigned wrap=0;wrap<2;wrap++)for(unsigned n=1;n<=1024;n+=31)for(unsigned have=0;have<8;have++)for(unsigned mute=0;mute<2;mute++){
  memset(q,0xa5,sizeof q);memset(memory+NES_AUDIO,0x39,4096);
  audio_read=wrap?0xfffffffc:2;audio_write=audio_read+have;samples=n;wakes=0;player_muted=mute;
  unsigned index=audio_write%8;assert(player_audio_frame());assert(audio_write-audio_read==have+1);assert(q[index].count==n);
  for(unsigned i=0;i<n*CHANNELS*2;i++)assert(q[index].data[i]==(mute?0:0x39));
  for(unsigned i=n*CHANNELS*2;i<4096;i++)assert(q[index].data[i]==0xa5);
  assert(wakes==(have+1>=3));int before=wakes;assert(player_audio_submit());assert(wakes==before);cases++;
 }
 samples=1025;assert(!player_audio_frame());samples=0;assert(player_audio_frame());audio_error=1;assert(!player_audio_frame()&&!player_audio_submit());audio_error=0;
 audio_force=0;int before=wakes;player_audio_flush();assert(audio_force&&wakes==before+1);
 printf("PASS console audio: %u copy/mute/wrap/batch-wake cases, errors and short flush\n",cases);
}
''')
  subprocess.run(['clang','-std=c99','-O2','-fsanitize=address,undefined','-DCHANNELS='+str(channels),str(p/'test.c'),'-o',str(p/'test')],check=True)
  subprocess.run([str(p/'test')],check=True)
