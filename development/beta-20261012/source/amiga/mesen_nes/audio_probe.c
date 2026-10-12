/* GPL-3.0-or-later. Standalone silent AHI worker lifecycle probe.
 * Does not start Core1, open a screen or access any ZZ9000 register. */
#include <exec/memory.h>
#include <devices/ahi.h>
#include <devices/timer.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/timer.h>
#include <stdio.h>
#include <string.h>
struct Device *TimerBase;
static ULONG audio_samples,audio_gaps,audio_wait_us,audio_copy_us,audio_send_us;
static int player_muted,quit;
#define MB_REPLY 0
#define NES_AUDIO 128
static UBYTE shared[128+2048];
static void pull(volatile UBYTE *p,ULONG n,void *ctx){(void)p;(void)n;(void)ctx;}
static struct {void (*pull)(volatile UBYTE *,ULONG,void *);} ops={pull},*syncio=&ops;
static ULONG ad_get(const UBYTE *p,unsigned off){ULONG n;CopyMem((APTR)(p+off),&n,4);return n;}
static ULONG us(void){struct timeval t;GetSysTime(&t);return t.tv_secs*1000000u+t.tv_micro;}
static void events(void){if(SetSignal(0,SIGBREAKF_CTRL_C)&SIGBREAKF_CTRL_C)quit=1;}
static int pause_us(ULONG n){(void)n;Delay(1);return 1;}
#include "player_audio.h"
int main(void){
 struct MsgPort *port=CreateMsgPort();struct timerequest *timer=NULL;
 int rc=20,opened=0;ULONG i,n=533;
 if(!port)goto done;
 timer=(struct timerequest *)CreateIORequest(port,sizeof(*timer));if(!timer)goto done;
 if(OpenDevice(TIMERNAME,UNIT_MICROHZ,(struct IORequest *)timer,0))goto done;
 opened=1;TimerBase=timer->tr_node.io_Device;
 puts("AUDIO_PROBE starting DOS worker; no ARM or display access");fflush(stdout);
 if(!player_audio_open())goto done;
 puts("AUDIO_PROBE AHI opened");fflush(stdout);
 CopyMem(&n,shared+40,4);
 for(i=0;i<32&&!quit;i++){
  if(!player_audio_frame()||!player_audio_submit())goto done;
  if(i==0){player_audio_flush();Delay(4);} /* Flush below startup threshold. */
  Delay(1);events();
 }
 rc=0;
done:
 player_audio_close();
 if(audio_error||audio_submitted!=audio_samples||audio_completed!=audio_samples)rc=20;
 printf("AUDIO_PROBE exit=%d samples=%u gaps=%u wait_us=%u copy_us=%u main_send_us=%u\n",
  rc,audio_samples,audio_gaps,audio_wait_us,audio_copy_us,audio_send_us);fflush(stdout);
 if(opened)CloseDevice((struct IORequest *)timer);
 if(timer)DeleteIORequest((struct IORequest *)timer);
 if(port)DeleteMsgPort(port);
 return rc;
}
