import subprocess,tempfile,sys
from pathlib import Path
s=(Path(sys.argv[1])/'player_audio.h').read_text();s=s[s.index('static int player_audio_room(void){'):]
pre=r'''#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef uint32_t ULONG;typedef void* APTR;
#define AUDIO_SLOTS 8u
#define MB_REPLY 0u
#define NES_AUDIO 96u
static unsigned char shared[4096];
static struct {unsigned count;unsigned char data[2048];}audio_queue[8];
static ULONG audio_write,audio_read,audio_error,audio_opened=1,audio_done,audio_highwater,audio_samples,audio_copy_us,audio_wait_us;
static int direct_locked,quit,player_muted,events_seen;
static ULONG ad_get(void *p,unsigned n){ULONG v;memcpy(&v,(char*)p+n,4);return v;}
static ULONG us(void){return 1000;}
static void Forbid(void){}static void Permit(void){}
static void pull(void *p,unsigned n,void *ctx){(void)p;(void)n;(void)ctx;}
static struct {void(*pull)(void*,unsigned,void*);}io={pull},*syncio=&io;
static void CopyMem(void *s,void*d,unsigned n){memcpy(d,s,n);}
static void events(void){assert(!direct_locked);events_seen++;audio_read+=3;}
static int profile_pause(unsigned p,unsigned t){(void)p;(void)t;assert(!direct_locked);return 1;}
'''
test=r'''int main(void){
 ULONG n=533;memcpy(shared+40,&n,4);memset(shared+NES_AUDIO,0x79,n*2);
 audio_write=UINT32_MAX;audio_read=audio_write-7;direct_locked=1;
 assert(player_audio_room());assert(player_audio_frame());
 assert(!events_seen&&audio_write==0&&audio_queue[7].count==533&&audio_queue[7].data[1065]==0x79);
 assert(!player_audio_room());
 /* Caller drains PAN before entering full-ring input/wait branch. */
 direct_locked=0;assert(player_audio_frame());assert(events_seen==1&&audio_samples==1066);
 n=1025;memcpy(shared+40,&n,4);assert(!player_audio_frame());
 puts("PASS PCM: counter wrap, available-slot copy without events, full-ring unlocked wait, bounded metadata");
}'''
with tempfile.TemporaryDirectory() as tmp:
 f=Path(tmp)/'test.c';f.write_text(pre+s+test);out=Path(tmp)/'test';subprocess.run(['clang','-std=c99','-Wall','-Wextra','-Werror',str(f),'-o',str(out)],check=True);subprocess.run([str(out)],check=True)
